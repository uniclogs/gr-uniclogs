#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/uniclogs/ieee802_15_4_encoder.h>

namespace gr {
namespace uniclogs {
ieee802_15_4_encoder::sptr
ieee802_15_4_encoder::make(uint8_t preamble,
                           size_t preamble_len,
                           size_t length_field_bytes,
                           const std::vector<uint8_t>& sync_word)
{
    return ieee802_15_4_encoder::sptr(
        new ieee802_15_4_encoder(preamble, preamble_len, length_field_bytes, sync_word));
}

ieee802_15_4_encoder::sptr
ieee802_15_4_encoder::make(uint8_t preamble,
                           size_t preamble_len,
                           const std::vector<uint8_t>& sync_word,
                           size_t frame_len)
{
    return ieee802_15_4_encoder::sptr(
        new ieee802_15_4_encoder(preamble, preamble_len, sync_word, frame_len));
}

ieee802_15_4_encoder::ieee802_15_4_encoder(uint8_t preamble,
                                           size_t preamble_len,
                                           const std::vector<uint8_t>& sync_word,
                                           size_t frame_len)
    : d_max_frame_len(2047),
      d_preamle_len(preamble_len),
      d_var_len(0),
      d_length_field_bytes(0)
{

    if (!sync_word.size()) {
        throw std::invalid_argument(
            "ieee802_15_4_encoder: SYNC word should be at least one byte");
    }

    d_buffer = new uint8_t[preamble_len + sync_word.size() + d_max_frame_len];
    std::fill_n(d_buffer, preamble_len, preamble);
    std::copy(sync_word.begin(), sync_word.end(), d_buffer + preamble_len);
    d_payload = d_buffer + preamble_len + sync_word.size();
}

ieee802_15_4_encoder::ieee802_15_4_encoder(uint8_t preamble,
                                           size_t preamble_len,
                                           size_t length_field_bytes,
                                           const std::vector<uint8_t>& sync_word)
    : d_max_frame_len(((size_t)1 << (length_field_bytes * 8)) - 1),
      d_preamle_len(preamble_len),
      d_var_len(1),
      d_length_field_bytes(length_field_bytes)
{
    if (!sync_word.size()) {
        throw std::invalid_argument(
            "ieee802_15_4_encoder: SYNC word should be at least one byte");
    }
    d_buffer = new uint8_t[preamble_len + sync_word.size() + d_max_frame_len +
                           d_length_field_bytes];
    std::fill_n(d_buffer, preamble_len, preamble);
    std::copy(sync_word.begin(), sync_word.end(), d_buffer + preamble_len);
    d_payload = d_buffer + preamble_len + sync_word.size();
}

ieee802_15_4_encoder::~ieee802_15_4_encoder() { delete[] d_buffer; }

pmt::pmt_t ieee802_15_4_encoder::encode(pmt::pmt_t msg)
{
    pmt::pmt_t b;
    if (pmt::is_blob(msg)) {
        b = msg;
    } else if (pmt::is_pair(msg)) {
        b = pmt::cdr(msg);
    } else {
        throw std::runtime_error(
            "ieee802_15_4_encoder: received a malformed pdu message");
    }

    size_t pdu_len(0);
    const uint8_t* pdu = (const uint8_t*)pmt::uniform_vector_elements(b, pdu_len);
    if (pdu_len > d_max_frame_len) {
        throw std::runtime_error("ieee802_15_4_encoder: PDU received has a size larger "
                                 "than the maximum allowed");
    }

    uint8_t* payload = d_payload;
    if (d_var_len) {


        uint8_t* out = new uint8_t[d_length_field_bytes]();
        size_t tmp_pdu_len = pdu_len;

        for (size_t i = 0; i < d_length_field_bytes; i++) {
            out[d_length_field_bytes - 1 - i] = (uint8_t)(tmp_pdu_len & 0xFF);
            tmp_pdu_len >>= 8;
        }

        for (size_t i = 0; i < d_length_field_bytes; i++)
            *payload++ = out[i];
    }

    payload = std::copy(pdu, pdu + pdu_len, payload);

    /* Make a pmt compatible with the pdu to tagged stream block */
    pmt::pmt_t vecpmt(pmt::make_blob(d_buffer, (size_t)(payload - d_buffer)));
    pmt::pmt_t res(pmt::cons(pmt::PMT_NIL, vecpmt));
    return res;
}

} /* namespace uniclogs */
} /* namespace gr */
