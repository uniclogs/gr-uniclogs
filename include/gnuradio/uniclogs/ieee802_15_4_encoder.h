#ifndef INCLUDED_UNICLOGS_IEEE802_15_4_ENCODER_H
#define INCLUDED_UNICLOGS_IEEE802_15_4_ENCODER_H

#include <gnuradio/uniclogs/api.h>
#include <gnuradio/uniclogs/encoder.h>
#include <gnuradio/uniclogs/shift_reg.h>
#include <cstddef>
#include <cstdint>

namespace gr {
namespace uniclogs {

/*!
 * \brief An IEEE802.15.4 frame encoder with some extended parameterization
 */
class UNICLOGS_API ieee802_15_4_encoder : public encoder
{
public:
    using sptr = std::shared_ptr<ieee802_15_4_encoder>;
    static sptr make(uint8_t preamble,
                     size_t preamble_len,
                     bool FEC_toggle,
                     const std::vector<uint8_t>& sync_word,
                     size_t frame_len);

    ieee802_15_4_encoder(uint8_t preamble,
                         size_t preamble_len,
                         bool FEC_toggle,
                         const std::vector<uint8_t>& sync_word,
                         size_t frame_len);

    ~ieee802_15_4_encoder();

    pmt::pmt_t encode(pmt::pmt_t msg);

private:
    const uint8_t tail_bits = 3;   // ieee 802.15.4 adds 3 tail bits for a terminated code
    const uint8_t coding_rate = 2; // 1/2 rate convolutional coder
    const uint32_t frame_buffer = 2048; // AT86RF215 RX frame decoder
    const uint8_t phr_len = 2;          // length of the PHR in bytes
    const int len_field_bits = 11;
    const int phr_len_bits = 16;
    const size_t d_max_frame_len;
    const size_t d_preamble_len;
    const uint8_t d_preamble;
    std::vector<uint8_t> d_sync_word;
    uint8_t* d_payload;
    bool d_FEC_toggle;
    uint8_t* d_buffer_frame_len;
    uint8_t d_length_field_bytes;
};

} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_IEEE802_15_4_ENCODER_H */
