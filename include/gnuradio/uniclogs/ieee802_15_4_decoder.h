#ifndef INCLUDED_UNICLOGS_IEEE802_15_4_DECODER_H
#define INCLUDED_UNICLOGS_IEEE802_15_4_DECODER_H

#include "shift_reg.h"
#include <gnuradio/uniclogs/api.h>
#include <gnuradio/uniclogs/decoder.h>
#include <cstdint>
#include <memory>
#include <vector>

// The real work is done by classes private to lib/. They are only declared
// here (not included), so this installed header does not depend on them.
namespace uniclogs_fec {
class FrameExtractor;
}

namespace gr {
namespace uniclogs {

/*!
 * \brief Decoder for a preamble + sync word framing scheme carrying a
 * rate-1/2, K=4 convolutionally coded frame.
 *
 * The decoder looks for the preamble, then the sync word, then collects the
 * coded bits that follow, decodes them with a Viterbi decoder and publishes
 * the message (header and tail removed) as a PDU.
 *
 * Frame layout after the sync word, as decoded bits: 5 zero bits, an 11-bit
 * length in bytes, 8*length message bits, 3 tail bits.
 */
class UNICLOGS_API ieee802_15_4_decoder : public gr::uniclogs::decoder
{
public:
    using sptr = std::shared_ptr<ieee802_15_4_decoder>;

    /**
     * @param preamble pattern bytes, sent most significant bit first. A repeated
     * word may be given as a shorter portion of the real preamble.
     *
     * @param preamble_threshold the maximum number of bits that may be wrong
     * in the preamble
     *
     * @param sync the synchronization word following the preamble
     *
     * @param sync_threshold the maximum number of bits that may be wrong in
     * the synchronization word
     *
     * @param invert_payload if true, every bit after the sync word is flipped
     * before decoding (IEEE 802.15.4 sends inverted bits). The preamble and
     * sync word are always compared with the stream as received.
     *
     * @param length_little_endian if true, the 11 length bits in the header are
     * sent least significant bit first. If false, most significant bit first.
     */
    static sptr make(const std::vector<uint8_t>& preamble,
                     size_t preamble_threshold,
                     const std::vector<uint8_t>& sync,
                     size_t sync_threshold,
                     bool FEC_toggle,
                     bool invert_payload,
                     bool length_little_endian);

    ieee802_15_4_decoder(const std::vector<uint8_t>& preamble,
                         size_t preamble_threshold,
                         const std::vector<uint8_t>& sync,
                         size_t sync_threshold,
                         bool FEC_toggle,
                         bool invert_payload,
                         bool length_little_endian);

    ~ieee802_15_4_decoder();

    decoder_status_t decode(const void* in, int len);

    void reset();

private:
    std::unique_ptr<uniclogs_fec::FrameExtractor> d_extractor;

    typedef enum {
        SEARCHING,                  //!< when searching for the start of the preamble
        SEARCHING_SYNC,             //!< We have preamble, search for sync
        DECODING_GENERIC_FRAME_LEN, //!< Decoding the frame length
        DECODING_PAYLOAD            //!< Decoding the payload
    } decoding_state_t;

    shift_reg d_preamble;
    shift_reg d_preamble_shift_reg;
    const size_t d_preamble_len;
    const size_t d_preamble_thrsh;
    shift_reg d_sync;
    shift_reg d_sync_shift_reg;
    const size_t d_sync_len;
    const size_t d_sync_thrsh;
    size_t d_len;
    size_t d_length_field_len;
    decoding_state_t d_state;
    size_t d_cnt;
    uint64_t d_frame_start_idx;
    uint8_t* d_pdu;
    size_t d_max_frame_len;

    bool d_FEC_toggle;
    int decode_frame_len(const uint8_t* in);
    int search_preamble(const uint8_t* in, int len);
    int search_sync(const uint8_t* in, int len);
    void decode_payload(decoder_status_t& status, const uint8_t* in, int len);
};

} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_IEEE802_15_4_DECODER_H */
