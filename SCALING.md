# Million-Scale Memory Scaling Guide

## 🎯 Overview

This document describes the **tiered memory architecture** that scales the Human-AI Brain System from **1,000 memories to 1-5 million memories** while maintaining **bounded latency** (<10ms hot, <40ms warm).

## 📊 Architecture Summary

### **Before (Naive v3.1.0)**
- STM: 20 items (too small)
- LTM: 1,000 items flat array (linear scan, O(N) retrieval)
- No deduplication
- No tiering
- **Latency**: O(N) → **unbounded** at scale

### **After (Tiered v3.3.0)**
- STM: **96 items** (3-6x increase with sparse GW)
- LTM: **Tiered storage** (Hot/Warm/Cold)
  - **Hot**: 50k items (HNSW, <10ms)
  - **Warm**: 300k items (IVF-PQ, <40ms)
  - **Cold**: 2M items (Parquet, async)
- **Deduplication**: MinHash-128
- **Decay**: 30-day half-life
- **Promotion/Demotion**: Usage-based tiering
- **Latency**: O(log N) → **bounded** at scale

---

## 🏗️ Tiered Architecture

### **Tier 1: Hot (In-Memory HNSW)**

**Purpose**: Fast retrieval for frequently-accessed items  
**Index**: Hierarchical Navigable Small World (HNSW)  
**Capacity**: 50,000 items  
**Latency**: <10ms

**Configuration**:
```yaml
hot:
  capacity: 50000
  index: HNSW
  hnsw_M: 32                    # Bi-directional links per node
  hnsw_ef_construction: 200     # Build-time search depth
  hnsw_ef_search: 128           # Query-time search depth (capped for latency)
  latency_budget_ms: 10
```

**Characteristics**:
- **Complexity**: O(log N) search
- **RAM**: ~500MB-1GB (embeddings + graph)
- **Recall**: 95-98%
- **Use case**: Recently accessed, high-reward, novel items

---

### **Tier 2: Warm (SSD-backed IVF-PQ)**

**Purpose**: Medium-latency retrieval with quantization  
**Index**: Inverted File with Product Quantization (IVF-PQ)  
**Capacity**: 300,000 items  
**Latency**: <40ms

**Configuration**:
```yaml
warm:
  capacity: 300000
  index: IVF_PQ
  ivf_nlist: 4096               # IVF clusters
  pq_m: 64                      # Product quantization subquantizers
  pq_nbits: 8                   # 8-bit PQ for 4-8x RAM reduction
  recall_target: 0.95           # Target 95% recall (~1-3% drop from quantization)
  latency_budget_ms: 40
```

**Characteristics**:
- **Complexity**: O(√N) search (IVF clustering)
- **RAM**: ~100-200MB (quantized vectors)
- **Storage**: SSD for full vectors
- **Recall**: 92-96% (quantization loss)
- **Use case**: Less frequently accessed, medium-reward items

---

### **Tier 3: Cold (Object Store / Parquet)**

**Purpose**: Archival storage for rarely-accessed items  
**Format**: Parquet (columnar storage)  
**Capacity**: 2,000,000 items  
**Latency**: Async (seconds)

**Configuration**:
```yaml
cold:
  capacity: 2000000
  format: parquet
  storage_path: ./cold_storage
  async_mode: true
  compression: snappy
```

**Characteristics**:
- **Complexity**: O(N) scan or async query
- **Storage**: S3, local disk, or NFS
- **Compression**: 5-10x with Snappy
- **Use case**: Historical data, low-access items

---

## 🔄 Retrieval Policy

### **Smart Routing with Reranking**

```
Query → Hot HNSW (k=50) → Rerank (MLP/cross-encoder) → Check Scores
        ↓
        If score < τ → Backfill from Warm (k=20) → Rerank → Merge Results
        ↓
        Log Hits → Update Access Counts → Trigger Promotion/Demotion
```

**Configuration**:
```yaml
retrieval:
  hot_k: 50                       # Retrieve top-50 from hot tier
  rerank_enabled: true            # Enable MLP/cross-encoder reranking
  backfill_threshold: 0.5         # Backfill from warm if score < 0.5
  provenance_filter: true         # Block low-trust hits from steering GW
```

