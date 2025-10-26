// chunking.h
//
// FDQC v4.0: Chunking System - 4→7±2 Capacity Expansion
//
// Implements pattern-based chunking to expand effective working memory
// capacity from base n=4 up to 7±2 (Miller 1956, Cowan 2001).
//
// Mechanism:
// - Learns frequently co-occurring patterns as "chunks"
// - Stores chunks in a library with embeddings
// - Replaces recognized patterns with single chunk tokens
// - Effective capacity: C = n_WM × (1 + γ·N_chunks/100), max 1.75×
//
// References:
// - Miller (1956): Magical number seven, plus or minus two
// - Cowan (2001): Working memory capacity of ~4 chunks
// - Gobet & Simon (1998): CHREST model of expertise via chunking

#ifndef CHUNKING_H
#define CHUNKING_H

#include "fdqc_params.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace chunking {

// Chunk representation
struct Chunk {
    std::vector<double> pattern;  // Pattern embedding (vector representation)
    std::vector<std::string> tokens;  // Original token sequence
    size_t frequency;             // Usage count
    double salience;              // Importance weight
    
    Chunk() : frequency(0), salience(0.0) {}
    
    Chunk(const std::vector<double>& pat, const std::vector<std::string>& toks)
        : pattern(pat), tokens(toks), frequency(1), salience(0.0) {}
};

// Chunker statistics
struct ChunkingStats {
    size_t total_chunks;         // Total chunks in library
    size_t sequences_processed;  // Total sequences encoded
    double average_compression;  // Average compression ratio
    double capacity_multiplier;  // Current capacity boost
    
    ChunkingStats() 
        : total_chunks(0), sequences_processed(0),
          average_compression(1.0), capacity_multiplier(1.0) {}
};

class Chunker {
public:
    // Constructor
    Chunker();
    
    // Encode sequence into chunked representation
    // Returns compressed sequence (some tokens may be chunk IDs)
    std::vector<std::string> encode(const std::vector<std::string>& tokens);
    
    // Decode chunked representation back to original tokens
    std::vector<std::string> decode(const std::vector<std::string>& codes) const;
    
    // Learn chunk from pattern (update library)
    void learn_chunk(const std::vector<double>& pattern_embedding,
                    const std::vector<std::string>& tokens);
    
    // Check if pattern matches existing chunk (similarity threshold)
    int find_matching_chunk(const std::vector<double>& pattern_embedding) const;
    
    // Get effective capacity multiplier (1.0 to 1.75)
    double effective_capacity_multiplier() const;
    
    // Calculate effective capacity for base dimensionality n
    double calculate_effective_capacity(int base_n) const;
    
    // Get statistics
    const ChunkingStats& stats() const { return stats_; }
    size_t chunk_count() const { return chunk_library_.size(); }
    
    // Clear chunk library (for retraining)
    void clear();

private:
    // Chunk library (ID → Chunk)
    std::unordered_map<int, Chunk> chunk_library_;
    
    // Next chunk ID
    int next_chunk_id_;
    
    // Statistics
    ChunkingStats stats_;
    
    // Helper: compute cosine similarity between two vectors
    static double cosine_similarity(const std::vector<double>& a,
                                   const std::vector<double>& b);
    
    // Helper: compute embedding for token sequence (simplified)
    std::vector<double> compute_embedding(const std::vector<std::string>& tokens) const;
    
    // Helper: update statistics
    void update_stats(size_t original_length, size_t compressed_length);
    
    // Helper: decay less-used chunks (LRU-style pruning)
    void prune_library_if_needed();
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline Chunker::Chunker() : next_chunk_id_(1000) {
    // Start chunk IDs at 1000 to distinguish from regular tokens
}

inline double Chunker::effective_capacity_multiplier() const {
    return stats_.capacity_multiplier;
}

inline double Chunker::calculate_effective_capacity(int base_n) const {
    // C_effective = n_WM × (1 + γ·N_chunks/100)
    // Capped at 1.75× base capacity (to match Miller's 7±2)
    double boost = 1.0 + fdqc_params::CHUNK_GAMMA * 
                   chunk_library_.size() / 100.0;
    double multiplier = std::min(boost, fdqc_params::CHUNK_BOOST_MAX);
    
    return base_n * multiplier;
}

inline void Chunker::clear() {
    chunk_library_.clear();
    next_chunk_id_ = 1000;
    stats_ = ChunkingStats();
}

inline double Chunker::cosine_similarity(const std::vector<double>& a,
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

} // namespace chunking

#endif // CHUNKING_H