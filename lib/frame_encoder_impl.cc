#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "frame_encoder_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace uniclogs {

frame_encoder::sptr frame_encoder::make(encoder::encoder_sptr enc)
{
    return gnuradio::get_initial_sptr(new frame_encoder_impl(enc));
}


/*
 * The private constructor
 */
frame_encoder_impl::frame_encoder_impl(encoder::encoder_sptr enc)
    : gr::block("frame_encoder",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_encoder(enc)
{
    message_port_register_in(pmt::mp("pdu"));
    message_port_register_out(pmt::mp("pdu"));

    set_msg_handler(pmt::mp("pdu"), [this](pmt::pmt_t msg) { this->encode(msg); });
}

/*
 * Our virtual destructor.
 */
frame_encoder_impl::~frame_encoder_impl() {}

void frame_encoder_impl::encode(pmt::pmt_t pdu)
{
    message_port_pub(pmt::mp("pdu"), d_encoder->encode(pdu));
}

} /* namespace uniclogs */
} /* namespace gr */
