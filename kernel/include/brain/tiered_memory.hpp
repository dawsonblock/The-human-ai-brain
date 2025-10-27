#pragma once

#include "utils.hpp"
#include <Eigen/Dense>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <memory>
#include <array>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <optional>
#include <stdexcept>

namespace hab {

// ============================================================================
// Tiered Memory Configuration
// ============================================================================

enum class IndexType {
    HNSW,        // Hierarchical Navigable Small World (hot)
    IVF_PQ,      // Inverted File with Product Quantization (warm)
    PARQUET      // Cold archival storage
};

enum class PromotionPolicy {
    RECENT_USE,
    TASK_REWARD,
    NOVELTY
};

enum class DemotionPolicy {
    STALE,
    LOW_REWARD,
    REDUNDANT
};

// Hot tier configuration (in-memory HNSW)
struct HotTierConfig {
    int capacity = 50000;
    IndexType index = IndexType::HNSW;
    int hnsw_M = 32;                    // Number of bi-directional links per node
    int hnsw_ef_construction = 200;     // Build-time search parameter
    int hnsw_ef_search = 128;           // Query-time search parameter
    int latency_budget_ms = 10;         // Max query latency
};

// Warm tier configuration (SSD-backed IVF-PQ)
struct WarmTierConfig {
    int capacity = 300000;
    IndexType index = IndexType::IVF_PQ;
    int ivf_nlist = 4096;               // Number of IVF clusters
    int pq_m = 64;                      // Number of PQ subquantizers
    Scalar recall_target = 0.95;        // Target recall rate
    int latency_budget_ms = 40;         // Max query latency
};

// Cold tier configuration (object store)
struct ColdTierConfig {
    int capacity = 2000000;
    std::string format = "parquet";
    std::string storage_path = "./cold_storage";
    bool async_mode = true;             // Async retrieval
};

// Deduplication configuration
enum class DedupMethod {
    MINHASH_128,
    SIMHASH_64
};

struct DedupConfig {
    DedupMethod method = DedupMethod::MINHASH_128;
    int num_hashes = 128;               // Number of hash functions
    Scalar similarity_threshold = 0.95; // Dedup if similarity > threshold
};

// Decay configuration
struct DecayConfig {
    Scalar half_life_days = 30.0;       // Time for importance to decay by 50%
    bool enable_temporal_decay = true;
    bool enable_usage_decay = true;
};

// Retrieval configuration
struct RetrievalConfig {
    int hot_k = 50;                     // Retrieve top-k from hot tier
    bool rerank_enabled = true;         // Enable MLP/cross-encoder reranking
    Scalar backfill_threshold = 0.5;    // Backfill from warm if score < threshold
    bool provenance_filter = true;      // Block low-trust hits from steering GW
};

// Complete tiered LTM configuration
struct TieredLTMConfig {
    HotTierConfig hot;
    WarmTierConfig warm;
    ColdTierConfig cold;
    DedupConfig dedup;
    DecayConfig decay;
    RetrievalConfig retrieval;
    Scalar consolidation_threshold = 0.7;
    std::vector<PromotionPolicy> promotion_policies = {
        PromotionPolicy::RECENT_USE,
        PromotionPolicy::TASK_REWARD,
        PromotionPolicy::NOVELTY
    };
    std::vector<DemotionPolicy> demotion_policies = {
        DemotionPolicy::STALE,
        DemotionPolicy::LOW_REWARD,
        DemotionPolicy::REDUNDANT
    };
};

// ============================================================================
// Memory Item with Metadata
// ============================================================================

struct MemoryItem {
    Eigen::VectorXd embedding;          // Vector representation
    Eigen::VectorXd gw_state;           // Global workspace state
    Eigen::VectorXd qw_onehot;          // Quantum workspace one-hot
    int action;
    Scalar reward;
    TimePoint timestamp;
    
    // Metadata for tiering
    Scalar importance = 0.5;
    Scalar provenance_score = 1.0;      // Trust score [0, 1]
    std::atomic<int> access_count{0};   // Thread-safe access count
    TimePoint last_access;
    std::string source_doc_id;
    std::string tier;                   // "hot", "warm", "cold"
    
