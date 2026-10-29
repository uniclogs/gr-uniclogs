#include <cstdint>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <gnuradio/pdu.h>
#include <gnuradio/uniclogs/ieee802_15_4_decode_n_check.h>
#include <gnuradio/uniclogs/metadata.h>
#include <pmt/pmt.h>


namespace gr {
namespace uniclogs {
ieee802_15_4_decode_n_check::sptr
ieee802_15_4_decode_n_check::make(const std::vector<uint8_t>& preamble,
                                  size_t preamble_threshold,
                                  const std::vector<uint8_t>& sync,
                                  size_t sync_threshold,
                                  const std::vector<uint8_t>& known_msg,
                                  size_t max_len)
{
    return ieee802_15_4_decode_n_check::sptr(new ieee802_15_4_decode_n_check(
        preamble, preamble_threshold, sync, sync_threshold, known_msg, max_len));
}

ieee802_15_4_decode_n_check::ieee802_15_4_decode_n_check(
    const std::vector<uint8_t>& preamble,
    size_t preamble_threshold,
    const std::vector<uint8_t>& sync,
    size_t sync_threshold,
    const std::vector<uint8_t>& known_msg,
    size_t max_len)
    : decoder("IEEE-802.15.4", "1.1", sizeof(uint8_t), max_len),
      d_preamble(preamble.size() * 8),
      d_preamble_shift_reg(preamble.size() * 8),
      d_preamble_len(preamble.size() * 8),
      d_preamble_thrsh(preamble_threshold),
      d_sync(sync.size() * 8),
      d_sync_shift_reg(sync.size() * 8),
      d_sync_len(sync.size() * 8),
      d_sync_thrsh(sync_threshold),
      d_len(max_len),
      d_length_field_len(0),
      d_state(SEARCHING),
      d_cnt(0),
      d_frame_start_idx(0),
      d_known_msg(known_msg),
      d_expected_codeword(known_msg.size() * 8),
      d_expected_codeword_bytes(known_msg.size() * 2)
{
    bool m1 = 0;
    bool m2 = 0;
    bool m3 = 0;
    bool m = 0;

    for (size_t i = 0; i < (known_msg.size() * 8); i++) {
        m1 = m2;
        m2 = m3;
        m3 = m;
        m = known_msg[i];
        d_expected_codeword <<= ~(m ^ m2 ^ m3);
        d_expected_codeword <<= ~(m ^ m1 ^ m2 ^ m3);
    }

    for (size_t i = 0; i < (known_msg.size() * 2); i++) {
        uint8_t b = 0x0;
        b  = d_expected_codeword[i * 8 + 0] << 7;
        b |= d_expected_codeword[i * 8 + 1] << 6;
        b |= d_expected_codeword[i * 8 + 2] << 5;
        b |= d_expected_codeword[i * 8 + 3] << 4;
        b |= d_expected_codeword[i * 8 + 4] << 3;
        b |= d_expected_codeword[i * 8 + 5] << 2;
        b |= d_expected_codeword[i * 8 + 6] << 1;
        b |= d_expected_codeword[i * 8 + 7];
        d_expected_codeword_bytes[i] = b;
    }

    for (uint8_t b : preamble) {
        d_preamble <<= (b >> 7);
        d_preamble <<= ((b >> 6) & 0x1);
        d_preamble <<= ((b >> 5) & 0x1);
        d_preamble <<= ((b >> 4) & 0x1);
        d_preamble <<= ((b >> 3) & 0x1);
        d_preamble <<= ((b >> 2) & 0x1);
        d_preamble <<= ((b >> 1) & 0x1);
        d_preamble <<= (b & 0x1);
    }
    for (uint8_t b : sync) {
        d_sync <<= (b >> 7);
        d_sync <<= ((b >> 6) & 0x1);
        d_sync <<= ((b >> 5) & 0x1);
        d_sync <<= ((b >> 4) & 0x1);
        d_sync <<= ((b >> 3) & 0x1);
        d_sync <<= ((b >> 2) & 0x1);
        d_sync <<= ((b >> 1) & 0x1);
        d_sync <<= (b & 0x1);
    }
    /* Parameters checking */
    if (max_len == 0) {
        throw std::invalid_argument("The maximum frame size should be at least 1 byte");
    }

    if (d_sync_len < 8) {
        throw std::invalid_argument("SYNC word should be at least 8 bits");
    }

    if (d_preamble_len && d_preamble_len < 2 * d_preamble_thrsh) {
        throw std::invalid_argument("Too many error bits are allowed for the preamble."
                                    "Consider lowering the threshold");
    }

    if (d_sync_len < 2 * d_sync_thrsh) {
        throw std::invalid_argument("Too many error bits are allowed for the sync word. "
                                    "Consider lowering the threshold");
    }

    d_pdu = new uint8_t[d_len + d_length_field_len];

    reset();
}


ieee802_15_4_decode_n_check::~ieee802_15_4_decode_n_check() { delete[] d_pdu; }

void ieee802_15_4_decode_n_check::reset()
{
    d_cnt = 0;
    /* There are case that no preamble is used */
    if (d_preamble_len) {
        d_state = SEARCHING;
    } else {
        d_state = SEARCHING_SYNC;
    }
    d_preamble_shift_reg.reset();
    d_sync_shift_reg.reset();
}

decoder_status_t ieee802_15_4_decode_n_check::decode(const void* in, int len)
{
    decoder_status_t status;
    switch (d_state) {
    case SEARCHING:
        status.consumed = search_preamble((const uint8_t*)in, len);
        break;
    case SEARCHING_SYNC:
        status.consumed = search_sync((const uint8_t*)in, len);
        break;
    case DECODING_PAYLOAD:
        decode_payload(status, (const uint8_t*)in, len);
        break;
    default:
        throw std::runtime_error("ieee802_15_4_decode_n_check: Invalid state");
    }
    incr_nitems_read(static_cast<uint64_t>(status.consumed));
    return status;
}

/**
 * To greatly simplify the logic, the decoder requests that the number of
 * input items should be a multiple of 8
 * @return 8
 */
size_t ieee802_15_4_decode_n_check::input_multiple() const { return 8; }

int ieee802_15_4_decode_n_check::search_preamble(const uint8_t* in, int len)
{
    for (int i = 0; i < len; i++) {
        d_preamble_shift_reg <<= in[i];
        shift_reg tmp = d_preamble_shift_reg ^ d_preamble;
        if (tmp.count() <= d_preamble_thrsh) {
            d_state = SEARCHING_SYNC;
            d_frame_start_idx = nitems_read() + i + 1;
            d_cnt = 0;
            return i + 1;
        }
    }
    return len;
}

int ieee802_15_4_decode_n_check::search_sync(const uint8_t* in, int len)
{
    for (int i = 0; i < len; i++) {
        d_sync_shift_reg <<= in[i];
        shift_reg tmp = d_sync_shift_reg ^ d_sync;
        d_cnt++;
        if (tmp.count() <= d_sync_thrsh) {
            d_state = DECODING_PAYLOAD;
            d_cnt = 0;
            return i + 1;
        }

        /* The sync word should be available by now */
        if (d_cnt > d_preamble_len * 2 + d_sync_len + d_sync_thrsh) {
            reset();
            return i + 1;
        }
    }
    return len;
}


void ieee802_15_4_decode_n_check::decode_payload(decoder_status_t& status,
                                                 const uint8_t* in,
                                                 int len)
{
    const int s = len / 8;
    size_t flen = d_len;
    for (int i = 0; i < s; i++) {
        uint8_t b = 0x0;
        b  = in[i * 8 + 0] << 7;
        b |= in[i * 8 + 1] << 6;
        b |= in[i * 8 + 2] << 5;
        b |= in[i * 8 + 3] << 4;
        b |= in[i * 8 + 4] << 3;
        b |= in[i * 8 + 5] << 2;
        b |= in[i * 8 + 6] << 1;
        b |= in[i * 8 + 7];
        d_pdu[d_cnt++] = b;
        status.consumed = (i + 1) * 8;

        if (d_cnt == flen + d_length_field_len) {
            uint8_t error_bits[d_known_msg.size() * 2];
            for (size_t i = 0; i < d_known_msg.size() * 2; i++){
              error_bits[i] = d_expected_codeword_bytes[i] ^ d_pdu[i];
            }
            metadata::add_decoder(status.data, this);
            metadata::add_time_iso8601(status.data);
            metadata::add_sample_start(status.data, d_frame_start_idx);
            metadata::add_sample_cnt(status.data,
                                     nitems_read() + (i + 1) * 8 - d_frame_start_idx);

            status.decode_success = true;
            metadata::add_pdu(status.data, d_pdu + d_length_field_len, flen);
            metadata::add_pdu(status.data, error_bits, flen);

            reset();
            return;
        }
    }
    status.consumed = s * 8;
}


} /* namespace uniclogs */
} /* namespace gr */
