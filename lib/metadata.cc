#include <gnuradio/uniclogs/date.h>
#include <gnuradio/uniclogs/metadata.h>
#include <chrono>
#include <stdexcept>

namespace gr {
namespace uniclogs {

metadata::~metadata() {}
std::string metadata::value(const key_t& k)
{
    switch (k) {
    case PDU:
        return "pdu";
    case DECODER_CRC_VALID:
        return "decoder_crc_valid";
    case CENTER_FREQ:
        return "center_freq";
    case FREQ_OFFSET:
        return "freq_offset";
    case DECODER_CORRECTED_BITS:
        return "decoder_corrected_bits";
    case TIME:
        return "time";
    case SAMPLE_START:
        return "sample_start";
    case SAMPLE_CNT:
        return "sample_cnt";
    case DECODER_SYMBOL_ERASURES:
        return "decoder_symbol_erasures";
    case SNR:
        return "snr";
    case DECODER_NAME:
        return "decoder_name";
    case DECODER_VERSION:
        return "decoder_version";
    case ANTENNA_AZIMUTH:
        return "antenna_azimuth";
    case ANTENNA_ELEVATION:
        return "antenna_elevation";
    case ANTENNA_POLARIZATION:
        return "antenna_polarization";
    case DECODER_PHASE_DELAY:
        return "decoder_phase";
    case DECODER_RESAMPLING_RATIO:
        return "decoder_resampling_ratio";
    case SYMBOL_TIMING_ERROR:
        return "symbol_timing_error";
    default:
        throw std::invalid_argument("metadata: invalid key");
    }
}

std::string metadata::time_iso8601()
{
    /* check for the current UTC time */
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();

    return date::format("%FT%TZ", date::floor<std::chrono::microseconds>(tp));
}
void metadata::add_decoder(pmt::pmt_t& m, const decoder* dec)
{
    if (!dec) {
        return;
    }

    auto meta = pmt::car(m);
    meta = pmt::dict_add(meta, pmt::mp(value(DECODER_NAME)), pmt::mp(dec->name()));
    meta = pmt::dict_add(meta, pmt::mp(value(DECODER_VERSION)), pmt::mp(dec->version()));
    pmt::set_car(m, meta);
}

void metadata::add_time_iso8601(pmt::pmt_t& m)
{
    auto meta = pmt::car(m);
    meta = pmt::dict_add(meta, pmt::mp(value(TIME)), pmt::mp(time_iso8601()));
    pmt::set_car(m, meta);
}

void metadata::add_pdu(pmt::pmt_t& m, const uint8_t* in, size_t len)
{
    auto pdu = pmt::cdr(m);
    pdu = pmt::make_blob(in, len);
    pmt::set_cdr(m, pdu);
}

void metadata::add_sample_start(pmt::pmt_t& m, uint64_t idx)
{
    auto meta = pmt::car(m);
    meta = pmt::dict_add(meta, pmt::mp(value(SAMPLE_START)), pmt::from_uint64(idx));
    pmt::set_car(m, meta);
}

void metadata::add_sample_cnt(pmt::pmt_t& m, uint64_t cnt)
{
    auto meta = pmt::car(m);
    meta = pmt::dict_add(meta, pmt::mp(value(SAMPLE_CNT)), pmt::from_uint64(cnt));
    pmt::set_car(m, meta);
}

void metadata::add_symbol_timing_error(pmt::pmt_t& m, double error)
{
    auto meta = pmt::car(m);
    meta =
        pmt::dict_add(meta, pmt::mp(value(SYMBOL_TIMING_ERROR)), pmt::from_double(error));
    pmt::set_car(m, meta);
}
} // namespace uniclogs

} // namespace gr