    // Validation
    bool is_valid() const {
        return !embedding.size() == 0 && 
               importance >= 0.0 && importance <= 1.0 &&
               provenance_score >= 0.0 && provenance_score <= 1.0 &&
               !source_doc_id.empty();
    }
    
    // MinHash signature for deduplication
    std::array<uint64_t, 2> minhash_sig = {0, 0};
    
    MemoryItem() : action(0), reward(0.0),
                   timestamp(std::chrono::steady_clock::now()),
                   last_access(std::chrono::steady_clock::now()),
                   tier("hot") {}
    
    // Custom copy constructor (atomic cannot be copied directly)
    MemoryItem(const MemoryItem& other) 
        : embedding(other.embedding),
          gw_state(other.gw_state),
          qw_onehot(other.qw_onehot),
          action(other.action),
          reward(other.reward),
          timestamp(other.timestamp),
          importance(other.importance),
          provenance_score(other.provenance_score),
          access_count(other.access_count.load(std::memory_order_relaxed)),
          last_access(other.last_access),
          source_doc_id(other.source_doc_id),
          tier(other.tier),
          minhash_sig(other.minhash_sig) {}
    
    // Custom copy assignment (atomic cannot be copied directly)
    MemoryItem& operator=(const MemoryItem& other) {
        if (this != &other) {
            embedding = other.embedding;
            gw_state = other.gw_state;
            qw_onehot = other.qw_onehot;
            action = other.action;
            reward = other.reward;
            timestamp = other.timestamp;
            importance = other.importance;
            provenance_score = other.provenance_score;
            access_count.store(other.access_count.load(std::memory_order_relaxed), 
                              std::memory_order_relaxed);
            last_access = other.last_access;
            source_doc_id = other.source_doc_id;
            tier = other.tier;
            minhash_sig = other.minhash_sig;
        }
        return *this;
    }
    
    // Move constructor
    MemoryItem(MemoryItem&& other) noexcept
        : embedding(std::move(other.embedding)),
          gw_state(std::move(other.gw_state)),
          qw_onehot(std::move(other.qw_onehot)),
          action(other.action),
          reward(other.reward),
          timestamp(other.timestamp),
          importance(other.importance),
          provenance_score(other.provenance_score),
          access_count(other.access_count.load(std::memory_order_relaxed)),
          last_access(other.last_access),
          source_doc_id(std::move(other.source_doc_id)),
          tier(std::move(other.tier)),
          minhash_sig(other.minhash_sig) {}
    
    // Move assignment
    MemoryItem& operator=(MemoryItem&& other) noexcept {
        if (this != &other) {
            embedding = std::move(other.embedding);
            gw_state = std::move(other.gw_state);
            qw_onehot = std::move(other.qw_onehot);
            action = other.action;
            reward = other.reward;
            timestamp = other.timestamp;
            importance = other.importance;
            provenance_score = other.provenance_score;
            access_count.store(other.access_count.load(std::memory_order_relaxed), 
                              std::memory_order_relaxed);
            last_access = other.last_access;
            source_doc_id = std::move(other.source_doc_id);
            tier = std::move(other.tier);
            minhash_sig = other.minhash_sig;
        }
        return *this;
    }
};

// ============================================================================
// MinHash Deduplicator
// ============================================================================

class MinHashDedup {
public:
    explicit MinHashDedup(int num_hashes = 128);
    
    // Compute MinHash signature for a text string
    std::array<uint64_t, 2> compute_signature(const std::string& text) const;
    
    // Compute MinHash signature for an embedding
    std::array<uint64_t, 2> compute_signature(const Eigen::VectorXd& embedding) const;
    
    // Compute Jaccard similarity from signatures
    Scalar jaccard_similarity(const std::array<uint64_t, 2>& sig1,
                             const std::array<uint64_t, 2>& sig2) const;
    
    // Check if item is duplicate
    bool is_duplicate(const std::array<uint64_t, 2>& sig,
                     const std::vector<std::array<uint64_t, 2>>& existing_sigs,
                     Scalar threshold = 0.95) const;
    
private:
    int num_hashes_;
    std::vector<uint32_t> hash_seeds_;
};

// ============================================================================
// Retrieval Policy
// ============================================================================

struct RetrievalResult {
    std::vector<MemoryItem> items;
    std::vector<Scalar> scores;
    std::vector<std::string> tiers;     // Which tier each item came from
    Scalar latency_ms;
};

class RetrievalPolicy {
public:
    explicit RetrievalPolicy(const TieredLTMConfig& config);
    
