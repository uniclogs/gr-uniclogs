#include "frame_extractor.h"
 
#include <stdexcept>
#include <utility>
 
namespace uniclogs_fec {
 
// Turn pattern bytes into one bit per element, most significant bit first
static std::vector<uint8_t> expand_bytes_msb_first(const std::vector<uint8_t>& bytes)
{
    std::vector<uint8_t> bits;
    bits.reserve(bytes.size() * bitsPerByte);
    for (uint8_t byte : bytes) {
        for (int shift = bitsPerByte - 1; shift >= 0; shift--) {
            bits.push_back((byte >> shift) & 1);
        }
    }
    return bits;
}

// ====================BitWindow====================
 
BitWindow::BitWindow(size_t size) : bits_(size, 0), oldest_(0) {}
 
void BitWindow::push(uint8_t bit) {
    // Circular buffer - slot holding oldest is given newest,
    // and oldest marker increments
    bits_[oldest_] = bit;
    oldest_ = (oldest_ + 1) % bits_.size();
}
 
void BitWindow::clear() {
    bits_.assign(bits_.size(), 0);
    oldest_ = 0;
}
 
size_t BitWindow::distance_to(const std::vector<uint8_t>& pattern) const {
    size_t differences = 0;
    for (size_t k = 0; k < pattern.size(); k++) {
        const uint8_t bit = bits_[(oldest_ + k) % bits_.size()];
        differences += (bit != pattern[k]);
    }
    return differences;
}
 
// ====================FrameExtractor====================
 
FrameExtractor::FrameExtractor(const std::vector<uint8_t>& preamble,
                               size_t preamble_threshold,
                               const std::vector<uint8_t>& sync,
                               size_t sync_threshold,
                               bool invert_payload,
			       bool length_lsb_first)
    : preamble_(expand_bytes_msb_first(preamble)),
      sync_(expand_bytes_msb_first(sync)),
      preamble_threshold_(preamble_threshold),
      sync_threshold_(sync_threshold),
      invert_payload_(invert_payload),
      decoder_(length_lsb_first),
      state_(State::kSearchingPreamble),
      preamble_window_(preamble_.size()),
      sync_window_(sync_.size()),
      sync_bits_seen_(0),
      items_seen_(0),
      frame_start_(0),
      payload_start_(0),
      decode_at_(kHeaderCodedBits),
      frame_ready_(false),
      ready_start_(0),
      ready_end_(0) {
    
        // Sanity checks
    if (sync_.size() < 8) {
        throw std::invalid_argument("SYNC word should be at least 8 bits");
    }
    if (!preamble_.empty() && preamble_.size() < 2 * preamble_threshold_) {
        throw std::invalid_argument("Too many error bits are allowed for the preamble. "
                                    "Consider lowering the threshold");
    }
    if (sync_.size() < 2 * sync_threshold_) {
        throw std::invalid_argument("Too many error bits are allowed for the sync word. "
                                    "Consider lowering the threshold");
    }
    restart_search();
}
 
void FrameExtractor::reset() {
    frame_ready_ = false;
    message_.clear();
    restart_search();
}
 
// Back to the start of the state machine. Does not touch items_seen_ or the
// finished-frame fields, so a completed frame survives until it is taken
void FrameExtractor::restart_search() {
    // If no preamble is used
    if (preamble_.empty()) {
        state_ = State::kSearchingSync;   
        frame_start_ = items_seen_;
    } else {
        state_ = State::kSearchingPreamble;
    }
    if (!preamble_.empty()) {
        preamble_window_.clear();
    }
    sync_window_.clear();
    sync_bits_seen_ = 0;
    coded_.clear();
    decode_at_ = kHeaderCodedBits;
}
 
size_t FrameExtractor::process(const uint8_t* in, size_t len) {
    frame_ready_ = false;  
 
    size_t used = 0;
    while (used < len && !frame_ready_) {
        const uint8_t bit = (in[used] != 0) ? 1 : 0;
        used++;
        items_seen_++;
 
        switch (state_) {
        case State::kSearchingPreamble: on_preamble_bit(bit); break;
        case State::kSearchingSync:     on_sync_bit(bit);     break;
        case State::kCollectingFrame:   on_frame_bit(bit);    break;
        }
    }
    return used;
}
 
std::vector<uint8_t> FrameExtractor::take_message() {
    frame_ready_ = false;
    return std::move(message_);
}
 
void FrameExtractor::on_preamble_bit(uint8_t bit) {
    preamble_window_.push(bit);
    if (preamble_window_.distance_to(preamble_) <= preamble_threshold_) {
        state_ = State::kSearchingSync;
        frame_start_ = items_seen_;     // first item after the preamble
        sync_bits_seen_ = 0;
    }
}
 
void FrameExtractor::on_sync_bit(uint8_t bit) {
    sync_window_.push(bit);
    sync_bits_seen_++;
 
    if (sync_window_.distance_to(sync_) <= sync_threshold_) {
        state_ = State::kCollectingFrame;
        payload_start_ = items_seen_;   // first item after the sync word
        coded_.clear();
        decode_at_ = kHeaderCodedBits;
        return;
    }
 
    // If sync word has not been seen after 2 * preamble length + sync length + threshold, give up
    if (sync_bits_seen_ > preamble_.size() * 2 + sync_.size() + sync_threshold_) {
        restart_search();
    }
}
 
void FrameExtractor::on_frame_bit(uint8_t bit) {
    // Correct the polarity, then store.
    coded_.push_back(invert_payload_ ? (bit ^ 1) : bit);
 
    // Wait until the header is available to decode,
    // and after that until the whole frame has arrived
    if (coded_.size() < decode_at_) {
        return;
    }
 
    DecodeResult result = decoder_.decode(coded_);
    switch (result.status) {
    case DecodeStatus::kOk:
        message_ = std::move(result.message);
        ready_start_ = frame_start_;
        ready_end_ = payload_start_ + result.coded_bits_needed;
        frame_ready_ = true;
        restart_search();
        break;
    case DecodeStatus::kBadHeader:
        restart_search();              
        break;
    case DecodeStatus::kNeedMoreBits:
        // Header is known: wait for the full frame
        decode_at_ = (result.coded_bits_needed > coded_.size()) ? result.coded_bits_needed
                                                                : coded_.size() + 1;
        break;
    }
}
 
} // namespace uniclogs_fec
