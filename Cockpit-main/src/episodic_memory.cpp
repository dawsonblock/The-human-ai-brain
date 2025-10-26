// episodic_memory.cpp
//
// FDQC v4.0: Episodic Memory Implementation
//
// Implements long-term memory storage with vector similarity search,
// importance-based consolidation, and capacity management.

#include "episodic_memory.h"
#include <algorithm>
#include <numeric>

namespace episodic_memory {

//===========================================================================
// RECORDING
//===========================================================================

void EpisodicMemory::record(const std::vector<double>& embedding,
                            const std::string& metadata,
                            double importance,
                            double affective_valence) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create new episode
    Episode episode(embedding, metadata, importance, affective_valence);
    
    // Add to memory store
    episodes_.push_back(episode);
    
    // Update statistics
    update_stats(importance);
    
    // Prune if over capacity
    if (episodes_.size() > fdqc_params::MAX_EPISODIC_MEMORIES) {
        prune_if_needed();
    }
}

void EpisodicMemory::record(const std::vector<float>& embedding,
                            const std::string& metadata) {
    // Convert float to double and use default importance
    std::vector<double> double_embedding = float_to_double(embedding);
    record(double_embedding, metadata, 0.5, 0.0);
}

//===========================================================================
// RETRIEVAL (k-NN SEARCH)
//===========================================================================

std::vector<RetrievalResult> EpisodicMemory::retrieve(
    const std::vector<double>& query_embedding,
    size_t k) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (episodes_.empty()) {
        return {};
    }
    
    // Compute similarity for all episodes
    std::vector<RetrievalResult> all_results;
    all_results.reserve(episodes_.size());
    
    for (size_t i = 0; i < episodes_.size(); ++i) {
        double similarity = cosine_similarity(query_embedding, 
                                             episodes_[i].embedding);
        
        all_results.emplace_back(episodes_[i], similarity, i);
    }
    
    // Sort by similarity (descending)
    std::sort(all_results.begin(), all_results.end());
    
    // Take top k
    size_t result_count = std::min(k, all_results.size());
    std::vector<RetrievalResult> top_k(
        all_results.begin(),
        all_results.begin() + result_count
    );
    
    // Mark episodes as retrieved
    for (const auto& result : top_k) {
        mark_retrieved(result.index);
    }
    
    // Update statistics
    const_cast<MemoryStats&>(stats_).total_retrievals++;
    
    return top_k;
}

std::vector<std::pair<std::vector<float>, std::string>> EpisodicMemory::retrieve(
    const std::vector<float>& embedding,
    size_t k) const {
    
    // Convert to double and retrieve
    std::vector<double> double_embedding = float_to_double(embedding);
    auto results = retrieve(double_embedding, k);
    
    // Convert back to old format
    std::vector<std::pair<std::vector<float>, std::string>> output;
    output.reserve(results.size());
    
    for (const auto& result : results) {
        output.emplace_back(
            double_to_float(result.episode.embedding),
            result.episode.metadata
        );
    }
    
    return output;
}

//===========================================================================
// FILTERED RETRIEVAL
//===========================================================================

std::vector<Episode> EpisodicMemory::get_important_episodes(
    double threshold) const {
    
    std::vector<Episode> result;
    
    for (const auto& episode : episodes_) {
        if (episode.importance >= threshold) {
            result.push_back(episode);
        }
    }
    
    return result;
}

std::vector<Episode> EpisodicMemory::get_recent_episodes(size_t n) const {
    size_t start_idx = (episodes_.size() > n) ? episodes_.size() - n : 0;
    
    return std::vector<Episode>(
        episodes_.begin() + start_idx,
        episodes_.end()
    );
}

//===========================================================================
// MEMORY DECAY
//===========================================================================

void EpisodicMemory::apply_decay() {
    // Apply exponential decay to all episode importance
    for (auto& episode : episodes_) {
        episode.importance *= (1.0 - fdqc_params::MEMORY_DECAY_RATE);
    }
}

//===========================================================================
// MEMORY PRUNING
//===========================================================================

void EpisodicMemory::prune_if_needed() {
    if (episodes_.size() <= fdqc_params::MAX_EPISODIC_MEMORIES) {
        return;
    }
    
    // Compute pruning score: importance + retrieval_bonus - age_penalty
    std::vector<std::pair<double, size_t>> scores;
    scores.reserve(episodes_.size());
    
    auto now = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < episodes_.size(); ++i) {
        const auto& episode = episodes_[i];
        
        // Age in hours
        auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - episode.timestamp).count();
        double age_hours = age_ms / 3600000.0;
        
        // Score components
        double importance_score = episode.importance;
        double retrieval_bonus = episode.retrieval_count * 0.1;
        double age_penalty = std::log(1.0 + age_hours) * 0.1;
        
        double total_score = importance_score + retrieval_bonus - age_penalty;
        
        scores.emplace_back(total_score, i);
    }
    
    // Sort by score (ascending - lowest scores first)
    std::sort(scores.begin(), scores.end());
    
    // Remove bottom 20%
    size_t num_to_remove = episodes_.size() / 5;
    
    // Collect indices to remove
    std::vector<size_t> indices_to_remove;
    for (size_t i = 0; i < num_to_remove; ++i) {
        indices_to_remove.push_back(scores[i].second);
    }
    
    // Sort indices in descending order for safe removal
    std::sort(indices_to_remove.rbegin(), indices_to_remove.rend());
    
    // Remove episodes
    for (size_t idx : indices_to_remove) {
        episodes_.erase(episodes_.begin() + idx);
    }
    
    stats_.pruning_events++;
}

//===========================================================================
// STATISTICS UPDATE
//===========================================================================

void EpisodicMemory::update_stats(double importance) {
    stats_.total_episodes = episodes_.size();
    
    // Update average importance (exponential moving average)
    double alpha = 0.95;
    if (episodes_.size() == 1) {
        stats_.average_importance = importance;
    } else {
        stats_.average_importance = alpha * stats_.average_importance +
                                   (1.0 - alpha) * importance;
    }
}

} // namespace episodic_memory
