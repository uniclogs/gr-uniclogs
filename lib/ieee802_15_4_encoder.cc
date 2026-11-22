#include <cstdint>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/uniclogs/ieee802_15_4_encoder.h>
#include <gnuradio/uniclogs/util.h>
#include <stdio.h>

namespace gr {
namespace uniclogs {
ieee802_15_4_encoder::sptr
ieee802_15_4_encoder::make(uint8_t preamble,
                           size_t d_preamble_len,
                           bool FEC_toggle,
                           const std::vector<uint8_t>& sync_word,
                           size_t frame_len)
{
    return ieee802_15_4_encoder::sptr(new ieee802_15_4_encoder(
        preamble, d_preamble_len, FEC_toggle, sync_word, frame_len));
}

ieee802_15_4_encoder::ieee802_15_4_encoder(uint8_t preamble,
                                           size_t d_preamble_len,
                                           bool FEC_toggle,
                                           const std::vector<uint8_t>& sync_word,
                                           size_t frame_len)
    : d_max_frame_len(frame_len),
      d_preamble_len(d_preamble_len),
      d_preamble(preamble),
      d_FEC_toggle(FEC_toggle),
      d_length_field_bytes(0)
{

    if (!sync_word.size()) {
        throw std::invalid_argument(
            "ieee802_15_4_encoder: SYNC word should be at least one byte");
    }
    if (d_max_frame_len > (frame_buffer) * 8) {
        throw std::invalid_argument(
            "ieee802_15_4_encoder: Frame len should not be bigger than 2048 bits");
    }

    d_sync_word = { sync_word[0], sync_word[1] };
}


ieee802_15_4_encoder::~ieee802_15_4_encoder() {}

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

    std::vector<uint8_t> buffer;
    buffer.reserve(d_max_frame_len);

    for (size_t i = 0; i < d_preamble_len; i++)
        buffer.push_back(d_preamble);
    // add in sync word into the buffer after the preamble
    buffer.push_back(d_sync_word[0]);
    buffer.push_back(d_sync_word[1]);

    uint16_t phr = 0x07FF & pdu_len;
    if (d_FEC_toggle) {
        // uint16_t phr = (0x1000 & d_FCS_type)| (0x0800 & d_whitening_toggle) | (0x07FF &
        // pdu_len);

        std::vector<uint8_t> psdu;
        psdu.reserve(phr_len + pdu_len);

        psdu.push_back((uint8_t)(0xFF & (phr >> 8)));
        psdu.push_back((uint8_t)(0xFF & phr));

        psdu.insert(psdu.end(), pdu, pdu + pdu_len);

        auto unpacked_psdu = pack_bits(psdu.data(), psdu.size(), false, true);

        std::vector<uint8_t> coded_psdu;
        coded_psdu.reserve((unpacked_psdu.size() * coding_rate + tail_bits));

        bool m3, m2, m1, m = 0;
        for (size_t i = 0; i < unpacked_psdu.size(); i++) {
            m3 = m2;
            m2 = m1;
            m1 = m;
            m = unpacked_psdu[i];
            coded_psdu.push_back((uint8_t)(m ^ m1 ^ m2 ^ m3) ^ 1);
            coded_psdu.push_back((uint8_t)(m ^ m2 ^ m3) ^ 1);
        }

        // insert tail bits
        coded_psdu.insert(coded_psdu.end(), { 0x00, 0x00, 0x00 });

        auto packed_psdu = pack_bits(coded_psdu.data(), coded_psdu.size(), true, true);

        buffer.insert(buffer.end(), packed_psdu.begin(), packed_psdu.end());

    } else {

        // TODO: Once we move to a nonstatic PHR, i.e. when we have FEC functional
        // need to add the flags for FEC and eventually data whitening into the PHR
        buffer.push_back((uint8_t)(0xFF & (phr >> 8)));
        buffer.push_back((uint8_t)(0xFF & phr));

        // insert in the payload
        buffer.insert(buffer.end(), pdu, pdu + pdu_len);
    }

    uint8_t* buf = buffer.data();
    /* Make a pmt compatible with the pdu to tagged stream block */
    pmt::pmt_t vecpmt(pmt::make_blob(buf, (size_t)(buffer.size())));
    pmt::pmt_t res(pmt::cons(pmt::PMT_NIL, vecpmt));

    return res;
}

} /* namespace uniclogs */
} /* namespace gr */