    // Route query through tiers with reranking
    RetrievalResult retrieve(const Eigen::VectorXd& query,
                            int k = 50,
                            Scalar score_threshold = 0.5);
    
    // Rerank results using MLP or cross-encoder
    std::vector<Scalar> rerank(const Eigen::VectorXd& query,
                              const std::vector<MemoryItem>& candidates);
    
    // Backfill from warm tier if needed
    void backfill_from_warm(RetrievalResult& result,
                           const Eigen::VectorXd& query,
                           Scalar score_threshold);
    
private:
    TieredLTMConfig config_;
};

// ============================================================================
// Tiered Long-Term Memory
// ============================================================================

class TieredLTM {
public:
    explicit TieredLTM(const TieredLTMConfig& config = TieredLTMConfig{});
    
    // Add item with automatic tier placement
    // Returns: true if added, false if duplicate or invalid
    bool add(const MemoryItem& item);
    
    // Retrieve top-k items with smart routing
    // Throws: std::invalid_argument if query is empty or k < 1
    RetrievalResult retrieve(const Eigen::VectorXd& query, int k = 50);
    
    // Consolidate from STM
    void consolidate(const MemoryItem& item, Scalar importance);
    
    // Promote item to higher tier
    void promote(const std::string& item_id, const std::string& from_tier, const std::string& to_tier);
    
    // Demote item to lower tier
    void demote(const std::string& item_id, const std::string& from_tier, const std::string& to_tier);
    
    // Periodic maintenance (decay, compaction, promotion/demotion)
    void maintenance();
    
    // Apply temporal and usage decay
    void apply_decay();
    
    // Check and apply promotion policies
    void check_promotions();
    
    // Check and apply demotion policies
    void check_demotions();
    
    // Deduplicate incoming item
    bool is_duplicate(const MemoryItem& item) const;
    
    // Statistics
    struct Stats {
        std::atomic<int> hot_count{0};
        std::atomic<int> warm_count{0};
        std::atomic<int> cold_count{0};
        std::atomic<int> total_count{0};
        std::atomic<int> duplicates_blocked{0};
        std::atomic<int> promotions{0};
        std::atomic<int> demotions{0};
        std::atomic<int> total_queries{0};
        Scalar avg_hot_latency_ms = 0.0;  // Updated under lock
        Scalar avg_warm_latency_ms = 0.0; // Updated under lock
        
        // Default constructor
        Stats() = default;
        
        // Custom copy constructor (atomic cannot be copied directly)
        Stats(const Stats& other) 
            : hot_count(other.hot_count.load(std::memory_order_relaxed)),
              warm_count(other.warm_count.load(std::memory_order_relaxed)),
              cold_count(other.cold_count.load(std::memory_order_relaxed)),
              total_count(other.total_count.load(std::memory_order_relaxed)),
              duplicates_blocked(other.duplicates_blocked.load(std::memory_order_relaxed)),
              promotions(other.promotions.load(std::memory_order_relaxed)),
              demotions(other.demotions.load(std::memory_order_relaxed)),
              total_queries(other.total_queries.load(std::memory_order_relaxed)),
              avg_hot_latency_ms(other.avg_hot_latency_ms),
              avg_warm_latency_ms(other.avg_warm_latency_ms) {}
        
