// This is a port of a Python implementation of a k=4 Viterbi decoder, meant 
// to be called by a program capable of parsing messages from a bitstream.
//
// Input: vector of coded bits
// Output: vector of decoded bits and status

#ifndef VITERBI_FRAME_DECODER_H_
#define VITERBI_FRAME_DECODER_H_

#include <array>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace uniclogs_fec {

constexpr int kRegisterSize = 4;                                        // k = 4
constexpr int kNumStates = 1 << (kRegisterSize - 1);                    // 8 states tracked at a time
constexpr int kHeaderBits = 16;                                         // 16 bit header
constexpr int kLengthFieldBits = 11;                                    // 11 bits define the frame size
constexpr int kTailBits = kRegisterSize - 1;                            // 3 tail bits
constexpr int kHeaderLookaheadSteps = kHeaderBits * kRegisterSize;      // 64 extra bits used to enforce a more valid decoding of header
constexpr int kHeaderSteps = kHeaderBits + kHeaderLookaheadSteps;       // 80 total bits parsed for the header
constexpr size_t kHeaderCodedBits = 2 * kHeaderSteps;                   // 160 total coded bits for the header, 2 bits per step
constexpr int kMaxMessageBytes = 240;                                   // 240 is the packet size limit
constexpr int bitsPerByte = 8;                                          // 8 bits in a byte                                                                   // 240 is maximum packet size

enum class DecodeStatus {
    kOk,            // The entire frame has been decoded
    kNeedMoreBits,  // Wait for more coded bits before calling again
    kBadHeader      // Header does not pass sanity checks, do not treat as a frame
};

struct DecodeResult {
    DecodeStatus status = DecodeStatus::kNeedMoreBits;
    size_t coded_bits_needed = 0;                           // Total coded bits in the whole frame, updated from 0 once header parsed 
    std::vector<uint8_t> message;                           // Decoded binary message with header and tail removed
};

// Coded bits in a frame with message of given number of bytes
size_t coded_bits_for_message(int message_bytes);

// Pack bits into bytes. An incomplete last byte is 0-padded
std::vector<uint8_t> pack_bits(const std::vector<uint8_t>& bits, bool lsb_first);

class FrameDecoder {
    
    public:
        explicit FrameDecoder(bool length_lsb_first = true);
        // Starts at the first bit after the sync word, assumes polarity has already been corrected
        DecodeResult decode(const std::vector<uint8_t>& coded) const;
        // Parses the header. The message_bytes field will be the length of the message in bytes only if the status is kOk
        DecodeStatus parse_header(const std::vector<uint8_t>& coded, int* message_bytes) const;

    private:
        // Trellis structure for Viterbi decoding
        // This is a combination of metrics and traceback_table from Python implementation
        struct Trellis {
            // The field decisions[t][s] holds which of two possible predecessors of some 
            // state s survived at some timestep t (aka the bit m3 of the predecessor)
            std::vector<std::array<uint8_t, kNumStates>> decisions;
            // The branch metric sum of the best path into each state after the last timestep
            std::array<int, kNumStates> metrics;
        };

        Trellis run_trellis(const std::vector<uint8_t>& coded, size_t steps) const;
        std::vector<uint8_t> traceback(const Trellis& trellis, int end_state) const;
        static int best_state(const std::array<int, kNumStates>& metrics);
        // expected_output_[state][input_bit] is the 2-bit pair result that would be encoded for a given input bit
        std::array<std::array<uint8_t, 2>, kNumStates> expected_output_;
	bool length_lsb_first_;
};

} // namespace uniclogs_fec

#endif
