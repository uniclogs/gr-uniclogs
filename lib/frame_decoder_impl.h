#ifndef INCLUDED_UNICLOGS_FRAME_DECODER_IMPL_H
#define INCLUDED_UNICLOGS_FRAME_DECODER_IMPL_H

#include <gnuradio/uniclogs/frame_decoder.h>
#include <volk/volk_alloc.hh>

namespace gr {
namespace uniclogs {

class frame_decoder_impl : public frame_decoder
{

public:
    frame_decoder_impl(decoder::decoder_sptr decoder_object, int input_size);
    ~frame_decoder_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);


private:
    decoder::decoder_sptr d_decoder;
    volk::vector<float> d_t_err;
    double d_t_err_acc;

    void reset(pmt::pmt_t m);
};
} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_FRAME_DECODER_IMPL_H */
