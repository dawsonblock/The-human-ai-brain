#pragma once

#include "utils.hpp"
#include <Eigen/Dense>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <memory>
#include <array>

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

// Complete tiered LTM configuration
struct TieredLTMConfig {
    HotTierConfig hot;
    WarmTierConfig warm;
    ColdTierConfig cold;
    DedupConfig dedup;
    DecayConfig decay;
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
    int access_count = 0;
    TimePoint last_access;
    std::string source_doc_id;
    std::string tier;                   // "hot", "warm", "cold"
    
    // MinHash signature for deduplication
    std::array<uint64_t, 2> minhash_sig = {0, 0};
    
    MemoryItem() : action(0), reward(0.0),
                   timestamp(std::chrono::steady_clock::now()),
                   last_access(std::chrono::steady_clock::now()),
                   tier("hot") {}
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
    void add(const MemoryItem& item);
    
    // Retrieve top-k items with smart routing
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
        int hot_count = 0;
        int warm_count = 0;
        int cold_count = 0;
        int total_count = 0;
        Scalar avg_hot_latency_ms = 0.0;
        Scalar avg_warm_latency_ms = 0.0;
        int duplicates_blocked = 0;
        int promotions = 0;
        int demotions = 0;
    };
    
    Stats get_stats() const { return stats_; }
    
private:
    TieredLTMConfig config_;
    
    // Hot tier (in-memory HNSW index)
    std::vector<MemoryItem> hot_tier_;
    std::unordered_map<std::string, size_t> hot_index_; // doc_id -> index
    
    // Warm tier (simulated IVF-PQ)
    std::vector<MemoryItem> warm_tier_;
    std::unordered_map<std::string, size_t> warm_index_;
    
    // Cold tier (archival)
    std::vector<MemoryItem> cold_tier_;
    std::unordered_map<std::string, size_t> cold_index_;
    
    // Deduplication
    MinHashDedup dedup_;
    std::vector<std::array<uint64_t, 2>> signatures_;
    
    // Statistics
    Stats stats_;
    
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
};

} // namespace hab
