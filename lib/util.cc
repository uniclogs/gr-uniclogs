#include <stdio.h>
#include <cstddef>
#include <cstdint>
#include <vector>

// repack a uint8_t buffer
std::vector<uint8_t> pack_bits(uint8_t* buf, size_t buf_len, bool repack, bool lsb_first)
{
    uint8_t input_bits;
    uint8_t out_bits;
    size_t buf_size;
    if (repack) {
        buf_size = buf_len / 8;
        input_bits = 1;
        out_bits = 8;
    } else {
        buf_size = buf_len * 8;
        input_bits = 8;
        out_bits = 1;
    }

    auto repacked_buf = std::vector<uint8_t>(buf_size, 0);
    size_t n_written = 0;
    size_t n_read = 0;
    size_t out_index = 0;
    size_t in_index = 0;

    if (lsb_first)
        while (n_written < repacked_buf.size() && n_read < buf_len) {
            repacked_buf[n_written] |= ((buf[n_read] >> in_index) & 0x01) << out_index;

            in_index = (in_index + 1) % input_bits;
            out_index = (out_index + 1) % out_bits;

            if (in_index == 0) {
                n_read++;
            }
            if (out_index == 0) {
                n_written++;
            }
        }
    else {
        while (n_written < repacked_buf.size() && n_read < buf_len) {
            repacked_buf[n_written] |=
                ((buf[n_read] >> (input_bits - 1 - in_index)) & 0x01)
                << (out_bits - 1 - out_index);

            in_index = (in_index + 1) % input_bits;
            out_index = (out_index + 1) % out_bits;
            if (in_index == 0) {
                n_read++;
            }
            if (out_index == 0) {
                n_written++;
            }
        }
    }
    return repacked_buf;
}
