// This is a port of a Python implementation of a k=4 Viterbi decoder, meant 
// to be called by a program capable of parsing messages from a bitstream.
//
// Input: vector of coded bits
// Output: vector of decoded bits and status

#include "viterbi_frame_decoder.h"

#include <algorithm>
#include <limits>

namespace uniclogs_fec {

// Marks a state that no path has been able to reach
constexpr int kUnreachable = std::numeric_limits<int>::max();

// How many of two coded bits differ from the expected values (Hamming distance for u1u0 pair)
constexpr int kPopcount2[4] = {0, 1, 1, 2};

size_t coded_bits_for_message(int message_bytes) {
    const int steps = kHeaderBits + message_bytes * bitsPerByte + kTailBits;
    return 2 * static_cast<size_t>(steps);
}

std::vector<uint8_t> pack_bits(const std::vector<uint8_t>& bits, bool lsb_first) {
    std::vector<uint8_t> bytes((bits.size() + 7) / bitsPerByte, 0);
    for (size_t i = 0; i < bits.size(); i++) {
        if (bits[i] == 0) {
            continue;
        }
        const size_t position_in_byte = i % bitsPerByte;
        const size_t shift = lsb_first ? position_in_byte : bitsPerByte - 1 - position_in_byte;
        bytes[i / bitsPerByte] |= static_cast<uint8_t>(1u << shift);
    }
    return bytes;
}



// Combination of lookup_table and _out table from Python version
FrameDecoder::FrameDecoder(bool length_lsb_first) : length_lsb_first_(length_lsb_first) {
    for (int state = 0; state < kNumStates; state++) {
        const int m1 = (state >> 2) & 1;
        const int m2 = (state >> 1) & 1;
        const int m3 = state & 1;
        for (int m0 = 0; m0 < 2; m0++) {
            const int u0 = m0 ^ m2 ^ m3;
            const int u1 = m0 ^ m1 ^ m2 ^ m3;
            expected_output_[state][m0] = static_cast<uint8_t>((u0 << 1) | u1);
        }
    }
}

// Returns the first state with the smallest metric, ties go to the lower state
int FrameDecoder::best_state(const std::array<int, kNumStates>& metrics) {
    return static_cast<int>(std::min_element(metrics.begin(), metrics.end()) - metrics.begin());
}

// The bulk of the decoding work happens here
FrameDecoder::Trellis FrameDecoder::run_trellis(const std::vector<uint8_t>& coded, size_t steps) const {
    Trellis trellis;
    trellis.metrics.fill(kUnreachable);
    trellis.metrics[0] = 0;                 // initialise encoder to state 0
    trellis.decisions.reserve(steps);

    for (size_t t = 0; t < steps; t++) {
        // Received pair for this step, packed in the same manner as expected_output_
        const int received = (coded[2 * t] << 1) | coded[2 * t + 1];
        std::array<int, kNumStates> new_metrics;
        new_metrics.fill(kUnreachable);
        std::array<uint8_t, kNumStates> row{};

        for (int state = 0; state < kNumStates; state++) {
            // Newest input bit placed in top bit of state
            const int input_bit = (state >> 2) & 1;

            // Two possible predecessors, only differing in lowest bit m3
            for (int m3 = 0; m3 < 2; m3++) {
                const int previous_state = ((state & 3) << 1) | m3;
                // Check if predecessor is even possible
                if (trellis.metrics[previous_state] == kUnreachable) {
                    continue;
                }
                // Generate a candidate, keep it if it is better than previous candidate
                const int hamming_distance = kPopcount2[expected_output_[previous_state][input_bit] ^ received];
                const int candidate = trellis.metrics[previous_state] + hamming_distance;
                if (candidate < new_metrics[state]) {
                    new_metrics[state] = candidate;
                    row[state] = static_cast<uint8_t>(m3);
                }
            }
        }
        // Store resulting information from this timestep
        trellis.metrics = new_metrics;
        trellis.decisions.push_back(row);
    }
    return trellis;
}

// Traceback, fetches ordered input bits based on recorded trellis 
std::vector<uint8_t> FrameDecoder::traceback(const Trellis& trellis, int end_state) const {
    const size_t steps = trellis.decisions.size();
    std::vector<uint8_t> bits(steps, 0);
    int state = end_state;
    // Trace back from last step to first
    for (size_t t = steps; t --> 0;) {
        // Input bit that led into current state is top bit
        bits[t] = static_cast<uint8_t>(state >> 2);
        // Move to surviving predecessor, lower two bits become upper 
        // two bits and recorded decision becomes new lower bit
        state = ((state & 3) << 1) | trellis.decisions[t][state];
    }
    return bits;
}

// Header parsing logic
DecodeStatus FrameDecoder::parse_header(const std::vector<uint8_t>& coded, int* message_bytes) const {
    if (coded.size() < kHeaderCodedBits) {
        return DecodeStatus::kNeedMoreBits;
    }

    // Run solver over the header size plus lookahead, but ditch non-header results
    const Trellis trellis = run_trellis(coded, kHeaderSteps);
    const std::vector<uint8_t> bits = traceback(trellis, best_state(trellis.metrics));

    // FIXME this should reflect the actual expected values!
    // Currently it just checks that all 5 initial header bits are 0
    for (int i = 0; i < kHeaderBits - kLengthFieldBits; i++) {
        if (bits[i] != 0) {
            return DecodeStatus::kBadHeader;
        }
    }

    // Retrieve the length of the packet, in the configured bit order
    int length = 0;
    for (int i = 0; i < kLengthFieldBits; i++) {
        const int bit = bits[kHeaderBits - kLengthFieldBits + i];
        if (length_lsb_first_) {
            length |= bit << i;                 // first bit received is the least significant
        } else {
            length = (length << 1) | bit;       // first bit received is the most significant
        }
    }

    // If program has reached this point, the header is valid
    *message_bytes = length;
    return DecodeStatus::kOk;
}

// Logic for decoding a message based on header information
DecodeResult FrameDecoder::decode(const std::vector<uint8_t>& coded) const {
    DecodeResult result;

    // Get header information and status
    int message_bytes = 0;
    const DecodeStatus header_status = parse_header(coded, &message_bytes);

    // If status is not Ok, return as such
    if (header_status != DecodeStatus::kOk) {
        result.status = header_status;
        return result;
    }
    
    // Total coded bits in the whole frame, now that the length is known
    result.coded_bits_needed = coded_bits_for_message(message_bytes);

    // If more bits are needed, return as such
    if (coded.size() < result.coded_bits_needed) {
        result.status = DecodeStatus::kNeedMoreBits;
        return result;
    }

    // If decoding has the green light, do it
    const size_t steps = result.coded_bits_needed / 2;
    // Find paths
    const Trellis trellis = run_trellis(coded, steps);
    // Retrieve decoded information from best path
    const std::vector<uint8_t> bits = traceback(trellis, 0);
    // Drop non-message bits
    result.message.assign(bits.begin() + kHeaderBits, bits.end() - kTailBits);
    result.status = DecodeStatus::kOk;
    return result;
}


} // namespace uniclogs_fec
