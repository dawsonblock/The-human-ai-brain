// preconscious_buffer.h
//
// FDQC v4.0: Preconscious Buffer - 2-Second Sensory Window
//
// Implements iconic/echoic memory: a 2-second rolling buffer of sensory
// inputs before conscious access. Provides:
// - Circular buffer with timestamp tracking
// - Similarity-based retrieval (avoid redundant encoding)
// - Importance-gated consolidation to episodic memory
// - Automatic time-based expiration
//
// Corresponds to sensory memory in Atkinson-Shiffrin model (1968).
// Duration matches iconic/echoic memory (~2 seconds).

#ifndef PRECONSCIOUS_BUFFER_H
#define PRECONSCIOUS_BUFFER_H

#include "fdqc_params.h"
#include <deque>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <cmath>
#include <mutex>

namespace preconscious_buffer {

// Buffer entry with metadata
struct BufferEntry {
    std::vector<double> embedding;  // Vector representation
    std::string content;            // Raw content (optional)
    std::chrono::steady_clock::time_point timestamp;
    double salience;                // Importance score
    double similarity_to_previous;  // Similarity to last entry
    
    BufferEntry() : salience(0.0), similarity_to_previous(0.0) {
        timestamp = std::chrono::steady_clock::now();
    }
    
    BufferEntry(const std::vector<double>& emb, const std::string& cont, double sal)
        : embedding(emb), content(cont), salience(sal), similarity_to_previous(0.0) {
        timestamp = std::chrono::steady_clock::now();
    }
};

// Buffer statistics
struct BufferStats {
    size_t total_entries_processed;
    size_t consolidations;          // Entries promoted to episodic memory
    double average_salience;
    size_t duplicates_filtered;     // High-similarity entries skipped
    
    BufferStats() 
        : total_entries_processed(0), consolidations(0),
          average_salience(0.0), duplicates_filtered(0) {}
};

class PreConsciousBuffer {
public:
    // Constructor
    PreConsciousBuffer();
    
    // Add entry with vector embedding
    void push(const std::vector<double>& embedding, 
             const std::string& content = "",
             double salience = 0.5);
    
    // Simplified push (backward compatibility)
    void push(const std::string& entry);
    
    // Get all current entries (non-expired)
    std::vector<BufferEntry> contents() const;
    
    // Get entries above salience threshold
    std::vector<BufferEntry> get_salient_entries(double threshold) const;
    
    // Find similar entries (cosine similarity)
    std::vector<size_t> find_similar(const std::vector<double>& query_embedding,
                                     double threshold = fdqc_params::BUFFER_SIMILARITY_THRESHOLD) const;
    
    // Check if similar entry exists (deduplication)
    bool contains_similar(const std::vector<double>& embedding,
                         double threshold = fdqc_params::BUFFER_SIMILARITY_THRESHOLD) const;
    
    // Get most recent N entries
    std::vector<BufferEntry> get_recent(size_t n) const;
    
    // Clear expired entries (older than 2 seconds)
    void expire_old_entries();
    
    // Clear all entries
    void clear();
    
    // Get buffer size
    size_t size() const { return buffer_.size(); }
    
    // Check if buffer is empty
    bool empty() const { return buffer_.empty(); }
    
    // Get statistics
    const BufferStats& stats() const { return stats_; }
    
    // Mark entry as consolidated to episodic memory
    void mark_consolidated(size_t index);

private:
    std::deque<BufferEntry> buffer_;
    BufferStats stats_;
    mutable std::mutex mutex_;  // Thread safety
    
    // Helper: compute cosine similarity
    static double cosine_similarity(const std::vector<double>& a,
                                   const std::vector<double>& b);
    
    // Helper: check if entry is expired
    bool is_expired(const BufferEntry& entry) const;
    
    // Helper: compute embedding from string (simplified)
    std::vector<double> string_to_embedding(const std::string& str) const;
    
    // Helper: update running statistics
    void update_stats(double salience, bool is_duplicate);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline PreConsciousBuffer::PreConsciousBuffer() {
    // Note: std::deque doesn't have reserve(), but that's okay
    // It manages memory automatically
}

inline void PreConsciousBuffer::clear() {
    buffer_.clear();
}

inline std::vector<BufferEntry> PreConsciousBuffer::contents() const {
    // Return non-expired entries
    std::vector<BufferEntry> result;
    result.reserve(buffer_.size());
    
    for (const auto& entry : buffer_) {
        if (!is_expired(entry)) {
            result.push_back(entry);
        }
    }
    
    return result;
}

inline bool PreConsciousBuffer::is_expired(const BufferEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - entry.timestamp).count();
    
    // Expire after BUFFER_DURATION_SEC seconds
    return age > (fdqc_params::BUFFER_DURATION_SEC * 1000);
}

inline double PreConsciousBuffer::cosine_similarity(
    const std::vector<double>& a,
    const std::vector<double>& b) {
    
    if (a.size() != b.size() || a.empty()) {
        return 0.0;
    }
    
    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a < fdqc_params::EPSILON || norm_b < fdqc_params::EPSILON) {
        return 0.0;
    }
    
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

inline void PreConsciousBuffer::mark_consolidated(size_t index) {
    if (index < buffer_.size()) {
        stats_.consolidations++;
    }
}

} // namespace preconscious_buffer

#endif // PRECONSCIOUS_BUFFER_H