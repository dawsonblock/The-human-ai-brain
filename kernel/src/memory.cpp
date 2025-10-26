#include "brain/memory.hpp"
#include <cmath>
#include <algorithm>

namespace hab {

// ShortTermMemory Implementation

ShortTermMemory::ShortTermMemory(const STMConfig& config)
    : config_(config) {}

void ShortTermMemory::add(const Episode& episode) {
    episodes_.push_back(episode);
    
    // Keep only recent episodes
    while (episodes_.size() > static_cast<size_t>(config_.capacity)) {
        episodes_.pop_front();
    }
}

Eigen::VectorXd ShortTermMemory::recall(const Eigen::VectorXd& query, Scalar beta) const {
    if (episodes_.empty()) {
        return Eigen::VectorXd::Zero(query.size());
    }
    
    auto now = std::chrono::steady_clock::now();
    
    // Compute weighted sum with temporal decay and similarity
    Eigen::VectorXd result = Eigen::VectorXd::Zero(query.size());
    Scalar total_weight = 0.0;
    
    for (const auto& episode : episodes_) {
        // Time decay
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<Scalar>>(
            now - episode.timestamp);
        Scalar time_weight = std::exp(-elapsed.count() / config_.decay_tau);
        
        // Cosine similarity
        Scalar dot = query.dot(episode.gw_state);
        Scalar norm_q = query.norm();
        Scalar norm_e = episode.gw_state.norm();
        
        Scalar similarity = 0.0;
        if (norm_q > 1e-10 && norm_e > 1e-10) {
            similarity = dot / (norm_q * norm_e);
        }
        
        // Softmax attention
        Scalar score = std::exp(beta * similarity);
        Scalar weight = time_weight * score;
        
        result += weight * episode.gw_state;
        total_weight += weight;
    }
    
    if (total_weight > 1e-10) {
        result /= total_weight;
    }
    
    return result;
}

Scalar ShortTermMemory::get_importance(const Episode& episode) const {
    // Importance = |reward| + novelty
    Scalar importance = std::abs(episode.reward);
    
    // Novelty based on distance to recent memories
    if (!episodes_.empty()) {
        Scalar min_dist = std::numeric_limits<Scalar>::max();
        
        for (const auto& e : episodes_) {
            Scalar dist = (episode.gw_state - e.gw_state).norm();
            min_dist = std::min(min_dist, dist);
        }
        
        importance += min_dist;
    }
    
    return importance;
}

// LongTermMemory Implementation

LongTermMemory::LongTermMemory(const LTMConfig& config)
    : config_(config) {}

void LongTermMemory::consolidate(const Episode& episode, Scalar importance) {
    if (importance < config_.importance_threshold) {
        return;
    }
    
    memories_.push_back(episode);
    importance_scores_.push_back(importance);
    
    // Evict least important if over capacity
    while (memories_.size() > static_cast<size_t>(config_.max_memories)) {
        auto min_it = std::min_element(importance_scores_.begin(), importance_scores_.end());
        size_t min_idx = std::distance(importance_scores_.begin(), min_it);
        
        memories_.erase(memories_.begin() + min_idx);
        importance_scores_.erase(importance_scores_.begin() + min_idx);
    }
}

std::vector<Episode> LongTermMemory::retrieve(const Eigen::VectorXd& query, int top_k) const {
    if (memories_.empty()) {
        return {};
    }
    
    // Compute similarities
    std::vector<std::pair<Scalar, size_t>> similarities;
    similarities.reserve(memories_.size());
    
    for (size_t i = 0; i < memories_.size(); ++i) {
        Scalar sim = similarity(query, memories_[i].gw_state);
        similarities.push_back({sim, i});
    }
    
    // Sort descending
    std::partial_sort(similarities.begin(),
                     similarities.begin() + std::min(top_k, static_cast<int>(similarities.size())),
                     similarities.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top-k
    std::vector<Episode> result;
    for (int i = 0; i < std::min(top_k, static_cast<int>(similarities.size())); ++i) {
        result.push_back(memories_[similarities[i].second]);
    }
    
    return result;
}

Scalar LongTermMemory::similarity(const Eigen::VectorXd& a, const Eigen::VectorXd& b) const {
    Scalar dot = a.dot(b);
    Scalar norm_a = a.norm();
    Scalar norm_b = b.norm();
    
    if (norm_a < 1e-10 || norm_b < 1e-10) {
        return 0.0;
    }
    
    return dot / (norm_a * norm_b);
}

// Memory System Implementation

Memory::Memory(const STMConfig& stm_config, const LTMConfig& ltm_config)
    : stm_(stm_config), ltm_(ltm_config) {}

void Memory::store(const Episode& episode) {
    // Add to STM
    stm_.add(episode);
    
    // Check if important enough for LTM consolidation
    Scalar importance = stm_.get_importance(episode);
    ltm_.consolidate(episode, importance);
}

Eigen::VectorXd Memory::recall(const Eigen::VectorXd& query) const {
    // Combine STM and LTM recall
    Eigen::VectorXd stm_recall = stm_.recall(query);
    
    auto ltm_episodes = ltm_.retrieve(query, 3);
    
    Eigen::VectorXd ltm_recall = Eigen::VectorXd::Zero(query.size());
    if (!ltm_episodes.empty()) {
        for (const auto& ep : ltm_episodes) {
            ltm_recall += ep.gw_state;
        }
        ltm_recall /= ltm_episodes.size();
    }
    
    // Weighted combination (favor STM)
    return 0.7 * stm_recall + 0.3 * ltm_recall;
}

} // namespace hab