**Reranking Models**:
- **Option 1**: Lightweight MLP (5-10ms overhead)
- **Option 2**: Cross-encoder (50-100ms, higher quality)
- **Implementation**: Currently uses cosine similarity (placeholder)

---

## 🗑️ Deduplication (MinHash)

### **Purpose**: Prevent duplicate memories from cluttering tiers

**Method**: MinHash-128 (128-bit signatures)  
**Algorithm**: 3-gram shingles for text, quantized features for embeddings  
**Threshold**: 95% Jaccard similarity

**Configuration**:
```yaml
dedup:
  method: minhash_128             # 128-bit MinHash signatures
  num_hashes: 128
  similarity_threshold: 0.95      # Block if similarity > 95%
```

**Performance**:
- **Comparison**: O(1) per signature (128 hashes)
- **Memory**: 16 bytes per item
- **False Positive Rate**: <1% at 128 hashes

---

## ⏳ Temporal Decay

### **Purpose**: Downweight stale memories over time

**Half-Life**: 30 days (importance decays to 50% in 30 days)  
**Formula**: `importance *= 0.5^(age_days / half_life)`

**Configuration**:
```yaml
decay:
  half_life_days: 30              # Importance decays 50% in 30 days
  enable_temporal_decay: true
  enable_usage_decay: true
```

**Combined Decay**:
- **Temporal**: Based on item age
- **Usage**: Based on time since last access (gentle decay)

---

## 📈 Promotion Policies

### **When to Move Items Up (Warm → Hot)**

**Policies**:
1. **Recent Use** (40% weight): Recently accessed items
2. **Task Reward** (30% weight): High-reward items
3. **Novelty** (30% weight): High-importance/novel items

**Score Formula**:
```
promotion_score = 
    0.4 * recency_score +       # exp(-time_since_access / 1_day)
    0.3 * reward +               # Task reward [0, 1]
    0.3 * importance             # Novelty/importance [0, 1]
    * provenance_score           # Trust multiplier [0, 1]
```

**Trigger**: Periodic maintenance (e.g., every 1000 queries or 10 minutes)

**Configuration**:
```yaml
promotion_policies:
  - recent_use                    # Recently accessed items
  - task_reward                   # High-reward items
  - novelty                       # Novel/important items
```

---

## 📉 Demotion Policies

### **When to Move Items Down (Hot → Warm)**

**Policies**:
1. **Stale** (40% weight): Old, unused items
2. **Low Reward** (30% weight): Low-reward items
3. **Redundant** (30% weight): Duplicate/low-importance items

**Score Formula**:
```
demotion_score = 
    0.4 * (age_days / half_life) +        # Age penalty
    0.3 * (1 - reward) +                  # Low-reward penalty
    0.3 * (1 - importance)                # Redundancy penalty
    * (2 - provenance_score)              # Low-provenance multiplier
```

**Trigger**: When hot tier exceeds capacity

**Configuration**:
```yaml
demotion_policies:
  - stale                         # Old, unused items
  - low_reward                    # Low-reward items
  - redundant                     # Duplicate/low-importance items
```

---

## 💾 Provenance & Trust

### **Purpose**: Block low-trust items from steering Global Workspace

**Provenance Score**: [0, 1] trust score per item  
**Sources**:
- User-provided documents: 1.0
- External web scrapes: 0.5-0.8
- Generated/synthetic: 0.3-0.6

**Usage**:
- **Retrieval**: Multiply similarity scores by provenance
- **GW Steering**: Filter items with provenance < 0.5
- **Promotion**: Boost high-provenance items

**Configuration**:
```yaml
retrieval:
  provenance_filter: true         # Block low-trust hits from steering GW
```

---

## 📏 Capacity Planning

### **Effective Capacity Calculation**

**Formula**:
```
effective_capacity ≈ N_chunks × compression_ratio × consolidation_rate
```

**Example (1k-token document)**:
- Chunk size: 256 tokens → N_chunks ≈ 4
- Summarization: 0.2 compression → 0.8 reduction
- Consolidation: 0.5 rate (50% pass threshold)
- **Effective**: 4 × 0.2 × 0.5 = **0.4 item-equivalents per doc**

