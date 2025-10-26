// episodic_memory.h
//
// FDQC v4.0: Episodic Memory - Long-Term Vector Storage
//
// Implements long-term episodic memory with:
// - Vector similarity search (k-NN retrieval)
// - Importance-based consolidation
// - Exponential decay of salience over time
// - Memory capacity management with pruning
//
// Corresponds to hippocampal episodic memory system.
// Stores experiences with global workspace embeddings.

#ifndef EPISODIC_MEMORY_H
#define EPISODIC_MEMORY_H

#include "fdqc_params.h"
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <cmath>
#include <mutex>

namespace episodic_memory {

// Episodic memory entry
struct Episode {
    std::vector<double> embedding;      // Global workspace encoding
    std::string metadata;               // Textual description
    double importance;                  // Consolidation importance
    double affective_valence;           // Emotional valence at encoding
    std::chrono::steady_clock::time_point timestamp;
    size_t retrieval_count;             // Number of times retrieved
    
    Episode() : importance(0.0), affective_valence(0.0), retrieval_count(0) {
        timestamp = std::chrono::steady_clock::now();
    }
    
    Episode(const std::vector<double>& emb, const std::string& meta,
            double imp, double valence)
        : embedding(emb), metadata(meta), importance(imp), 
          affective_valence(valence), retrieval_count(0) {
        timestamp = std::chrono::steady_clock::now();
    }
};

// Retrieval result with similarity score
struct RetrievalResult {
    Episode episode;
    double similarity;
    size_t index;
    
    RetrievalResult() : similarity(0.0), index(0) {}
    
    RetrievalResult(const Episode& ep, double sim, size_t idx)
        : episode(ep), similarity(sim), index(idx) {}
    
    // For sorting by similarity (descending)
    bool operator<(const RetrievalResult& other) const {
        return similarity > other.similarity;  // Descending order
    }
};

// Memory statistics
struct MemoryStats {
    size_t total_episodes;
    size_t total_retrievals;
    double average_importance;
    size_t pruning_events;
    
    MemoryStats() 
        : total_episodes(0), total_retrievals(0),
          average_importance(0.0), pruning_events(0) {}
};

class EpisodicMemory {
public:
    // Constructor
    EpisodicMemory();
    
    // Record episode with full metadata
    void record(const std::vector<double>& embedding,
               const std::string& metadata,
               double importance,
               double affective_valence = 0.0);
    
    // Simplified record (backward compatibility)
    void record(const std::vector<float>& embedding, const std::string& metadata);
    
    // Retrieve k most similar episodes
    std::vector<RetrievalResult> retrieve(const std::vector<double>& query_embedding,
                                         size_t k = fdqc_params::KNN_NEIGHBORS) const;
    
    // Simplified retrieve (backward compatibility)
    std::vector<std::pair<std::vector<float>, std::string>> retrieve(
        const std::vector<float>& embedding, size_t k) const;
    
    // Retrieve episodes above importance threshold
    std::vector<Episode> get_important_episodes(double threshold) const;
    
    // Retrieve recent episodes (last N)
    std::vector<Episode> get_recent_episodes(size_t n) const;
    
    // Apply memory decay (importance fades over time)
    void apply_decay();
    
    // Prune least important memories if over capacity
    void prune_if_needed();
    
    // Get memory size
    size_t size() const { return episodes_.size(); }
    
    // Check if empty
    bool empty() const { return episodes_.empty(); }
    
    // Get statistics
    const MemoryStats& stats() const { return stats_; }
    
    // Clear all memories
    void clear();

private:
    std::vector<Episode> episodes_;
    MemoryStats stats_;
    mutable std::mutex mutex_;  // Thread safety
    
    // Helper: compute cosine similarity
    static double cosine_similarity(const std::vector<double>& a,
                                   const std::vector<double>& b);
    
    // Helper: convert float vector to double vector
    static std::vector<double> float_to_double(const std::vector<float>& vec);
    
    // Helper: convert double vector to float vector
    static std::vector<float> double_to_float(const std::vector<double>& vec);
    
    // Helper: update running statistics
    void update_stats(double importance);
    
    // Helper: mark episode as retrieved
    void mark_retrieved(size_t index) const;
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline EpisodicMemory::EpisodicMemory() {
    episodes_.reserve(fdqc_params::MAX_EPISODIC_MEMORIES);
}

inline void EpisodicMemory::clear() {
    episodes_.clear();
    stats_ = MemoryStats();
}

inline std::vector<double> EpisodicMemory::float_to_double(
    const std::vector<float>& vec) {
    return std::vector<double>(vec.begin(), vec.end());
}

inline std::vector<float> EpisodicMemory::double_to_float(
    const std::vector<double>& vec) {
    return std::vector<float>(vec.begin(), vec.end());
}

inline double EpisodicMemory::cosine_similarity(
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

inline void EpisodicMemory::mark_retrieved(size_t index) const {
    if (index < episodes_.size()) {
        // Const cast to allow retrieval count update
        const_cast<Episode&>(episodes_[index]).retrieval_count++;
    }
}

} // namespace episodic_memory

#endif // EPISODIC_MEMORY_H