#ifndef INCLUDED_UNICLOGS_FRAME_ENCODER_H
#define INCLUDED_UNICLOGS_FRAME_ENCODER_H

#include <gnuradio/block.h>
#include <gnuradio/uniclogs/api.h>
#include <gnuradio/uniclogs/encoder.h>

namespace gr {
namespace uniclogs {

/*!
 * \brief <+description of block+>
 * \ingroup uniclogs
 *
 */
class UNICLOGS_API frame_encoder : virtual public gr::block
{
public:
    typedef std::shared_ptr<frame_encoder> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of uniclogs::frame_encoder.
     *
     * To avoid accidental use of raw pointers, uniclogs::frame_encoder's
     * constructor is in a private implementation
     * class. uniclogs::frame_encoder::make is the public interface for
     * creating new instances.
     */
    static sptr make(encoder::encoder_sptr enc);
};

} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_FRAME_ENCODER_H */
