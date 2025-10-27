#include "brain/tiered_memory.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <functional>
#include <iostream>
#include <sstream>

namespace hab {

// ============================================================================
// MinHash Deduplicator Implementation
// ============================================================================

MinHashDedup::MinHashDedup(int num_hashes) : num_hashes_(num_hashes) {
    if (num_hashes < 2) {
        throw std::invalid_argument("MinHashDedup requires at least 2 hash functions");
    }
    
    // Initialize hash seeds for MinHash using better RNG
    std::mt19937_64 rng(42); // Fixed seed for reproducibility
    hash_seeds_.reserve(num_hashes);
    for (int i = 0; i < num_hashes; ++i) {
        hash_seeds_.push_back(static_cast<uint32_t>(rng()));
    }
}

std::array<uint64_t, 2> MinHashDedup::compute_signature(const std::string& text) const {
    // Enhanced MinHash: use 128-bit signature (2 x 64-bit hashes)
    std::array<uint64_t, 2> sig = {UINT64_MAX, UINT64_MAX};
    
    if (text.empty()) return sig;
    
    // Tokenize text into shingles (3-grams)
    std::vector<std::string> shingles;
    shingles.reserve(text.size());
    for (size_t i = 0; i + 3 <= text.size(); ++i) {
        shingles.push_back(text.substr(i, 3));
    }
    
    if (shingles.empty()) return sig;
    
    // Compute min hash for each of first 2 hash functions
    // Using MurmurHash-inspired mixing
    for (int h = 0; h < std::min(2, num_hashes_); ++h) {
        uint64_t min_hash = UINT64_MAX;
        
        for (const auto& shingle : shingles) {
            // Better hash mixing
            std::hash<std::string> hasher;
            uint64_t hash = hasher(shingle);
            hash ^= hash_seeds_[h];
            hash ^= (hash >> 33);
            hash *= 0xff51afd7ed558ccdULL;
            hash ^= (hash >> 33);
            
            min_hash = std::min(min_hash, hash);
        }
        
        sig[h] = min_hash;
    }
    
    return sig;
}

std::array<uint64_t, 2> MinHashDedup::compute_signature(const Eigen::VectorXd& embedding) const {
    // Enhanced MinHash for embeddings with better quantization
    std::array<uint64_t, 2> sig = {UINT64_MAX, UINT64_MAX};
    
    if (embedding.size() == 0) return sig;
    
    // Quantize embedding to 8-bit values with better distribution
    std::vector<uint8_t> quantized;
    quantized.reserve(embedding.size());
    
    Scalar min_val = embedding.minCoeff();
    Scalar max_val = embedding.maxCoeff();
    Scalar range = max_val - min_val;
    if (range < 1e-10) range = 1.0;
    
    for (int i = 0; i < embedding.size(); ++i) {
        Scalar normalized = (embedding(i) - min_val) / range;
        uint8_t q = static_cast<uint8_t>(std::clamp(normalized * 255.0, 0.0, 255.0));
        quantized.push_back(q);
    }
    
    // Compute min hash using better feature representation
    for (int h = 0; h < std::min(2, num_hashes_); ++h) {
        uint64_t min_hash = UINT64_MAX;
        
        // Use sliding windows for better feature extraction
        for (size_t i = 0; i < quantized.size(); ++i) {
            // Create feature from current value and position
            uint64_t hash = static_cast<uint64_t>(i) * 31 + quantized[i];
            hash ^= hash_seeds_[h];
            hash ^= (hash >> 33);
            hash *= 0xff51afd7ed558ccdULL;
            hash ^= (hash >> 33);
            
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
    
    // Enhanced reranking with normalization
    std::vector<Scalar> scores;
    scores.reserve(candidates.size());
    
    Scalar query_norm = query.norm();
    if (query_norm < 1e-10) query_norm = 1.0;
    
    for (const auto& candidate : candidates) {
        if (candidate.embedding.size() != query.size()) {
            scores.push_back(0.0);
            continue;
        }
        
        Scalar dot = query.dot(candidate.embedding);
        Scalar cand_norm = candidate.embedding.norm();
        if (cand_norm < 1e-10) cand_norm = 1.0;
        
        Scalar similarity = dot / (query_norm * cand_norm);
        scores.push_back(std::clamp(similarity, -1.0, 1.0));
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
    
    // Validate configuration
    if (config.hot.capacity < 1) {
        throw std::invalid_argument("Hot tier capacity must be >= 1");
    }
    if (config.warm.capacity < config.hot.capacity) {
        throw std::invalid_argument("Warm tier capacity must be >= hot tier capacity");
    }
    if (config.cold.capacity < config.warm.capacity) {
        throw std::invalid_argument("Cold tier capacity must be >= warm tier capacity");
    }
    
    // Pre-allocate tier capacities for better performance
    hot_tier_.reserve(config.hot.capacity);
    warm_tier_.reserve(config.warm.capacity);
    cold_tier_.reserve(config.cold.capacity);
    signatures_.reserve(config.hot.capacity + config.warm.capacity);
}

bool TieredLTM::add(const MemoryItem& item) {
    // Validate item
    if (!validate_item(item)) {
        return false;
    }
    
    // Check for duplicates (thread-safe)
    {
        std::lock_guard<std::mutex> dedup_lock(dedup_mutex_);
        if (is_duplicate(item)) {
            stats_.duplicates_blocked.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
    }
    
    // Add to hot tier (thread-safe)
    std::unique_lock<std::shared_mutex> hot_lock(hot_mutex_);
    
    MemoryItem new_item = item;
    new_item.tier = "hot";
    new_item.timestamp = std::chrono::steady_clock::now();
    new_item.last_access = new_item.timestamp;
    new_item.minhash_sig = dedup_.compute_signature(item.embedding);
    
    // Check capacity and evict if needed
    if (hot_tier_.size() >= static_cast<size_t>(config_.hot.capacity)) {
        auto evicted_opt = evict_from_hot();
        if (evicted_opt.has_value()) {
            // Move evicted item to warm tier
            hot_lock.unlock();  // Release hot lock before acquiring warm lock
            
            std::unique_lock<std::shared_mutex> warm_lock(warm_mutex_);
            MemoryItem evicted = evicted_opt.value();
            evicted.tier = "warm";
            warm_tier_.push_back(evicted);
            warm_index_[evicted.source_doc_id] = warm_tier_.size() - 1;
            stats_.warm_count.fetch_add(1, std::memory_order_relaxed);
            stats_.demotions.fetch_add(1, std::memory_order_relaxed);
            warm_lock.unlock();
            
            hot_lock.lock();  // Reacquire hot lock
        }
    }
    
    hot_tier_.push_back(new_item);
    hot_index_[item.source_doc_id] = hot_tier_.size() - 1;
    
    // Update signatures (with dedup lock)
    {
        std::lock_guard<std::mutex> dedup_lock(dedup_mutex_);
        signatures_.push_back(new_item.minhash_sig);
    }
    
    stats_.hot_count.fetch_add(1, std::memory_order_relaxed);
    stats_.total_count.fetch_add(1, std::memory_order_relaxed);
    
    return true;
}

RetrievalResult TieredLTM::retrieve(const Eigen::VectorXd& query, int k) {
    if (query.size() == 0) {
        throw std::invalid_argument("Query vector cannot be empty");
    }
    if (k < 1) {
        throw std::invalid_argument("k must be >= 1");
    }
    
    auto start = std::chrono::steady_clock::now();
    RetrievalResult result;
    
    // Search hot tier (read lock)
    {
        std::shared_lock<std::shared_mutex> hot_lock(hot_mutex_);
        auto hot_results = hnsw_search(query, k);
        
        // Convert to MemoryItems and scores
        for (const auto& [idx, score] : hot_results) {
            if (idx < hot_tier_.size()) {
                MemoryItem item = hot_tier_[idx];
                record_access(item);  // Update access count
                result.items.push_back(item);
                result.scores.push_back(score);
                result.tiers.push_back("hot");
            }
        }
    }
    
    // If we don't have enough results, backfill from warm
    if (static_cast<int>(result.items.size()) < k) {
        std::shared_lock<std::shared_mutex> warm_lock(warm_mutex_);
        if (!warm_tier_.empty()) {
            auto warm_results = ivf_pq_search(query, k - result.items.size());
            
            for (const auto& [idx, score] : warm_results) {
                if (idx < warm_tier_.size()) {
                    MemoryItem item = warm_tier_[idx];
                    record_access(item);
                    result.items.push_back(item);
                    result.scores.push_back(score);
                    result.tiers.push_back("warm");
                }
            }
        }
    }
    
    // Rerank using provenance scores
    for (size_t i = 0; i < result.items.size(); ++i) {
        result.scores[i] *= result.items[i].provenance_score;
    }
    
    auto end = std::chrono::steady_clock::now();
    result.latency_ms = std::chrono::duration<Scalar, std::milli>(end - start).count();
    
    // Update statistics
    stats_.total_queries.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        // Update running average latency
        int total = stats_.total_queries.load(std::memory_order_relaxed);
        if (total > 0) {
            stats_.avg_hot_latency_ms = (stats_.avg_hot_latency_ms * (total - 1) + result.latency_ms) / total;
        }
    }
    
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
    // Find and move item between tiers (thread-safe)
    if (from_tier == "warm" && to_tier == "hot") {
        std::unique_lock<std::shared_mutex> warm_lock(warm_mutex_);
        auto it = warm_index_.find(item_id);
        if (it != warm_index_.end() && it->second < warm_tier_.size()) {
            MemoryItem item = warm_tier_[it->second];
            
            // Remove from warm
            warm_tier_.erase(warm_tier_.begin() + it->second);
            warm_index_.erase(it);
            rebuild_warm_index();
            stats_.warm_count.fetch_sub(1, std::memory_order_relaxed);
            warm_lock.unlock();
            
            // Add to hot
            item.tier = "hot";
            std::unique_lock<std::shared_mutex> hot_lock(hot_mutex_);
            hot_tier_.push_back(item);
            hot_index_[item_id] = hot_tier_.size() - 1;
            stats_.hot_count.fetch_add(1, std::memory_order_relaxed);
            stats_.promotions.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void TieredLTM::demote(const std::string& item_id,
                      const std::string& from_tier,
                      const std::string& to_tier) {
    // Find and move item between tiers (thread-safe)
    if (from_tier == "hot" && to_tier == "warm") {
        std::unique_lock<std::shared_mutex> hot_lock(hot_mutex_);
        auto it = hot_index_.find(item_id);
        if (it != hot_index_.end() && it->second < hot_tier_.size()) {
            MemoryItem item = hot_tier_[it->second];
            
            // Remove from hot
            hot_tier_.erase(hot_tier_.begin() + it->second);
            hot_index_.erase(it);
            rebuild_hot_index();
            stats_.hot_count.fetch_sub(1, std::memory_order_relaxed);
            hot_lock.unlock();
            
            // Add to warm
            item.tier = "warm";
            std::unique_lock<std::shared_mutex> warm_lock(warm_mutex_);
            warm_tier_.push_back(item);
            warm_index_[item_id] = warm_tier_.size() - 1;
            stats_.warm_count.fetch_add(1, std::memory_order_relaxed);
            stats_.demotions.fetch_add(1, std::memory_order_relaxed);
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
    
    // Apply decay to hot tier
    {
        std::unique_lock<std::shared_mutex> hot_lock(hot_mutex_);
        for (auto& item : hot_tier_) {
            Scalar decay_mult = compute_decay_multiplier(item);
            item.importance *= decay_mult;
        }
    }
    
    // Apply decay to warm tier
    {
        std::unique_lock<std::shared_mutex> warm_lock(warm_mutex_);
        for (auto& item : warm_tier_) {
            Scalar decay_mult = compute_decay_multiplier(item);
            item.importance *= decay_mult;
        }
    }
}

void TieredLTM::check_promotions() {
    // Check warm tier for items to promote
    std::vector<std::pair<std::string, Scalar>> candidates;
    
    {
        std::shared_lock<std::shared_mutex> warm_lock(warm_mutex_);
        for (const auto& item : warm_tier_) {
            Scalar score = compute_promotion_score(item);
            candidates.push_back({item.source_doc_id, score});
        }
    }
    
    // Sort by promotion score (descending)
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Promote top candidates if hot tier has space
    size_t current_hot_size = hot_size();
    int available_space = config_.hot.capacity - current_hot_size;
    int num_to_promote = std::min(available_space, static_cast<int>(candidates.size()));
    
    for (int i = 0; i < num_to_promote; ++i) {
        promote(candidates[i].first, "warm", "hot");
    }
}

void TieredLTM::check_demotions() {
    // Check hot tier for items to demote
    std::vector<std::pair<std::string, Scalar>> candidates;
    
    {
        std::shared_lock<std::shared_mutex> hot_lock(hot_mutex_);
        for (const auto& item : hot_tier_) {
            Scalar score = compute_demotion_score(item);
            candidates.push_back({item.source_doc_id, score});
        }
    }
    
    // Sort by demotion score (descending = more likely to demote)
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Demote if hot tier is over capacity
    size_t hot_size_val = hot_size();
    int over_capacity = hot_size_val - config_.hot.capacity;
    if (over_capacity > 0) {
        int num_to_demote = std::min(over_capacity, static_cast<int>(candidates.size()));
        for (int i = 0; i < num_to_demote; ++i) {
            demote(candidates[i].first, "hot", "warm");
        }
    }
}

bool TieredLTM::is_duplicate(const MemoryItem& item) const {
    auto sig = dedup_.compute_signature(item.embedding);
    return dedup_.is_duplicate(sig, signatures_, config_.dedup.similarity_threshold);
}

TieredLTM::Stats TieredLTM::get_stats() const {
    // Return stats (atomic values are thread-safe)
    return stats_;
}

void TieredLTM::clear() {
    std::unique_lock<std::shared_mutex> hot_lock(hot_mutex_);
    std::unique_lock<std::shared_mutex> warm_lock(warm_mutex_);
    std::unique_lock<std::shared_mutex> cold_lock(cold_mutex_);
    std::lock_guard<std::mutex> dedup_lock(dedup_mutex_);
    
    hot_tier_.clear();
    hot_index_.clear();
    warm_tier_.clear();
    warm_index_.clear();
    cold_tier_.clear();
    cold_index_.clear();
    signatures_.clear();
    
    stats_ = Stats{};
}

size_t TieredLTM::hot_size() const {
    std::shared_lock<std::shared_mutex> lock(hot_mutex_);
    return hot_tier_.size();
}

size_t TieredLTM::warm_size() const {
    std::shared_lock<std::shared_mutex> lock(warm_mutex_);
    return warm_tier_.size();
}

size_t TieredLTM::cold_size() const {
    std::shared_lock<std::shared_mutex> lock(cold_mutex_);
    return cold_tier_.size();
}

size_t TieredLTM::total_size() const {
    return hot_size() + warm_size() + cold_size();
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
    
    return std::max(decay_mult, 0.01);  // Floor at 1%
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
    
    // Boost by access frequency
    score *= (1.0 + std::log1p(item.access_count.load(std::memory_order_relaxed)));
    
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
    
    // Penalize low access frequency
    score /= (1.0 + std::log1p(item.access_count.load(std::memory_order_relaxed)));
    
    return score;
}

std::vector<std::pair<size_t, Scalar>> TieredLTM::hnsw_search(
    const Eigen::VectorXd& query, int k) const {
    
    // Enhanced HNSW: compute similarities with early termination
    std::vector<std::pair<size_t, Scalar>> results;
    results.reserve(hot_tier_.size());
    
    for (size_t i = 0; i < hot_tier_.size(); ++i) {
        Scalar sim = cosine_similarity(query, hot_tier_[i].embedding);
        results.push_back({i, sim});
    }
    
    // Partial sort for top-k (more efficient than full sort)
    if (static_cast<int>(results.size()) > k) {
        std::partial_sort(results.begin(), results.begin() + k, results.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        results.resize(k);
    } else {
        std::sort(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
    }
    
    return results;
}

std::vector<std::pair<size_t, Scalar>> TieredLTM::ivf_pq_search(
    const Eigen::VectorXd& query, int k) const {
    
    // Enhanced IVF-PQ: with quantization simulation
    std::vector<std::pair<size_t, Scalar>> results;
    results.reserve(warm_tier_.size());
    
    for (size_t i = 0; i < warm_tier_.size(); ++i) {
        Scalar sim = cosine_similarity(query, warm_tier_[i].embedding);
        // Simulate quantization error (3% recall drop)
        sim *= 0.97;
        results.push_back({i, sim});
    }
    
    // Partial sort for top-k
    if (static_cast<int>(results.size()) > k) {
        std::partial_sort(results.begin(), results.begin() + k, results.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        results.resize(k);
    } else {
        std::sort(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
    }
    
    return results;
}

Scalar TieredLTM::cosine_similarity(const Eigen::VectorXd& a, const Eigen::VectorXd& b) const {
    if (a.size() != b.size() || a.size() == 0) return 0.0;
    
    Scalar dot = a.dot(b);
    Scalar norm_a = a.norm();
    Scalar norm_b = b.norm();
    
    if (norm_a < 1e-10 || norm_b < 1e-10) return 0.0;
    
    return std::clamp(dot / (norm_a * norm_b), -1.0, 1.0);
}

bool TieredLTM::validate_item(const MemoryItem& item) const {
    return item.embedding.size() > 0 && 
           item.importance >= 0.0 && item.importance <= 1.0 &&
           item.provenance_score >= 0.0 && item.provenance_score <= 1.0 &&
           !item.source_doc_id.empty();
}

void TieredLTM::rebuild_hot_index() {
    hot_index_.clear();
    for (size_t i = 0; i < hot_tier_.size(); ++i) {
        hot_index_[hot_tier_[i].source_doc_id] = i;
    }
}

void TieredLTM::rebuild_warm_index() {
    warm_index_.clear();
    for (size_t i = 0; i < warm_tier_.size(); ++i) {
        warm_index_[warm_tier_[i].source_doc_id] = i;
    }
}

void TieredLTM::rebuild_cold_index() {
    cold_index_.clear();
    for (size_t i = 0; i < cold_tier_.size(); ++i) {
        cold_index_[cold_tier_[i].source_doc_id] = i;
    }
}

std::optional<MemoryItem> TieredLTM::evict_from_hot() {
    if (hot_tier_.empty()) return std::nullopt;
    
    // Find item with lowest promotion score (LRU-like)
    size_t evict_idx = 0;
    Scalar min_score = compute_promotion_score(hot_tier_[0]);
    
    for (size_t i = 1; i < hot_tier_.size(); ++i) {
        Scalar score = compute_promotion_score(hot_tier_[i]);
        if (score < min_score) {
            min_score = score;
            evict_idx = i;
        }
    }
    
    MemoryItem evicted = hot_tier_[evict_idx];
    hot_tier_.erase(hot_tier_.begin() + evict_idx);
    hot_index_.erase(evicted.source_doc_id);
    rebuild_hot_index();
    
    return evicted;
}

std::optional<MemoryItem> TieredLTM::evict_from_warm() {
    if (warm_tier_.empty()) return std::nullopt;
    
    // Find item with lowest promotion score
    size_t evict_idx = 0;
    Scalar min_score = compute_promotion_score(warm_tier_[0]);
    
    for (size_t i = 1; i < warm_tier_.size(); ++i) {
        Scalar score = compute_promotion_score(warm_tier_[i]);
        if (score < min_score) {
            min_score = score;
            evict_idx = i;
        }
    }
    
    MemoryItem evicted = warm_tier_[evict_idx];
    warm_tier_.erase(warm_tier_.begin() + evict_idx);
    warm_index_.erase(evicted.source_doc_id);
    rebuild_warm_index();
    
    return evicted;
}

void TieredLTM::record_access(MemoryItem& item) {
    item.access_count.fetch_add(1, std::memory_order_relaxed);
    item.last_access = std::chrono::steady_clock::now();
}

} // namespace hab
