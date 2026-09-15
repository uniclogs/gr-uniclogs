#include "viterbi/viterbi.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/pdu.h>
#include <gnuradio/uniclogs/ieee802_15_4_variant_decoder.h>
#include <gnuradio/uniclogs/metadata.h>
#include <pmt/pmt.h>

namespace gr {
namespace uniclogs {

ieee802_15_4_variant_decoder::sptr
ieee802_15_4_variant_decoder::make(const std::vector<uint8_t>& preamble,
                                   size_t preamble_threshold,
                                   const std::vector<uint8_t>& sync,
                                   size_t sync_threshold,
                                   int constraint,
                                   const std::vector<int>& polynomials,
                                   bool fec)
{
    return ieee802_15_4_variant_decoder::sptr(
        new ieee802_15_4_variant_decoder(preamble,
                                         preamble_threshold,
                                         sync,
                                         sync_threshold,
                                         constraint,
                                         polynomials,
                                         fec));
}

ieee802_15_4_variant_decoder::ieee802_15_4_variant_decoder(
    const std::vector<uint8_t>& preamble,
    size_t preamble_threshold,
    const std::vector<uint8_t>& sync,
    size_t sync_threshold,
    int constraint,
    const std::vector<int>& polynomials,
    bool fec)
    : decoder("IEEE-802.15.4",
              "1.1",
              sizeof(uint8_t),
              ((size_t)1 << (2 * 8))),
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
      d_fec(fec),
      d_frame_start_idx(0),
      d_max_frame_len(((size_t)1 << (2 * 8))),
      d_codec(constraint, polynomials)
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
}

ieee802_15_4_variant_decoder::~ieee802_15_4_variant_decoder() { delete[] d_pdu; }

void ieee802_15_4_variant_decoder::reset()
{

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

decoder_status_t ieee802_15_4_variant_decoder::decode(const void* in, int len)
{
    decoder_status_t status;
    switch (d_state) {
    case SEARCHING:
        status.consumed = search_preamble((const uint8_t*)in, len);
        break;
    case SEARCHING_SYNC:
        status.consumed = search_sync((const uint8_t*)in, len);
        break;
    case DECODING_GENERIC_FRAME_LEN:
        if (d_fec == true) {
            status.consumed = decode_frame_len((const uint8_t*)in);
        } else {
            status.consumed = decode_frame_len_fec((const uint8_t*)in);
        }
        break;
    case DECODING_PAYLOAD:
        decode_payload(status, (const uint8_t*)in, len);
        break;
    default:
        throw std::runtime_error("ieee802_15_4_variant_decoder: Invalid state");
    }
    incr_nitems_read(static_cast<uint64_t>(status.consumed));
    return status;
}

/**
 * To greatly simplify the logic, the decoder requests that the number of
 * input items should be a multiple of 8
 * @return 8
 */
size_t ieee802_15_4_variant_decoder::input_multiple() const { return 8; }

int ieee802_15_4_variant_decoder::search_preamble(const uint8_t* in, int len)
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

int ieee802_15_4_variant_decoder::search_sync(const uint8_t* in, int len)
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

int ieee802_15_4_variant_decoder::decode_frame_len(const uint8_t* in)
{
    uint32_t b = 0x0;
    uint8_t* bytes = new uint8_t[d_length_field_len];
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

int ieee802_15_4_variant_decoder::decode_frame_len_fec(const uint8_t* in)
{
    int phr_encoded_len = 30;
    uint8_t encoded_phr[phr_encoded_len];

    printf("stuff here\n");
    for (int i; i <= phr_encoded_len; i++) {
        encoded_phr[i] = in[i];
    }
    printf("stuff here again\n");

    // cast to pmt for compat with viterbi lib
    // pmt::pmt_t pdu_vector = pdu::make_pdu_vector(
    //     gr::types::vector_type::byte_t, encoded_phr, phr_encoded_len);
    // std::vector<uint8_t> msg = pmt::u8vector_elements(pmt::cdr(pdu_vector));
    std::string bits = (char*)encoded_phr;
    // for (auto b : msg) {
    //     bits.push_back(b ? '1' : '0');
    // }
    //
    printf("stuff here agian 2again\n");
    // decode phr
    std::string outbits = d_codec.Decode(bits);
    std::vector<uint8_t> out;
    for (auto b : outbits) {
        out.push_back(b == '1');
    }

    printf("stuff here again again\n");
    uint8_t* bytes = new uint8_t[d_length_field_len];
    for (size_t i = 0; i < d_length_field_len; i++) {
        printf("%x \r", out[i]);
        uint8_t b = 0x0;
        b = out[i * 8 + 7] << 0;
        b |= out[i * 8 + 6] << 1;
        b |= out[i * 8 + 5] << 2;
        b |= out[i * 8 + 4] << 3;
        b |= out[i * 8 + 3] << 4;
        b |= out[i * 8 + 2] << 5;
        b |= out[i * 8 + 1] << 6;
        b |= out[i * 8 + 0] << 7;
        bytes[i] = b;
    }

    uint32_t b = (bytes[0] << 8 | bytes[1]);
    printf("%x \r", b);

    d_len = b;
    d_state = DECODING_PAYLOAD;
    return 8 * phr_encoded_len;
}

void ieee802_15_4_variant_decoder::decode_payload(decoder_status_t& status,
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
