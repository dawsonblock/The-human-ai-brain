#include "brain/tiered_memory.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <functional>

namespace hab {

// ============================================================================
// MinHash Deduplicator Implementation
// ============================================================================

MinHashDedup::MinHashDedup(int num_hashes) : num_hashes_(num_hashes) {
    // Initialize hash seeds for MinHash
    std::mt19937 rng(42); // Fixed seed for reproducibility
    hash_seeds_.reserve(num_hashes);
    for (int i = 0; i < num_hashes; ++i) {
        hash_seeds_.push_back(rng());
    }
}

std::array<uint64_t, 2> MinHashDedup::compute_signature(const std::string& text) const {
    // Simplified MinHash: use first 2 hash values as 128-bit signature
    std::array<uint64_t, 2> sig = {UINT64_MAX, UINT64_MAX};
    
    // Tokenize text into shingles (3-grams)
    std::vector<std::string> shingles;
    for (size_t i = 0; i + 3 <= text.size(); ++i) {
        shingles.push_back(text.substr(i, 3));
    }
    
    if (shingles.empty()) return sig;
    
    // Compute min hash for each hash function
    for (int h = 0; h < std::min(2, num_hashes_); ++h) {
        uint64_t min_hash = UINT64_MAX;
        std::hash<std::string> hasher;
        
        for (const auto& shingle : shingles) {
            uint64_t hash = hasher(shingle) ^ hash_seeds_[h];
            min_hash = std::min(min_hash, hash);
        }
        
        sig[h] = min_hash;
    }
    
    return sig;
}

std::array<uint64_t, 2> MinHashDedup::compute_signature(const Eigen::VectorXd& embedding) const {
    // For embeddings, use quantized values as features
    std::array<uint64_t, 2> sig = {UINT64_MAX, UINT64_MAX};
    
    // Quantize embedding to 8-bit values
    std::vector<uint8_t> quantized;
    quantized.reserve(embedding.size());
    
    Scalar min_val = embedding.minCoeff();
    Scalar max_val = embedding.maxCoeff();
    Scalar range = max_val - min_val;
    if (range < 1e-10) range = 1.0;
    
    for (int i = 0; i < embedding.size(); ++i) {
        uint8_t q = static_cast<uint8_t>(((embedding(i) - min_val) / range) * 255);
        quantized.push_back(q);
    }
    
    // Compute min hash
    std::hash<std::string> hasher;
    for (int h = 0; h < std::min(2, num_hashes_); ++h) {
        uint64_t min_hash = UINT64_MAX;
        
        for (size_t i = 0; i < quantized.size(); ++i) {
            std::string feature = std::to_string(i) + ":" + std::to_string(quantized[i]);
            uint64_t hash = hasher(feature) ^ hash_seeds_[h];
            min_hash = std::min(min_hash, hash);
        }
        
        sig[h] = min_hash;
    }
    
    return sig;
}

Scalar MinHashDedup::jaccard_similarity(
    const std::array<uint64_t, 2>& sig1,
    const std::array<uint64_t, 2>& sig2) const {
    
    // Count matching hash values
    int matches = 0;
    for (size_t i = 0; i < 2; ++i) {
        if (sig1[i] == sig2[i]) matches++;
    }
    
    return static_cast<Scalar>(matches) / 2.0;
}

bool MinHashDedup::is_duplicate(
    const std::array<uint64_t, 2>& sig,
    const std::vector<std::array<uint64_t, 2>>& existing_sigs,
    Scalar threshold) const {
    
    for (const auto& existing_sig : existing_sigs) {
        if (jaccard_similarity(sig, existing_sig) >= threshold) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Retrieval Policy Implementation
// ============================================================================

RetrievalPolicy::RetrievalPolicy(const TieredLTMConfig& config) : config_(config) {}

RetrievalResult RetrievalPolicy::retrieve(
    const Eigen::VectorXd& query,
    int k,
    Scalar score_threshold) {
    
    RetrievalResult result;
    auto start = std::chrono::steady_clock::now();
    
    // This is a placeholder - actual implementation would call into TieredLTM
    // Real implementation would:
    // 1. Query hot tier (HNSW)
    // 2. Rerank top results
    // 3. If scores below threshold, backfill from warm
    // 4. Track latency
    
    auto end = std::chrono::steady_clock::now();
    result.latency_ms = std::chrono::duration<Scalar, std::milli>(end - start).count();
    
    return result;
}

std::vector<Scalar> RetrievalPolicy::rerank(
    const Eigen::VectorXd& query,
    const std::vector<MemoryItem>& candidates) {
    
    // Simplified reranking using cosine similarity
    // Production would use MLP or cross-encoder
    std::vector<Scalar> scores;
    scores.reserve(candidates.size());
    
    for (const auto& candidate : candidates) {
        Scalar dot = query.dot(candidate.embedding);
        Scalar norm_q = query.norm();
        Scalar norm_c = candidate.embedding.norm();
        Scalar similarity = dot / (norm_q * norm_c + 1e-10);
        scores.push_back(similarity);
    }
    
    return scores;
}

void RetrievalPolicy::backfill_from_warm(
    RetrievalResult& result,
    const Eigen::VectorXd& query,
    Scalar score_threshold) {
    
    // Check if we need to backfill
    bool needs_backfill = false;
    for (const auto& score : result.scores) {
        if (score < score_threshold) {
            needs_backfill = true;
            break;
        }
    }
    
    if (!needs_backfill) return;
    
    // Placeholder: would query warm tier here
}

// ============================================================================
// Tiered LTM Implementation
// ============================================================================

TieredLTM::TieredLTM(const TieredLTMConfig& config)
    : config_(config), dedup_(config.dedup.num_hashes) {
    
    // Pre-allocate tier capacities
    hot_tier_.reserve(config.hot.capacity);
    warm_tier_.reserve(config.warm.capacity);
    cold_tier_.reserve(config.cold.capacity);
}

void TieredLTM::add(const MemoryItem& item) {
    // Check for duplicates
    if (is_duplicate(item)) {
        stats_.duplicates_blocked++;
        return;
    }
    
    // Add to hot tier by default
    MemoryItem new_item = item;
    new_item.tier = "hot";
    new_item.minhash_sig = dedup_.compute_signature(item.embedding);
    
    // Check capacity and evict if needed
    if (hot_tier_.size() >= static_cast<size_t>(config_.hot.capacity)) {
        // Evict lowest-scoring item to warm
        auto it = std::min_element(hot_tier_.begin(), hot_tier_.end(),
            [this](const MemoryItem& a, const MemoryItem& b) {
                return compute_promotion_score(a) < compute_promotion_score(b);
            });
        
        if (it != hot_tier_.end()) {
            MemoryItem evicted = *it;
            evicted.tier = "warm";
            warm_tier_.push_back(evicted);
            hot_tier_.erase(it);
        }
    }
    
    hot_tier_.push_back(new_item);
    hot_index_[item.source_doc_id] = hot_tier_.size() - 1;
    signatures_.push_back(new_item.minhash_sig);
    stats_.hot_count = hot_tier_.size();
    stats_.total_count++;
}

RetrievalResult TieredLTM::retrieve(const Eigen::VectorXd& query, int k) {
    auto start = std::chrono::steady_clock::now();
    RetrievalResult result;
    
    // Search hot tier using simplified HNSW
    auto hot_results = hnsw_search(query, k);
    
    // Convert to MemoryItems and scores
    for (const auto& [idx, score] : hot_results) {
        if (idx < hot_tier_.size()) {
            result.items.push_back(hot_tier_[idx]);
            result.scores.push_back(score);
            result.tiers.push_back("hot");
        }
    }
    
    // If we don't have enough results, backfill from warm
    if (static_cast<int>(result.items.size()) < k && !warm_tier_.empty()) {
        auto warm_results = ivf_pq_search(query, k - result.items.size());
        
        for (const auto& [idx, score] : warm_results) {
            if (idx < warm_tier_.size()) {
                result.items.push_back(warm_tier_[idx]);
                result.scores.push_back(score);
                result.tiers.push_back("warm");
            }
        }
    }
    
    // Rerank using provenance scores
    for (size_t i = 0; i < result.items.size(); ++i) {
        result.scores[i] *= result.items[i].provenance_score;
    }
    
    auto end = std::chrono::steady_clock::now();
    result.latency_ms = std::chrono::duration<Scalar, std::milli>(end - start).count();
    
    return result;
}

void TieredLTM::consolidate(const MemoryItem& item, Scalar importance) {
    if (importance >= config_.consolidation_threshold) {
        MemoryItem consolidated = item;
        consolidated.importance = importance;
        add(consolidated);
    }
}

void TieredLTM::promote(const std::string& item_id,
                       const std::string& from_tier,
                       const std::string& to_tier) {
    // Find and move item between tiers
    if (from_tier == "warm" && to_tier == "hot") {
        auto it = warm_index_.find(item_id);
        if (it != warm_index_.end() && it->second < warm_tier_.size()) {
            MemoryItem item = warm_tier_[it->second];
            item.tier = "hot";
            hot_tier_.push_back(item);
            hot_index_[item_id] = hot_tier_.size() - 1;
            
            // Remove from warm
            warm_tier_.erase(warm_tier_.begin() + it->second);
            warm_index_.erase(it);
            
            stats_.promotions++;
            stats_.hot_count = hot_tier_.size();
            stats_.warm_count = warm_tier_.size();
        }
    }
}

void TieredLTM::demote(const std::string& item_id,
                      const std::string& from_tier,
                      const std::string& to_tier) {
    // Find and move item between tiers
    if (from_tier == "hot" && to_tier == "warm") {
        auto it = hot_index_.find(item_id);
        if (it != hot_index_.end() && it->second < hot_tier_.size()) {
            MemoryItem item = hot_tier_[it->second];
            item.tier = "warm";
            warm_tier_.push_back(item);
            warm_index_[item_id] = warm_tier_.size() - 1;
            
            // Remove from hot
            hot_tier_.erase(hot_tier_.begin() + it->second);
            hot_index_.erase(it);
            
            stats_.demotions++;
            stats_.hot_count = hot_tier_.size();
            stats_.warm_count = warm_tier_.size();
        }
    }
}

void TieredLTM::maintenance() {
    apply_decay();
    check_promotions();
    check_demotions();
}

void TieredLTM::apply_decay() {
    if (!config_.decay.enable_temporal_decay && !config_.decay.enable_usage_decay) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    // Apply decay to all tiers
    for (auto& item : hot_tier_) {
        Scalar decay_mult = compute_decay_multiplier(item);
        item.importance *= decay_mult;
    }
    
    for (auto& item : warm_tier_) {
        Scalar decay_mult = compute_decay_multiplier(item);
        item.importance *= decay_mult;
    }
}

void TieredLTM::check_promotions() {
    // Check warm tier for items to promote
    std::vector<std::pair<size_t, Scalar>> candidates;
    
    for (size_t i = 0; i < warm_tier_.size(); ++i) {
        Scalar score = compute_promotion_score(warm_tier_[i]);
        candidates.push_back({i, score});
    }
    
    // Sort by promotion score
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Promote top candidates if hot tier has space
    int available_space = config_.hot.capacity - hot_tier_.size();
    for (int i = 0; i < std::min(available_space, static_cast<int>(candidates.size())); ++i) {
        size_t idx = candidates[i].first;
        if (idx < warm_tier_.size()) {
            promote(warm_tier_[idx].source_doc_id, "warm", "hot");
        }
    }
}

void TieredLTM::check_demotions() {
    // Check hot tier for items to demote
    std::vector<std::pair<size_t, Scalar>> candidates;
    
    for (size_t i = 0; i < hot_tier_.size(); ++i) {
        Scalar score = compute_demotion_score(hot_tier_[i]);
        candidates.push_back({i, score});
    }
    
    // Sort by demotion score (higher = more likely to demote)
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Demote if hot tier is over capacity
    int over_capacity = hot_tier_.size() - config_.hot.capacity;
    if (over_capacity > 0) {
        for (int i = 0; i < over_capacity && i < static_cast<int>(candidates.size()); ++i) {
            size_t idx = candidates[i].first;
            if (idx < hot_tier_.size()) {
                demote(hot_tier_[idx].source_doc_id, "hot", "warm");
            }
        }
    }
}

bool TieredLTM::is_duplicate(const MemoryItem& item) const {
    auto sig = dedup_.compute_signature(item.embedding);
    return dedup_.is_duplicate(sig, signatures_, config_.dedup.similarity_threshold);
}

// ============================================================================
// Helper Methods
// ============================================================================

Scalar TieredLTM::compute_decay_multiplier(const MemoryItem& item) const {
    Scalar decay_mult = 1.0;
    
    if (config_.decay.enable_temporal_decay) {
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - item.timestamp).count();
        Scalar age_days = age / 86400.0;
        Scalar half_life = config_.decay.half_life_days;
        
        // Exponential decay: importance *= 0.5^(age_days / half_life)
        decay_mult *= std::pow(0.5, age_days / half_life);
    }
    
    if (config_.decay.enable_usage_decay) {
        // Decay based on lack of access
        auto now = std::chrono::steady_clock::now();
        auto time_since_access = std::chrono::duration_cast<std::chrono::seconds>(
            now - item.last_access).count();
        Scalar days_since_access = time_since_access / 86400.0;
        
        // Gentle decay for non-accessed items
        decay_mult *= std::exp(-days_since_access / (config_.decay.half_life_days * 2));
    }
    
    return decay_mult;
}

Scalar TieredLTM::compute_promotion_score(const MemoryItem& item) const {
    Scalar score = 0.0;
    
    for (const auto& policy : config_.promotion_policies) {
        switch (policy) {
            case PromotionPolicy::RECENT_USE:
                {
                    auto now = std::chrono::steady_clock::now();
                    auto time_since_access = std::chrono::duration_cast<std::chrono::seconds>(
                        now - item.last_access).count();
                    Scalar recency = std::exp(-time_since_access / 86400.0); // Decay over 1 day
                    score += recency * 0.4;
                }
                break;
            case PromotionPolicy::TASK_REWARD:
                score += item.reward * 0.3;
                break;
            case PromotionPolicy::NOVELTY:
                // Higher importance = more novel
                score += item.importance * 0.3;
                break;
        }
    }
    
    // Boost by provenance
    score *= item.provenance_score;
    
    return score;
}

Scalar TieredLTM::compute_demotion_score(const MemoryItem& item) const {
    Scalar score = 0.0;
    
    for (const auto& policy : config_.demotion_policies) {
        switch (policy) {
            case DemotionPolicy::STALE:
                {
                    auto now = std::chrono::steady_clock::now();
                    auto age = std::chrono::duration_cast<std::chrono::seconds>(
                        now - item.timestamp).count();
                    Scalar age_days = age / 86400.0;
                    score += age_days / config_.decay.half_life_days * 0.4;
                }
                break;
            case DemotionPolicy::LOW_REWARD:
                score += (1.0 - item.reward) * 0.3;
                break;
            case DemotionPolicy::REDUNDANT:
                // Low importance = redundant
                score += (1.0 - item.importance) * 0.3;
                break;
        }
    }
    
    // Penalize low provenance
    score *= (2.0 - item.provenance_score);
    
    return score;
}

std::vector<std::pair<size_t, Scalar>> TieredLTM::hnsw_search(
    const Eigen::VectorXd& query, int k) const {
    
    // Simplified HNSW: just compute cosine similarities
    std::vector<std::pair<size_t, Scalar>> results;
    results.reserve(hot_tier_.size());
    
    for (size_t i = 0; i < hot_tier_.size(); ++i) {
        Scalar sim = cosine_similarity(query, hot_tier_[i].embedding);
        results.push_back({i, sim});
    }
    
    // Sort by similarity (descending)
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top-k
    if (static_cast<int>(results.size()) > k) {
        results.resize(k);
    }
    
    return results;
}

std::vector<std::pair<size_t, Scalar>> TieredLTM::ivf_pq_search(
    const Eigen::VectorXd& query, int k) const {
    
    // Simplified IVF-PQ: just compute cosine similarities with quantization penalty
    std::vector<std::pair<size_t, Scalar>> results;
    results.reserve(warm_tier_.size());
    
    for (size_t i = 0; i < warm_tier_.size(); ++i) {
        Scalar sim = cosine_similarity(query, warm_tier_[i].embedding);
        // Simulate quantization error
        sim *= 0.97; // ~3% recall drop from 8-bit PQ
        results.push_back({i, sim});
    }
    
    // Sort by similarity (descending)
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top-k
    if (static_cast<int>(results.size()) > k) {
        results.resize(k);
    }
    
    return results;
}

Scalar TieredLTM::cosine_similarity(const Eigen::VectorXd& a, const Eigen::VectorXd& b) const {
    Scalar dot = a.dot(b);
    Scalar norm_a = a.norm();
    Scalar norm_b = b.norm();
    return dot / (norm_a * norm_b + 1e-10);
}

} // namespace hab
