#ifndef INCLUDED_UNICLOGS_FRAME_ENCODER_IMPL_H
#define INCLUDED_UNICLOGS_FRAME_ENCODER_IMPL_H

#include <gnuradio/uniclogs/frame_encoder.h>

namespace gr {
namespace uniclogs {

class frame_encoder_impl : public frame_encoder
{

public:
    frame_encoder_impl(encoder::encoder_sptr enc);
    ~frame_encoder_impl();

private:
    encoder::encoder_sptr d_encoder;

    void encode(pmt::pmt_t pdu);
};

} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_FRAME_ENCODER_IMPL_H */
