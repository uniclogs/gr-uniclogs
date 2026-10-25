// Thin GNU Radio wrapper. All of the frame finding and decoding lives in
// FrameExtractor / FrameDecoder, which know nothing about GNU Radio.
//
//   decode(in, len)  ->  FrameExtractor::process()  ->  (finished frame?)
//                                                        -> pack bits -> metadata

#include <gnuradio/uniclogs/ieee802_15_4_variant_decoder.h>
#include <gnuradio/uniclogs/metadata.h>

#include "frame_extractor.h"

namespace gr {
namespace uniclogs {

namespace {
// The message arrives one bit per element. It is packed into bytes for the PDU;
// true means the first bit received becomes the least significant bit of a byte
constexpr bool kPackLsbFirst = true;
} // namespace

ieee802_15_4_variant_decoder::sptr
ieee802_15_4_variant_decoder::make(const std::vector<uint8_t>& preamble,
                                   size_t preamble_threshold,
                                   const std::vector<uint8_t>& sync,
                                   size_t sync_threshold,
                                   bool invert_payload,
                                   bool length_little_endian) {

    return ieee802_15_4_variant_decoder::sptr(
        new ieee802_15_4_variant_decoder(preamble,
                                         preamble_threshold,
                                         sync,
                                         sync_threshold,
                                         invert_payload,
                                         length_little_endian));
}

ieee802_15_4_variant_decoder::ieee802_15_4_variant_decoder(
    const std::vector<uint8_t>& preamble,
    size_t preamble_threshold,
    const std::vector<uint8_t>& sync,
    size_t sync_threshold,
    bool invert_payload,
    bool length_little_endian)
    : decoder("IEEE-802.15.4",
              "1.1",
              sizeof(uint8_t),
              uniclogs_fec::kMaxMessageBytes),
      d_extractor(new uniclogs_fec::FrameExtractor(preamble,
                                                   preamble_threshold,
                                                   sync,
                                                   sync_threshold,
                                                   invert_payload,
                                                   length_little_endian)) {

    // FrameExtractor throws std::invalid_argument on bad thresholds/sync length
}

ieee802_15_4_variant_decoder::~ieee802_15_4_variant_decoder() {}

void ieee802_15_4_variant_decoder::reset() { d_extractor->reset(); }

decoder_status_t ieee802_15_4_variant_decoder::decode(const void* in, int len) {
    decoder_status_t status;
    if (len <= 0) {
        return status;
    }

    // Uses up to `len` items. It always uses at least one, so the scheduler
    // never sees "consumed nothing". It stops right after a frame completes.
    status.consumed = static_cast<int>(
        d_extractor->process(static_cast<const uint8_t*>(in), static_cast<size_t>(len)));

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
        // An empty vector's data() may be null, so give a zero-length PDU a valid pointer
        static const uint8_t kNoBytes = 0;
        metadata::add_pdu(status.data, pdu.empty() ? &kNoBytes : pdu.data(), pdu.size());
        status.decode_success = true;
    }

    incr_nitems_read(static_cast<size_t>(status.consumed));
    return status;
}

} /* namespace uniclogs */
} /* namespace gr */
