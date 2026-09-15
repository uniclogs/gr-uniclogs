#include <gnuradio/io_signature.h>
#include <gnuradio/uniclogs/encoder.h>

namespace gr {
namespace uniclogs {

int encoder::base_unique_id = 1;

encoder::encoder() : d_id(base_unique_id++) {}

encoder::~encoder() {}

int encoder::unique_id() const { return d_id; }
} /* namespace uniclogs */
} /* namespace gr */
