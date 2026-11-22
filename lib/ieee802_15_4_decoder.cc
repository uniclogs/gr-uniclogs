// Thin GNU Radio wrapper. All of the frame finding and decoding lives in
// FrameExtractor / FrameDecoder, which know nothing about GNU Radio.
//
//   decode(in, len)  ->  FrameExtractor::process()  ->  (finished frame?)
//                                                        -> pack bits -> metadata

#include "frame_extractor.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/pdu.h>
#include <gnuradio/uniclogs/ieee802_15_4_decoder.h>
#include <gnuradio/uniclogs/metadata.h>
#include <pmt/pmt.h>

namespace gr {
namespace uniclogs {

namespace {
// The message arrives one bit per element. It is packed into bytes for the PDU;
// true means the first bit received becomes the least significant bit of a byte
constexpr bool kPackLsbFirst = true;
} // namespace

ieee802_15_4_decoder::sptr
ieee802_15_4_decoder::make(const std::vector<uint8_t>& preamble,
                           size_t preamble_threshold,
                           const std::vector<uint8_t>& sync,
                           size_t sync_threshold,
                           bool FEC_toggle,
                           bool invert_payload,
                           bool length_little_endian)
{

    return ieee802_15_4_decoder::sptr(new ieee802_15_4_decoder(preamble,
                                                               preamble_threshold,
                                                               sync,
                                                               sync_threshold,
                                                               FEC_toggle,
                                                               invert_payload,
                                                               length_little_endian));
}

ieee802_15_4_decoder::ieee802_15_4_decoder(const std::vector<uint8_t>& preamble,
                                           size_t preamble_threshold,
                                           const std::vector<uint8_t>& sync,
                                           size_t sync_threshold,
                                           bool FEC_toggle,
                                           bool invert_payload,
                                           bool length_little_endian)
    : decoder("IEEE-802.15.4", "1.1", sizeof(uint8_t), uniclogs_fec::kMaxMessageBytes),
      d_extractor(new uniclogs_fec::FrameExtractor(preamble,
                                                   preamble_threshold,
                                                   sync,
                                                   sync_threshold,
                                                   invert_payload,
                                                   length_little_endian)),
      d_preamble(preamble.size() * 8),
      d_preamble_shift_reg(preamble.size() * 8),
      d_preamble_len(preamble.size() * 8),
      d_preamble_thrsh(preamble_threshold),
      d_sync(sync.size() * 8),
      d_sync_shift_reg(sync.size() * 8),
      d_sync_len(sync.size() * 8),
      d_sync_thrsh(sync_threshold),
      d_len(((size_t)1 << (2 * 8))),
      d_length_field_len(2),
      d_state(SEARCHING),
      d_cnt(0),
      d_frame_start_idx(0),
      d_max_frame_len(((size_t)1 << (2 * 8))),
      d_FEC_toggle(FEC_toggle)
{
    for (uint8_t b : preamble) {
        d_preamble <<= (b >> 7);
        d_preamble <<= ((b >> 6) & 0x1);
        d_preamble <<= ((b >> 5) & 0x1);
        d_preamble <<= ((b >> 4) & 0x1);
        d_preamble <<= ((b >> 3) & 0x1);
        d_preamble <<= ((b >> 2) & 0x1);
        d_preamble <<= ((b >> 1) & 0x1);
        d_preamble <<= (b & 0x1);
    }
    for (uint8_t b : sync) {
        d_sync <<= (b >> 7);
        d_sync <<= ((b >> 6) & 0x1);
        d_sync <<= ((b >> 5) & 0x1);
        d_sync <<= ((b >> 4) & 0x1);
        d_sync <<= ((b >> 3) & 0x1);
        d_sync <<= ((b >> 2) & 0x1);
        d_sync <<= ((b >> 1) & 0x1);
        d_sync <<= (b & 0x1);
    }
    /* Parameters checking */
    if (d_max_frame_len == 0) {
        throw std::invalid_argument("The maximum frame size should be at least 1 byte");
    }

    if (d_sync_len < 8) {
        throw std::invalid_argument("SYNC word should be at least 8 bits");
    }

    if (d_preamble_len && d_preamble_len < 2 * d_preamble_thrsh) {
        throw std::invalid_argument("Too many error bits are allowed for the preamble."
                                    "Consider lowering the threshold");
    }

    if (d_sync_len < 2 * d_sync_thrsh) {
        throw std::invalid_argument("Too many error bits are allowed for the sync word. "
                                    "Consider lowering the threshold");
    }

    d_pdu = new uint8_t[d_len + d_length_field_len];

    reset();
    // FrameExtractor throws std::invalid_argument on bad thresholds/sync length
}

ieee802_15_4_decoder::~ieee802_15_4_decoder() {}

void ieee802_15_4_decoder::reset()
{
    if (d_FEC_toggle) {
        d_extractor->reset();
    }
    d_cnt = 0;
    /* There are case that no preamble is used */
    if (d_preamble_len) {
        d_state = SEARCHING;
    } else {
        d_state = SEARCHING_SYNC;
    }
    d_preamble_shift_reg.reset();
    d_sync_shift_reg.reset();
}

decoder_status_t ieee802_15_4_decoder::decode(const void* in, int len)
{
    decoder_status_t status;
    if (len <= 0) {
        return status;
    }

    if (d_FEC_toggle) {
        // Uses up to `len` items. It always uses at least one, so the scheduler
        // never sees "consumed nothing". It stops right after a frame completes.
        status.consumed = static_cast<int>(d_extractor->process(
            static_cast<const uint8_t*>(in), static_cast<size_t>(len)));

        if (d_extractor->frame_ready()) {
            // The extractor counts every item it has been given, which is exactly
            // nitems_read(), so these are absolute stream positions.
            const uint64_t start = d_extractor->frame_start_index();
            const uint64_t end = d_extractor->frame_end_index();

            const std::vector<uint8_t> pdu =
                uniclogs_fec::pack_bits(d_extractor->take_message(), kPackLsbFirst);

            metadata::add_decoder(status.data, this);
            metadata::add_time_iso8601(status.data);
            metadata::add_sample_start(status.data, start);
            metadata::add_sample_cnt(status.data, end - start);
            // An empty vector's data() may be null, so give a zero-length PDU a valid
            // pointer
            static const uint8_t kNoBytes = 0;
            metadata::add_pdu(
                status.data, pdu.empty() ? &kNoBytes : pdu.data(), pdu.size());
            status.decode_success = true;
        }
    } else {
        switch (d_state) {
        case SEARCHING:
            status.consumed = search_preamble((const uint8_t*)in, len);
            break;
        case SEARCHING_SYNC:
            status.consumed = search_sync((const uint8_t*)in, len);
            break;
        case DECODING_GENERIC_FRAME_LEN:
            status.consumed = decode_frame_len((const uint8_t*)in);
            break;
        case DECODING_PAYLOAD:
            decode_payload(status, (const uint8_t*)in, len);
            break;
        default:
            throw std::runtime_error("ieee802_15_4_decoder: Invalid state");
        }
    }


    incr_nitems_read(static_cast<size_t>(status.consumed));
    return status;
}


int ieee802_15_4_decoder::search_preamble(const uint8_t* in, int len)
{
    for (int i = 0; i < len; i++) {
        d_preamble_shift_reg <<= in[i];
        shift_reg tmp = d_preamble_shift_reg ^ d_preamble;
        if (tmp.count() <= d_preamble_thrsh) {
            d_state = SEARCHING_SYNC;
            d_frame_start_idx = nitems_read() + i + 1;
            d_cnt = 0;
            return i + 1;
        }
    }
    return len;
}

int ieee802_15_4_decoder::search_sync(const uint8_t* in, int len)
{
    for (int i = 0; i < len; i++) {
        d_sync_shift_reg <<= in[i];
        shift_reg tmp = d_sync_shift_reg ^ d_sync;
        d_cnt++;
        if (tmp.count() <= d_sync_thrsh) {
            d_state = DECODING_GENERIC_FRAME_LEN;
            d_cnt = 0;
            return i + 1;
        }

        /* The sync word should be available by now */
        if (d_cnt > d_preamble_len * 2 + d_sync_len + d_sync_thrsh) {
            reset();
            return i + 1;
        }
    }
    return len;
}

int ieee802_15_4_decoder::decode_frame_len(const uint8_t* in)
{
    uint32_t b = 0x0;
    auto* bytes = new uint8_t[d_length_field_len];
    for (size_t i = 0; i < d_length_field_len; i++) {
        uint8_t b = 0x0;
        b = in[i * 8 + 7] << 0;
        b |= in[i * 8 + 6] << 1;
        b |= in[i * 8 + 5] << 2;
        b |= in[i * 8 + 4] << 3;
        b |= in[i * 8 + 3] << 4;
        b |= in[i * 8 + 2] << 5;
        b |= in[i * 8 + 1] << 6;
        b |= in[i * 8 + 0] << 7;
        bytes[i] = b;
    }

    b = (bytes[0] << 8 | bytes[1]);
    // Ref to IEEE 802.15.4 2012 18.1.1.3
    // Length field is [5:15]
    // bit 5 is L_10 (MSB), bit 15 is L_0 (LSB)
    b = 0x07FF & b;
    d_len = b;
    d_cnt = d_length_field_len;
    d_state = DECODING_PAYLOAD;
    return 8 * d_length_field_len;
}

void ieee802_15_4_decoder::decode_payload(decoder_status_t& status,
                                          const uint8_t* in,
                                          int len)
{
    const int s = len / 8;
    size_t flen = d_len;
    for (int i = 0; i < s; i++) {
        uint8_t b = 0x0;
        b = in[i * 8 + 0];
        b |= in[i * 8 + 1] << 1;
        b |= in[i * 8 + 2] << 2;
        b |= in[i * 8 + 3] << 3;
        b |= in[i * 8 + 4] << 4;
        b |= in[i * 8 + 5] << 5;
        b |= in[i * 8 + 6] << 6;
        b |= in[i * 8 + 7] << 7;
        d_pdu[d_cnt++] = b;
        status.consumed = (i + 1) * 8;

        if (d_cnt == flen + d_length_field_len) {

            metadata::add_decoder(status.data, this);
            metadata::add_time_iso8601(status.data);
            metadata::add_sample_start(status.data, d_frame_start_idx);
            metadata::add_sample_cnt(status.data,
                                     nitems_read() + (i + 1) * 8 - d_frame_start_idx);

            status.decode_success = true;
            metadata::add_pdu(status.data, d_pdu + d_length_field_len, flen);
            reset();
            return;
        }
    }
    status.consumed = s * 8;
}

} /* namespace uniclogs */
} /* namespace gr */
