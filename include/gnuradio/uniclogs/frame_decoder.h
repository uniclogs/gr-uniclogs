#ifndef INCLUDED_UNICLOGS_FRAME_DECODER_H
#define INCLUDED_UNICLOGS_FRAME_DECODER_H

#include <gnuradio/sync_block.h>
#include <gnuradio/uniclogs/api.h>
#include <gnuradio/uniclogs/decoder.h>

namespace gr {
namespace uniclogs {

/*!
 * \brief This is a generic frame decoder block. It takes as input a
 * bit stream and produces decoded frames and their metadata.
 *
 * The decoding is performed by using a proper decoder object.
 * Each decoder implements the virtual class ::decoder()
 *
 * The frame and metadata are produced in a pmt dictionary, with the
 * keys "pdu" and "metadata".
 *
 * \ingroup satnogs
 *
 */
class UNICLOGS_API frame_decoder : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<frame_decoder> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of satnogs::frame_decoder.
     * @param decoder_object the decoder object to use
     */
    static sptr make(decoder::decoder_sptr decoder_object, int input_size);
};


} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_FRAME_DECODER_H */
