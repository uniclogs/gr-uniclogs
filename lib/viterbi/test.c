#include <pmt/pmt.h>
#include <gnuradio/pdu.h>
#include "viterbi.h"

 int main(void){
    uint8_t encoded_phr[30] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,0,0, 0,0,0,0, 1,1,0,0, 0,0 };
    int phr_encoded_len = 30;

    // cast to pmt for compat with viterbi lib
    pmt::pmt_t pdu_vector = gr::pdu::make_pdu_vector(gr::types::vector_type::byte_t, encoded_phr, phr_encoded_len);
    std::vector<uint8_t> msg = pmt::u8vector_elements(pmt::cdr(pdu_vector));
    std::string bits;
    for (auto b : msg) {
        bits.push_back(b ? '1' : '0');
    }

    // decode phr
    ViterbiCodec codec(4, {7, 15});
    std::string outbits = codec.Decode(bits);
    std::vector<uint8_t> out;
    for (auto b : outbits) {
        out.push_back(b == '1');
    }
    uint32_t b = 0x0;
    uint8_t* bytes = new uint8_t[2];
    for (size_t i = 0; i < 2; i++) {
        uint8_t b = 0x0;
        b = out[i * 8 + 7] << 0;
        b |= out[i * 8 + 6] << 1;
        b |= out[i * 8 + 5] << 2;
        b |= out[i * 8 + 4] << 3;
        b |= out[i * 8 + 3] << 4;
        b |= out[i * 8 + 2] << 5;
        b |= out[i * 8 + 1] << 6;
        b |= out[i * 8 + 0] << 7;
        bytes[i] = b;
    }

    b = (bytes[0] << 8 | bytes[1]);
    printf("%x \r", b);
 }
