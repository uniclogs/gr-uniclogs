
#ifndef INCLUDED_UNICLOGS_ENCODER_H
#define INCLUDED_UNICLOGS_ENCODER_H

#include <gnuradio/uniclogs/api.h>
#include <pmt/pmt.h>
#include <deque>

namespace gr {
namespace uniclogs {

/*!
 * \brief Abstract class defining the API of the SatNOGS Encoders
 *
 * Abstract class defining the API of the SatNOGS Encoders
 *
 * The gr-satnogs module tries to provide a unified encoding framework,
 * for various satellites and framing schemes.
 *
 * Specialization is performed by passing to the generic encoding block
 * (\ref frame_encoder() ) the appropriate encoder class that implements
 * this abstract class API.
 */
class UNICLOGS_API encoder
{
public:
    typedef std::shared_ptr<encoder> encoder_sptr;

    static int base_unique_id;

    int unique_id() const;


    encoder();

    virtual ~encoder();

    virtual pmt::pmt_t encode(pmt::pmt_t pdu) = 0;

private:
    const int d_id;
};

} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_ENCODER_H */