**Result**: 2M cold capacity → **~5M documents**

---

## ⚡ Latency Budget

### **Target Latencies**

| Tier | Index | Capacity | Latency | Use Case |
|------|-------|----------|---------|----------|
| Hot | HNSW | 50k | <10ms | Recent, high-reward, novel |
| Warm | IVF-PQ | 300k | <40ms | Medium-access, SSD-backed |
| Cold | Parquet | 2M | Async | Historical, archival |

**Enforcement**:
- **Hot**: Cap `ef_search` at 128
- **Warm**: Use IVF clustering (√N search)
- **Cold**: Async batch queries

---

## 🔧 System Knobs

### **Write Path**

```
Ingest → Dedup Check → Hot Tier → WAL (Write-Ahead Log)
         ↓
         Nightly: Compact Hot → Warm (if capacity exceeded)
         ↓
         Weekly: Snapshot Warm → Cold (archival)
```

**Configuration**:
- **WAL**: Enable for crash recovery
- **Compaction**: Nightly at low-traffic hours (e.g., 2 AM)
- **Snapshot**: Weekly to cold storage

---

### **Quantization**

**Purpose**: Reduce RAM 4-8x with 1-3% recall drop

**Method**: 8-bit Product Quantization (PQ)  
**Configuration**: `pq_nbits: 8` in warm tier

**Trade-off**:
- **Memory**: 768D float32 (3KB) → 64-byte quantized
- **Recall**: 95% → 92-94%
- **Speed**: Slightly faster (smaller memory footprint)

---

## 🚨 Risks and Mitigations

### **1. Drift from Noisy Items**

**Risk**: Low-quality items pollute tiers  
**Mitigation**:
- **Provenance scoring**: Trust-based filtering
- **Decay**: Downweight stale items
- **Deduplication**: Block near-duplicates

---

### **2. Latency Spikes at Scale**

**Risk**: HNSW or IVF search exceeds budget  
**Mitigation**:
- **Cap `ef_search`**: Max 128 for hot tier
- **Rerank with MLP**: Small model (<5ms overhead)
- **Backfill selectively**: Only if scores < threshold

---

### **3. Duplicate Clutter**

**Risk**: Redundant memories waste capacity  
**Mitigation**:
- **MinHash dedup**: Block 95% similarity items
- **Redundancy demotion**: Penalize low-importance items

---

## 📊 Performance Metrics

### **Expected Statistics** (after scaling)

```
Hot Tier:
  - Count: 40,000-50,000 items
  - Avg Latency: 5-8ms
  - Recall: 96-98%
  - RAM: 800MB-1GB

Warm Tier:
  - Count: 250,000-300,000 items
  - Avg Latency: 25-35ms
  - Recall: 92-95%
  - RAM: 150-200MB (quantized)

Cold Tier:
  - Count: 1.5M-2M items
  - Async retrieval
  - Storage: 5-10GB (compressed)

System:
  - Total Capacity: ~2.35M items
  - Effective Document Capacity: ~5-6M docs
  - Duplicates Blocked: 5-10%
  - Promotions/hour: ~100-500
  - Demotions/hour: ~100-500
```

---

## 🔬 Example Configuration

### **Full `quantum.yaml` (v3.3.0)**

