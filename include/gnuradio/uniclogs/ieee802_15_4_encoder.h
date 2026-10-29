#ifndef INCLUDED_UNICLOGS_IEEE802_15_4_ENCODER_H
#define INCLUDED_UNICLOGS_IEEE802_15_4_ENCODER_H

#include <gnuradio/uniclogs/api.h>
#include <gnuradio/uniclogs/encoder.h>
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
                     const std::vector<uint8_t>& sync_word,
                     size_t frame_len);

    ieee802_15_4_encoder(uint8_t preamble,
                         size_t preamble_len,
                         const std::vector<uint8_t>& sync_word,
                         size_t frame_len);
    ~ieee802_15_4_encoder();

    pmt::pmt_t encode(pmt::pmt_t msg);

private:
    const size_t d_max_frame_len;
    const size_t d_preamle_len;
    const bool d_var_len = 0;
    uint8_t* d_buffer;
    uint8_t* d_payload;
    uint8_t* d_buffer_frame_len;
    uint8_t d_length_field_bytes;
};

} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_IEEE802_15_4_ENCODER_H */
