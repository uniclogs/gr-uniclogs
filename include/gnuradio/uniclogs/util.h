#include <stdint.h>
#include <cstddef>
#include <vector>

std::vector<uint8_t> pack_bits(uint8_t* buf, size_t buf_len, bool repack, bool lsb_first);
