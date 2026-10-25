// Code repurposed from the initial C++ decoder. It find a preamble then
// a sync word, then collects coded bits as a message.
//
//  kSearchingPreamble --(last bits match preamble)---> kSearchingSync
//  kSearchingSync     --(last bits match sync word)--> kCollectingFrame
//  kCollectingFrame   --(frame decoded elsewhere)----> kSearchingPreamble

#ifndef FRAME_EXTRACTOR_H_
#define FRAME_EXTRACTOR_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "viterbi_frame_decoder.h"

namespace uniclogs_fec {

// Most recent N bits received, oldest first
class BitWindow {
    
    public:
        explicit BitWindow(size_t size = 0);
        void push(uint8_t bit);                                         // new bit in, old bit out
        void clear();                                                   // reset to all 0
        size_t distance_to(const std::vector<uint8_t>& pattern) const;  // Hamming distance

    private:
        std::vector<uint8_t> bits_;
        size_t oldest_;                 // index of oldest bit in bits_
};


class FrameExtractor {

    public:
        FrameExtractor(const std::vector<uint8_t>& preamble,     /*   Bytes of preamble pattern that must be matched, most significant bit first                                */
                            size_t preamble_treshold,            /*   How many bits of preamble may differ without a match being discarded                                      */
                            const std::vector<uint8_t>& sync,    /*   Bytes of sync pattern that must be matched, most significant bit first                                    */
                            size_t sync_threshold,               /*   How many bits of sync may differ without a match being discarded                                          */
                            bool invert_payload,                 /*   Whether or not to fliip every bit AFTER the sync word. This does not affect preamble or sync comparison   */
			    bool length_lsb_first = true);       /*   Bit order of the length segment of the header                                                             */

        // Get rid of all current information and return to searching for preamble 
        void reset();               
        
        // Feed up to len stream items, returning how many were consumed
        size_t process(const uint8_t* in, size_t len);
            
        // Decoded message bits, mvoes data out and clears frame_ready()
        std::vector<uint8_t> take_message();

        bool frame_ready() const { return frame_ready_; }
        uint64_t frame_start_index() const { return ready_start_; }
        uint64_t frame_end_index() const { return ready_end_; }

    private: 
        enum class State { kSearchingPreamble, kSearchingSync, kCollectingFrame };

        void restart_search();
        void on_preamble_bit(uint8_t bit);
        void on_sync_bit(uint8_t bit);
        void on_frame_bit(uint8_t bit);


        // ---- configuration ----
        std::vector<uint8_t> preamble_;      // pattern, one bit per element
        std::vector<uint8_t> sync_;
        size_t preamble_threshold_;
        size_t sync_threshold_;
        bool invert_payload_;
        FrameDecoder decoder_;

        // ---- search state ----
        State state_;
        BitWindow preamble_window_;
        BitWindow sync_window_;
        size_t sync_bits_seen_;              // bits examined since the preamble
        uint64_t items_seen_;                // all items fed so far
        uint64_t frame_start_;               // working copy
        uint64_t payload_start_;             // items_seen_ when the sync word ended

        // ---- frame collection ----
        std::vector<uint8_t> coded_;         // polarity-corrected coded bits so far
        size_t decode_at_;                   // try decoding once this many are collected
 
        // ---- finished frame, kept until taken or the next process() call ----
        bool frame_ready_;
        std::vector<uint8_t> message_;
        uint64_t ready_start_;
        uint64_t ready_end_;
};

} // namespace uniclogs_fec

#endif