        // Custom copy assignment
        Stats& operator=(const Stats& other) {
            if (this != &other) {
                hot_count.store(other.hot_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                warm_count.store(other.warm_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                cold_count.store(other.cold_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                total_count.store(other.total_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                duplicates_blocked.store(other.duplicates_blocked.load(std::memory_order_relaxed), std::memory_order_relaxed);
                promotions.store(other.promotions.load(std::memory_order_relaxed), std::memory_order_relaxed);
                demotions.store(other.demotions.load(std::memory_order_relaxed), std::memory_order_relaxed);
                total_queries.store(other.total_queries.load(std::memory_order_relaxed), std::memory_order_relaxed);
                avg_hot_latency_ms = other.avg_hot_latency_ms;
                avg_warm_latency_ms = other.avg_warm_latency_ms;
            }
            return *this;
        }
        
        // Move constructor
        Stats(Stats&& other) noexcept
            : hot_count(other.hot_count.load(std::memory_order_relaxed)),
              warm_count(other.warm_count.load(std::memory_order_relaxed)),
              cold_count(other.cold_count.load(std::memory_order_relaxed)),
              total_count(other.total_count.load(std::memory_order_relaxed)),
              duplicates_blocked(other.duplicates_blocked.load(std::memory_order_relaxed)),
              promotions(other.promotions.load(std::memory_order_relaxed)),
              demotions(other.demotions.load(std::memory_order_relaxed)),
              total_queries(other.total_queries.load(std::memory_order_relaxed)),
              avg_hot_latency_ms(other.avg_hot_latency_ms),
              avg_warm_latency_ms(other.avg_warm_latency_ms) {}
        
        // Move assignment
        Stats& operator=(Stats&& other) noexcept {
            if (this != &other) {
                hot_count.store(other.hot_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                warm_count.store(other.warm_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                cold_count.store(other.cold_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                total_count.store(other.total_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                duplicates_blocked.store(other.duplicates_blocked.load(std::memory_order_relaxed), std::memory_order_relaxed);
                promotions.store(other.promotions.load(std::memory_order_relaxed), std::memory_order_relaxed);
                demotions.store(other.demotions.load(std::memory_order_relaxed), std::memory_order_relaxed);
                total_queries.store(other.total_queries.load(std::memory_order_relaxed), std::memory_order_relaxed);
                avg_hot_latency_ms = other.avg_hot_latency_ms;
                avg_warm_latency_ms = other.avg_warm_latency_ms;
            }
            return *this;
        }
    };
    
    Stats get_stats() const;
    
    // Clear all tiers (for testing)
    void clear();
    
    // Get tier sizes
    size_t hot_size() const;
    size_t warm_size() const;
    size_t cold_size() const;
    size_t total_size() const;
    
private:
    TieredLTMConfig config_;
    
    // Hot tier (in-memory HNSW index)
    std::vector<MemoryItem> hot_tier_;
    std::unordered_map<std::string, size_t> hot_index_; // doc_id -> index
    mutable std::shared_mutex hot_mutex_;  // Read-write lock for hot tier
    
    // Warm tier (simulated IVF-PQ)
    std::vector<MemoryItem> warm_tier_;
    std::unordered_map<std::string, size_t> warm_index_;
    mutable std::shared_mutex warm_mutex_; // Read-write lock for warm tier
    
    // Cold tier (archival)
    std::vector<MemoryItem> cold_tier_;
    std::unordered_map<std::string, size_t> cold_index_;
    mutable std::shared_mutex cold_mutex_; // Read-write lock for cold tier
    
    // Deduplication
    MinHashDedup dedup_;
    std::vector<std::array<uint64_t, 2>> signatures_;
    std::mutex dedup_mutex_;  // Protects signatures_
    
    // Statistics
    Stats stats_;
    mutable std::mutex stats_mutex_;  // Protects non-atomic stats
    
    // Helper: compute decay multiplier
    Scalar compute_decay_multiplier(const MemoryItem& item) const;
    
    // Helper: compute promotion score
    Scalar compute_promotion_score(const MemoryItem& item) const;
    
    // Helper: compute demotion score
    Scalar compute_demotion_score(const MemoryItem& item) const;
    
    // Helper: HNSW approximate search (simplified)
    std::vector<std::pair<size_t, Scalar>> hnsw_search(
        const Eigen::VectorXd& query, int k) const;
    
    // Helper: IVF-PQ approximate search (simplified)
    std::vector<std::pair<size_t, Scalar>> ivf_pq_search(
        const Eigen::VectorXd& query, int k) const;
    
    // Helper: cosine similarity
    Scalar cosine_similarity(const Eigen::VectorXd& a, const Eigen::VectorXd& b) const;
    
    // Helper: validate item before adding
    bool validate_item(const MemoryItem& item) const;
    
    // Helper: update index after tier modification
    void rebuild_hot_index();
    void rebuild_warm_index();
    void rebuild_cold_index();
    
    // Helper: evict item from tier (LRU-like)
    std::optional<MemoryItem> evict_from_hot();
    std::optional<MemoryItem> evict_from_warm();
    
    // Helper: record access for promotion scoring
    void record_access(MemoryItem& item);
};

} // namespace hab
