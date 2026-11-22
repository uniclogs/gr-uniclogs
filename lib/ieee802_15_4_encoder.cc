#include <cstdint>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/uniclogs/ieee802_15_4_encoder.h>

namespace gr {
namespace uniclogs {
ieee802_15_4_encoder::sptr
ieee802_15_4_encoder::make(uint8_t preamble,
                           size_t d_preamble_len,
                           const std::vector<uint8_t>& sync_word,
                           size_t frame_len)
{
    return ieee802_15_4_encoder::sptr(
        new ieee802_15_4_encoder(preamble, d_preamble_len, sync_word, frame_len));
}

ieee802_15_4_encoder::ieee802_15_4_encoder(uint8_t preamble,
                                           size_t d_preamble_len,
                                           const std::vector<uint8_t>& sync_word,
                                           size_t frame_len)
    : d_max_frame_len(frame_len),
      d_preamble_len(d_preamble_len),
      d_preamble(preamble),
      d_length_field_bytes(0)
{

    if (!sync_word.size()) {
        throw std::invalid_argument(
            "ieee802_15_4_encoder: SYNC word should be at least one byte");
    }
    if (d_max_frame_len > 2047*8) {
        throw std::invalid_argument(
            "ieee802_15_4_encoder: Frame len should not be bigger than 2047 bits");
    }

    d_sync_word = {sync_word[0], sync_word[1]};
    d_buffer.reserve(d_max_frame_len);
   // = new uint8_t[d_preamble_len + d_sync_word.size() + 2];
}


ieee802_15_4_encoder::~ieee802_15_4_encoder() { }

pmt::pmt_t ieee802_15_4_encoder::encode(pmt::pmt_t msg)
{
    if (pmt::is_pair(msg)) {
        msg = pmt::cdr(msg);
    } else {
        throw std::runtime_error(
            "ieee802_15_4_encoder: received a malformed pdu message");
    }

    size_t pdu_len(0);
    const uint8_t* pdu = (const uint8_t*)pmt::uniform_vector_elements(msg, pdu_len);

    if (pdu_len > d_max_frame_len) {
        throw std::runtime_error("ieee802_15_4_encoder: PDU received has a size larger "
                                 "than the maximum allowed");
    }

    // fill the buffer with the preamble
    for (size_t i = 0; i < d_preamble_len; i++)
      d_buffer.push_back(d_preamble);
    // add in sync word into the buffer after the preamble
    d_buffer.push_back(d_sync_word[0]);
    d_buffer.push_back(d_sync_word[1]);
    // TODO: Once we move to a nonstatic PHR, i.e. when we have FEC functional
    // need to add the flags for FEC and eventually data whitening into the PHR
    uint16_t payload_len = 0x07FF & pdu_len;
    d_buffer.push_back((uint8_t)(0xFF & (payload_len >> 8)));
    d_buffer.push_back((uint8_t)(0xFF  & payload_len));

    // insert in the payload
    d_buffer.insert(d_buffer.end(), pdu, pdu+pdu_len);
    uint8_t* buf = d_buffer.data();

    /* Make a pmt compatible with the pdu to tagged stream block */
    pmt::pmt_t vecpmt(pmt::make_blob(buf, (size_t)(d_buffer.size())));
    pmt::pmt_t res(pmt::cons(pmt::PMT_NIL, vecpmt));
    d_buffer.clear();
    return res;
}

} /* namespace uniclogs */
} /* namespace gr */