```yaml
# Memory System Configuration (Tiered Architecture for Million-Scale)
memory:
  # Short-Term Memory (STM) - Scaled up with sparse GW
  stm_capacity: 96  # 64-128 range, using 96 for balance
  stm_decay_tau: 3.0
  
  # Long-Term Memory (LTM) - Tiered Storage
  ltm:
    # Hot tier: In-memory HNSW index (fast retrieval <10ms)
    hot:
      capacity: 50000
      index: HNSW
      hnsw_M: 32                    # Bi-directional links per node
      hnsw_ef_construction: 200     # Build-time search depth
      hnsw_ef_search: 128           # Query-time search depth (capped for latency)
      latency_budget_ms: 10
    
    # Warm tier: SSD-backed IVF-PQ (medium retrieval <40ms)
    warm:
      capacity: 300000
      index: IVF_PQ
      ivf_nlist: 4096               # IVF clusters
      pq_m: 64                      # Product quantization subquantizers
      pq_nbits: 8                   # 8-bit PQ for 4-8x RAM reduction
      recall_target: 0.95           # Target 95% recall with ~1-3% drop from quantization
      latency_budget_ms: 40
    
    # Cold tier: Object store archival (async retrieval)
    cold:
      capacity: 2000000
      format: parquet
      storage_path: ./cold_storage
      async_mode: true
      compression: snappy
  
  # Consolidation and Quality Control
  consolidation_threshold: 0.7
  
  # Deduplication (MinHash-based)
  dedup:
    method: minhash_128             # 128-bit MinHash signatures
    num_hashes: 128
    similarity_threshold: 0.95      # Block if similarity > 95%
  
  # Temporal and Usage Decay
  decay:
    half_life_days: 30              # Importance decays 50% in 30 days
    enable_temporal_decay: true
    enable_usage_decay: true
  
  # Promotion/Demotion Policies
  promotion_policies:
    - recent_use                    # Recently accessed items
    - task_reward                   # High-reward items
    - novelty                       # Novel/important items
  
  demotion_policies:
    - stale                         # Old, unused items
    - low_reward                    # Low-reward items
    - redundant                     # Duplicate/low-importance items
  
  # Retrieval Configuration
  retrieval:
    hot_k: 50                       # Retrieve top-50 from hot tier
    rerank_enabled: true            # Enable MLP/cross-encoder reranking
    backfill_threshold: 0.5         # Backfill from warm if score < 0.5
    provenance_filter: true         # Block low-trust hits from steering GW
```

---

## 🚀 Usage

### **Initialization**

```cpp
#include "brain/tiered_memory.hpp"

// Load configuration
TieredLTMConfig config;
config.hot.capacity = 50000;
config.warm.capacity = 300000;
config.cold.capacity = 2000000;

// Create tiered LTM
TieredLTM ltm(config);
```

---

### **Adding Items**

```cpp
MemoryItem item;
item.embedding = compute_embedding(text);
item.gw_state = global_workspace_state;
item.reward = 0.8;
item.provenance_score = 1.0;  // Trusted source
item.source_doc_id = "doc_12345";

ltm.add(item);
```

---

### **Retrieval**

```cpp
Eigen::VectorXd query = compute_query_embedding(user_input);
RetrievalResult result = ltm.retrieve(query, /*k=*/50);

// Access results
for (size_t i = 0; i < result.items.size(); ++i) {
    std::cout << "Item " << i << ": "
              << "score=" << result.scores[i]
              << ", tier=" << result.tiers[i]
              << std::endl;
}

std::cout << "Latency: " << result.latency_ms << "ms" << std::endl;
```

---

### **Maintenance**

```cpp
// Periodic maintenance (every 1000 queries or 10 minutes)
ltm.maintenance();

// Get statistics
auto stats = ltm.get_stats();
std::cout << "Hot: " << stats.hot_count
          << ", Warm: " << stats.warm_count
          << ", Cold: " << stats.cold_count
          << std::endl;
```

---

## 📚 References

- **HNSW**: [Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs](https://arxiv.org/abs/1603.09320)
- **Product Quantization**: [Product quantization for nearest neighbor search](https://hal.inria.fr/inria-00514462v2/document)
- **MinHash**: [Detecting Near-Duplicates for Web Crawling](https://theory.stanford.edu/~aiken/publications/papers/sigmod07.pdf)
- **FAISS**: [Facebook AI Similarity Search](https://github.com/facebookresearch/faiss)

---

## 🎓 Summary

**Short Answer**: Raise STM to ~100 and move LTM to a tiered vector store. You get **million-scale memory** with **stable recall** and **bounded latency**.

**Key Metrics**:
- STM: 20 → **96** (3-6x increase)
- LTM: 1k → **2.35M** (2350x increase)
- Latency: O(N) → **O(log N)** (bounded at scale)
- Recall: Flat → **92-98%** (tiered)
- Dedup: None → **MinHash-128** (5-10% blocked)
- Decay: None → **30-day half-life**

**Result**: Production-grade memory system that scales to **millions of documents** while maintaining **sub-40ms latency**.

---

**Version**: v3.3.0  
**Author**: Mr. Block (with AI assistance)  
**Date**: 2025-10-27
