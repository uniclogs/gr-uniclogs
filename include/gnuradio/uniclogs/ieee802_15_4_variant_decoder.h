#ifndef INCLUDED_UNICLOGS_IEEE802_15_4_VARIANT_DECODER_H
#define INCLUDED_UNICLOGS_IEEE802_15_4_VARIANT_DECODER_H

#include <gnuradio/uniclogs/api.h>
#include <gnuradio/uniclogs/decoder.h>
#include <gnuradio/uniclogs/shift_reg.h>
#include "viterbi.h"

namespace gr {
namespace uniclogs {

/*!
 * \brief A IEEE 802.15.4 like decoder
 *
 * The IEEE 802.15.4 uses the well known preamble + sync word synchronization
 * scheme. Many popular on Cubesats ICs like the Texas Instruments CC1xxx family
 * or the AXxxxx of On Semiconductors follow this scheme. This decoder
 * class provides a generic way to decode signals following this framing
 * scheme.
 *
 */
class UNICLOGS_API ieee802_15_4_variant_decoder : public gr::uniclogs::decoder
{
public:
    /**
     *
     * @param preamble the preamble should be a repeated word. Note that due to AGC
     * settling, the receiver may not receive the whole preamble. If the preamble
     * is indeed a repeated pattern, a portion of it can be given as parameter.
     * The block should be able to deal with this. However, a quite small subset
     * may lead to a larger number of false alarms
     *
     * @param preamble_threshold the maximum number of bits that are
     * allowed to be wrong at the preamble
     *
     * @param sync the synchronization work following the preamble
     *
     * @param sync_threshold the maximum number of bits that are
     * allowed to be wrong at the synchronization word
     *
     * @param var_len if set to true, variable length decoding is used. Otherwise,
     * the \p max_len parameter indicates the fixed frame size
     *
     * @param rs if set, the decoder will perform RS(255,223) decoding prior the
     * descrambling and the CRC
     *
     * @return shared pointer of the decoder
     */
    using sptr = std::shared_ptr<ieee802_15_4_variant_decoder>;


    static sptr make(const std::vector<uint8_t>& preamble,
                     size_t preamble_threshold,
                     const std::vector<uint8_t>& sync,
                     size_t sync_threshold,
                     int constraint,
                     const std::vector<int>& polynomials,
                     bool fec);

    ieee802_15_4_variant_decoder(const std::vector<uint8_t>& preamble,
                                 size_t preamble_threshold,
                                 const std::vector<uint8_t>& sync,
                                 size_t sync_threshold,
                                 int constraint,
                                 const std::vector<int>& polynomials,
                                 bool fec);

    ~ieee802_15_4_variant_decoder();

    decoder_status_t decode(const void* in, int len);

    void reset();

    size_t input_multiple() const;

private:
    /**
     * Decoding FSM states
     */

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
    bool d_fec;
    uint64_t d_frame_start_idx;
    uint8_t* d_pdu;
    size_t d_max_frame_len;
    ViterbiCodec d_codec;

    int search_preamble(const uint8_t* in, int len);

    int search_sync(const uint8_t* in, int len);

    int decode_frame_len(const uint8_t* in);
    int decode_frame_len_fec(const uint8_t* in);

    void decode_payload(decoder_status_t& status, const uint8_t* in, int len);
};
} // namespace uniclogs
} // namespace gr

#endif /* INCLUDED_UNICLOGS_IEEE802_15_4_VARIANT_DECODER_H */
