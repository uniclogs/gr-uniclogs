#ifndef INCLUDE_UNICLOGS_METADATA_H_
#define INCLUDE_UNICLOGS_METADATA_H_

#include <gnuradio/uniclogs/api.h>
#include <gnuradio/uniclogs/decoder.h>
#include <pmt/pmt.h>
#include <cstdint>
#include <string>

namespace gr {

namespace uniclogs {
class UNICLOGS_API metadata
{
public:
    typedef enum key {
        PDU = 0,
        DECODER_CRC_VALID,
        CENTER_FREQ,
        DECODER_PHASE_DELAY,
        DECODER_RESAMPLING_RATIO,
        CRC_VALID,
        FREQ_OFFSET,
        DECODER_CORRECTED_BITS,
        TIME,
        SAMPLE_START,
        SAMPLE_CNT,
        DECODER_SYMBOL_ERASURES,
        SNR,
        DECODER_NAME,
        DECODER_VERSION,
        ANTENNA_AZIMUTH,
        ANTENNA_ELEVATION,
        ANTENNA_POLARIZATION,
        SYMBOL_TIMING_ERROR,
        KEYS_NUM
    } key_t;

    static std::string value(const key_t& k);

    static std::string time_iso8601();

    static void add_pdu(pmt::pmt_t& m, const uint8_t* in, size_t len);

    static void add_decoder(pmt::pmt_t& m, const decoder* dec);

    static void add_time_iso8601(pmt::pmt_t& m);

    static void add_sample_start(pmt::pmt_t& m, uint64_t idx);

    static void add_sample_cnt(pmt::pmt_t& m, uint64_t cnt);

    static void add_symbol_timing_error(pmt::pmt_t& m, double error);

    virtual ~metadata();
};
} // namespace uniclogs

} // namespace gr

#endif /* INCLUDE_UNICLOGS_METADATA_H_ */
