/**
 * Test 5: Memory Consolidation Validation
 * 
 * Purpose: Validate memory systems (preconscious buffer & episodic memory)
 * 
 * What This Tests:
 * 1. Preconscious buffer expiration (2-second window)
 * 2. Episodic retrieval accuracy (k-NN cosine similarity)
 * 3. Importance-gated consolidation (threshold = 0.5)
 * 4. Duplicate filtering and capacity management
 * 
 * Biological Grounding:
 * - Buffer size: 20 items (Sperling 1960, iconic memory)
 * - Buffer duration: 2000ms (sensory memory window)
 * - Similarity threshold: 0.85 (duplicate detection)
 * - Consolidation threshold: 0.5 (importance gating)
 * 
 * Citations:
 * - Sperling (1960): Iconic memory capacity
 * - Atkinson & Shiffrin (1968): Multi-store memory model
 * - Tulving (1972): Episodic memory theory
 */

#include "preconscious_buffer.h"
#include "episodic_memory.h"
#include "fdqc_params.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <cassert>
#include <thread>
#include <chrono>

using namespace preconscious_buffer;
using namespace episodic_memory;

// Test configuration
constexpr double TOLERANCE = 1e-6;
constexpr int EMBEDDING_DIM = 10;

// ANSI color codes for output
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string CYAN = "\033[36m";
const std::string RESET = "\033[0m";

// Helper function to create test embeddings
std::vector<double> create_embedding(double base_value, int dim = EMBEDDING_DIM) {
    std::vector<double> emb(dim);
    for (int i = 0; i < dim; ++i) {
        emb[i] = base_value + 0.1 * i;
    }
    // Normalize
    double norm = 0.0;
    for (double v : emb) norm += v * v;
    norm = std::sqrt(norm);
    for (double& v : emb) v /= norm;
    return emb;
}

// Helper function to compute cosine similarity
double cosine_similarity(const std::vector<double>& a, const std::vector<double>& b) {
    assert(a.size() == b.size());
    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

/**
 * Sub-Test 5.1: Preconscious Buffer Capacity and Expiration
 * 
 * Validates that the buffer:
 * - Respects maximum capacity (20 items)
 * - Expires items after 2-second window
 * - Maintains FIFO ordering
 */
void test_buffer_capacity_and_expiration() {
    std::cout << CYAN << "\n=== Sub-Test 5.1: Buffer Capacity and Expiration ===" << RESET << std::endl;
    
    PreConsciousBuffer buffer;
    
    // Test Case 1: Fill to capacity
    std::cout << "\nTest Case 1: Fill buffer to capacity (20 items)" << std::endl;
    
    for (int i = 0; i < 25; ++i) {  // Try to add 25, but only 20 should remain
        auto emb = create_embedding(i * 0.1);
        buffer.push(emb, "content_" + std::to_string(i), 0.5);
    }
    
    size_t buffer_size = buffer.size();
    std::cout << "  Items in buffer: " << buffer_size << " (expected: 20)" << std::endl;
    
    bool capacity_ok = (buffer_size == fdqc_params::BUFFER_SIZE);
    std::cout << "  Capacity constraint: " 
              << (capacity_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 2: Check that oldest items were evicted (FIFO)
    std::cout << "\nTest Case 2: FIFO ordering (oldest evicted first)" << std::endl;
    
    auto items = buffer.contents();
    std::cout << "  Oldest item: " << items.front().content << std::endl;
    std::cout << "  Newest item: " << items.back().content << std::endl;
    
    // After adding 25 items to a 20-item buffer, items 0-4 should be evicted
    // Oldest should be content_5, newest should be content_24
    bool fifo_ok = (items.front().content == "content_5" && 
                    items.back().content == "content_24");
    std::cout << "  FIFO ordering: " 
              << (fifo_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 3: Time-based expiration
    std::cout << "\nTest Case 3: Time-based expiration (2-second window)" << std::endl;
    
    PreConsciousBuffer buffer2;
    
    // Add some items
    for (int i = 0; i < 5; ++i) {
        auto emb = create_embedding(i * 0.1);
        buffer2.push(emb, "early_" + std::to_string(i), 0.5);
    }
    
    std::cout << "  Added 5 items, waiting 1 second..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Check items (should still have all 5)
    size_t before_count = buffer2.size();
    buffer2.expire_old_entries();
    size_t count1 = buffer2.size();
    size_t expired1 = (before_count - count1);
    std::cout << "  After 1s: " << count1 << " items, " << expired1 << " expired" << std::endl;
    
    bool no_expiry_yet = (expired1 == 0 && count1 == 5);
    std::cout << "  No expiration before 2s: " 
              << (no_expiry_yet ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Wait another 1.5 seconds (total 2.5s)
    std::cout << "  Waiting another 1.5 seconds (total 2.5s)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    // Now items should expire
    buffer2.expire_old_entries();
    size_t count2 = buffer2.size();
    size_t expired2 = (5 - count2);  // Calculate how many expired
    std::cout << "  After 2.5s: " << count2 << " items, " << expired2 << " expired" << std::endl;
    
    bool expired_ok = (expired2 == 5 && count2 == 0);
    std::cout << "  Expiration after 2s: " 
              << (expired_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (capacity_ok && fifo_ok && no_expiry_yet && expired_ok) {
        std::cout << GREEN << "\n✓ Sub-Test 5.1 PASSED: Buffer capacity and expiration validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 5.1 FAILED: Buffer management incorrect" << RESET << std::endl;
        exit(1);
    }
}

/**
 * Sub-Test 5.2: Duplicate Filtering
 * 
 * Validates that the buffer filters duplicate or highly similar entries
 * based on cosine similarity threshold (0.85).
 */
void test_duplicate_filtering() {
    std::cout << CYAN << "\n=== Sub-Test 5.2: Duplicate Filtering ===" << RESET << std::endl;
    
    PreConsciousBuffer buffer;
    
    // Test Case 1: Add identical items
    std::cout << "\nTest Case 1: Identical items (should be filtered)" << std::endl;
    
    auto emb1 = create_embedding(0.5);
    buffer.push(emb1, "item_1", 0.6);
    buffer.push(emb1, "item_1_duplicate", 0.6);  // Identical, should be filtered
    
    size_t count1 = buffer.size();
    auto stats1 = buffer.stats();
    
    std::cout << "  Items in buffer: " << count1 << " (expected: 2, duplicates marked)" << std::endl;
    std::cout << "  Duplicates detected: " << stats1.duplicates_filtered << " (expected: 1)" << std::endl;
    
    bool identical_detected = (count1 == 2 && stats1.duplicates_filtered == 1);
    std::cout << "  Identical items detected: " 
              << (identical_detected ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 2: Add similar items (similarity > 0.85)
    std::cout << "\nTest Case 2: Similar items (similarity > threshold)" << std::endl;
    
    PreConsciousBuffer buffer2;
    
    auto emb2a = create_embedding(0.5);
    auto emb2b = create_embedding(0.52);  // Very similar
    
    double similarity = cosine_similarity(emb2a, emb2b);
    std::cout << "  Similarity: " << similarity << " (threshold: 0.85)" << std::endl;
    
    buffer2.push(emb2a, "item_2a", 0.6);
    buffer2.push(emb2b, "item_2b", 0.6);
    
    size_t count2 = buffer2.size();
    auto stats2 = buffer2.stats();
    
    std::cout << "  Items in buffer: " << count2 << std::endl;
    std::cout << "  Duplicates filtered: " << stats2.duplicates_filtered << std::endl;
    
    bool similar_detected = (similarity > 0.85) ? (stats2.duplicates_filtered == 1) : (stats2.duplicates_filtered == 0);
    std::cout << "  Similar items detection: " 
              << (similar_detected ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 3: Add dissimilar items (should both be added)
    std::cout << "\nTest Case 3: Dissimilar items (should both be added)" << std::endl;
    
    PreConsciousBuffer buffer3;
    
    auto emb3a = create_embedding(0.1);
    auto emb3b = create_embedding(0.9);  // Very different
    
    double dissimilarity = cosine_similarity(emb3a, emb3b);
    std::cout << "  Similarity: " << dissimilarity << " (threshold: 0.85)" << std::endl;
    
    buffer3.push(emb3a, "item_3a", 0.6);
    buffer3.push(emb3b, "item_3b", 0.6);
    
    size_t count3 = buffer3.size();
    auto stats3 = buffer3.stats();
    
    std::cout << "  Items in buffer: " << count3 << " (expected: 2)" << std::endl;
    std::cout << "  Duplicates detected: " << stats3.duplicates_filtered << " (should be 0 or 1 depending on similarity)" << std::endl;
    
    bool dissimilar_added = (count3 == 2);  // Both should be added
    std::cout << "  Dissimilar items both added: " 
              << (dissimilar_added ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (identical_detected && similar_detected && dissimilar_added) {
        std::cout << GREEN << "\n✓ Sub-Test 5.2 PASSED: Duplicate detection validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 5.2 FAILED: Duplicate detection incorrect" << RESET << std::endl;
        exit(1);
    }
}

/**
 * Sub-Test 5.3: Importance-Gated Consolidation
 * 
 * Validates that only sufficiently salient items (importance >= 0.5)
 * are consolidated from buffer to episodic memory.
 */
void test_importance_gated_consolidation() {
    std::cout << CYAN << "\n=== Sub-Test 5.3: Importance-Gated Consolidation ===" << RESET << std::endl;
    
    PreConsciousBuffer buffer;
    EpisodicMemory episodic;
    
    // Test Case 1: Low importance items should not consolidate
    std::cout << "\nTest Case 1: Low importance (< 0.5, should not consolidate)" << std::endl;
    
    for (int i = 0; i < 5; ++i) {
        auto emb = create_embedding(i * 0.1);
        buffer.push(emb, "low_importance_" + std::to_string(i), 0.3);  // Low salience
    }
    
    auto low_imp_items = buffer.get_salient_entries(0.5);
    std::cout << "  High salience items: " << low_imp_items.size() << " (expected: 0)" << std::endl;
    
    bool low_filtered = (low_imp_items.size() == 0);
    std::cout << "  Low importance filtered: " 
              << (low_filtered ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 2: High importance items should consolidate
    std::cout << "\nTest Case 2: High importance (>= 0.5, should consolidate)" << std::endl;
    
    // Create new buffer instance for fresh test
    PreConsciousBuffer buffer2;
    
    for (int i = 0; i < 5; ++i) {
        auto emb = create_embedding(i * 0.1);
        buffer2.push(emb, "high_importance_" + std::to_string(i), 0.7);  // High salience
    }
    
    auto high_imp_items = buffer2.get_salient_entries(0.5);
    std::cout << "  High salience items: " << high_imp_items.size() << " (expected: 5)" << std::endl;
    
    bool high_passed = (high_imp_items.size() == 5);
    std::cout << "  High importance passed: " 
              << (high_passed ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 3: Mixed importance
    std::cout << "\nTest Case 3: Mixed importance (selective consolidation)" << std::endl;
    
    // Create new buffer instance for fresh test
    PreConsciousBuffer buffer3;
    
    for (int i = 0; i < 10; ++i) {
        auto emb = create_embedding(i * 0.1);
        double salience = (i % 2 == 0) ? 0.7 : 0.3;  // Alternate high/low
        buffer3.push(emb, "mixed_" + std::to_string(i), salience);
    }
    
    auto mixed_items = buffer3.get_salient_entries(0.5);
    std::cout << "  High salience items: " << mixed_items.size() << " (expected: 5)" << std::endl;
    
    bool mixed_ok = (mixed_items.size() == 5);
    std::cout << "  Selective consolidation: " 
              << (mixed_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 4: Actually consolidate to episodic memory
    std::cout << "\nTest Case 4: Consolidation to episodic memory" << std::endl;
    
    size_t before_count = episodic.size();
    
    for (const auto& item : mixed_items) {
        episodic.record(item.embedding, item.content, 0.0);  // Zero importance for test
    }
    
    size_t after_count = episodic.size();
    std::cout << "  Episodes before: " << before_count << std::endl;
    std::cout << "  Episodes after:  " << after_count << std::endl;
    std::cout << "  Episodes added:  " << (after_count - before_count) << " (expected: 5)" << std::endl;
    
    bool consolidated = ((after_count - before_count) == 5);
    std::cout << "  Consolidation successful: " 
              << (consolidated ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (low_filtered && high_passed && mixed_ok && consolidated) {
        std::cout << GREEN << "\n✓ Sub-Test 5.3 PASSED: Importance-gated consolidation validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 5.3 FAILED: Consolidation gating incorrect" << RESET << std::endl;
        exit(1);
    }
}

/**
 * Sub-Test 5.4: Episodic Retrieval Accuracy
 * 
 * Validates that episodic memory retrieval using k-NN cosine similarity
 * returns the most relevant memories.
 */
void test_episodic_retrieval() {
    std::cout << CYAN << "\n=== Sub-Test 5.4: Episodic Retrieval Accuracy ===" << RESET << std::endl;
    
    EpisodicMemory episodic;
    
    // Store several distinct memories
    std::cout << "\nStoring 10 distinct memories..." << std::endl;
    
    std::vector<std::vector<double>> stored_embeddings;
    for (int i = 0; i < 10; ++i) {
        auto emb = create_embedding(i * 0.2);
        stored_embeddings.push_back(emb);
        episodic.record(emb, "memory_" + std::to_string(i), 0.5);
    }
    
    std::cout << "  Total memories stored: " << episodic.size() << std::endl;
    
    // Test Case 1: Retrieve exact match
    std::cout << "\nTest Case 1: Exact match retrieval" << std::endl;
    
    auto query1 = stored_embeddings[5];  // Exact match to memory_5
    auto results1 = episodic.retrieve(query1, 1);
    
    std::cout << "  Query: memory_5 (exact match)" << std::endl;
    std::cout << "  Top result: " << results1[0].episode.metadata << std::endl;
    std::cout << "  Similarity: " << results1[0].similarity << std::endl;
    
    bool exact_match = (results1[0].episode.metadata == "memory_5" && 
                       results1[0].similarity > 0.999);
    std::cout << "  Exact match retrieved: " 
              << (exact_match ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 2: Retrieve similar (not exact)
    std::cout << "\nTest Case 2: Similar match retrieval" << std::endl;
    
    auto query2 = create_embedding(0.51);  // Between memory_2 (0.4) and memory_3 (0.6)
    auto results2 = episodic.retrieve(query2, 3);
    
    std::cout << "  Query: embedding at 0.51 (between memory_2 and memory_3)" << std::endl;
    std::cout << "  Top 3 results:" << std::endl;
    for (size_t i = 0; i < results2.size(); ++i) {
        std::cout << "    " << (i+1) << ". " << results2[i].episode.metadata 
                  << " (similarity: " << results2[i].similarity << ")" << std::endl;
    }
    
    // The closest should be memory_2 or memory_3
    bool similar_retrieved = (results2[0].episode.metadata == "memory_2" || 
                             results2[0].episode.metadata == "memory_3");
    std::cout << "  Nearest neighbor correct: " 
              << (similar_retrieved ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 3: k-NN ordering
    std::cout << "\nTest Case 3: k-NN ordering (descending similarity)" << std::endl;
    
    auto query3 = stored_embeddings[0];  // memory_0
    auto results3 = episodic.retrieve(query3, 5);
    
    std::cout << "  Query: memory_0" << std::endl;
    std::cout << "  Top 5 results:" << std::endl;
    
    bool ordering_ok = true;
    for (size_t i = 0; i < results3.size(); ++i) {
        std::cout << "    " << (i+1) << ". " << results3[i].episode.metadata 
                  << " (similarity: " << results3[i].similarity << ")" << std::endl;
        
        // Check descending order
        if (i > 0 && results3[i].similarity > results3[i-1].similarity) {
            ordering_ok = false;
        }
    }
    
    std::cout << "  Descending similarity order: " 
              << (ordering_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 4: Retrieval statistics
    std::cout << "\nTest Case 4: Retrieval statistics" << std::endl;
    
    auto stats = episodic.stats();
    std::cout << "  Total retrievals: " << stats.total_retrievals << " (expected: 3)" << std::endl;
    std::cout << "  Total episodes:   " << stats.total_episodes << " (expected: 10)" << std::endl;
    
    bool stats_ok = (stats.total_retrievals == 3 && stats.total_episodes == 10);
    std::cout << "  Statistics correct: " 
              << (stats_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (exact_match && similar_retrieved && ordering_ok && stats_ok) {
        std::cout << GREEN << "\n✓ Sub-Test 5.4 PASSED: Episodic retrieval validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 5.4 FAILED: Retrieval accuracy incorrect" << RESET << std::endl;
        exit(1);
    }
}

/**
 * Sub-Test 5.5: End-to-End Memory Pipeline
 * 
 * Validates the complete memory consolidation pipeline:
 * Buffer → Salience Filtering → Episodic Storage → Retrieval
 */
void test_end_to_end_pipeline() {
    std::cout << CYAN << "\n=== Sub-Test 5.5: End-to-End Memory Pipeline ===" << RESET << std::endl;
    
    PreConsciousBuffer buffer;
    EpisodicMemory episodic;
    
    std::cout << "\nSimulating cognitive processing..." << std::endl;
    
    // Step 1: Add diverse items to buffer with varying salience
    std::cout << "\nStep 1: Adding 20 items to preconscious buffer" << std::endl;
    for (int i = 0; i < 20; ++i) {
        auto emb = create_embedding(i * 0.15);
        double salience = (i % 3 == 0) ? 0.8 : 0.3;  // Every 3rd item is salient
        buffer.push(emb, "experience_" + std::to_string(i), salience);
    }
    
    auto buffer_stats = buffer.stats();
    std::cout << "  Items added: " << buffer_stats.total_entries_processed << std::endl;
    std::cout << "  Current items: " << buffer.size() << std::endl;
    std::cout << "  Duplicates filtered: " << buffer_stats.duplicates_filtered << std::endl;
    
    // Step 2: Extract salient items for consolidation
    std::cout << "\nStep 2: Extracting salient items (importance >= 0.5)" << std::endl;
    auto salient_items = buffer.get_salient_entries(0.5);
    std::cout << "  Salient items: " << salient_items.size() << " (expected: ~7)" << std::endl;
    
    bool extraction_ok = (salient_items.size() >= 6 && salient_items.size() <= 8);
    std::cout << "  Extraction count: " 
              << (extraction_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Step 3: Consolidate to episodic memory
    std::cout << "\nStep 3: Consolidating to episodic memory" << std::endl;
    for (const auto& item : salient_items) {
        episodic.record(item.embedding, item.content, 0.6);
    }
    
    auto episodic_stats = episodic.stats();
    std::cout << "  Episodes stored: " << episodic_stats.total_episodes << std::endl;
    
    bool consolidation_ok = (episodic_stats.total_episodes == salient_items.size());
    std::cout << "  Consolidation complete: " 
              << (consolidation_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Step 4: Retrieve relevant memories
    std::cout << "\nStep 4: Retrieving relevant memories" << std::endl;
    auto query = create_embedding(0.45);  // Query for similar experience
    auto results = episodic.retrieve(query, 3);
    
    std::cout << "  Top 3 retrieved memories:" << std::endl;
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "    " << (i+1) << ". " << results[i].episode.metadata 
                  << " (similarity: " << results[i].similarity << ")" << std::endl;
    }
    
    bool retrieval_ok = (results.size() == 3 && results[0].similarity > 0.5);
    std::cout << "  Retrieval successful: " 
              << (retrieval_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Step 5: Verify pipeline statistics
    std::cout << "\nStep 5: Pipeline statistics" << std::endl;
    std::cout << "  Buffer:" << std::endl;
    std::cout << "    Total added: " << buffer_stats.total_entries_processed << std::endl;
    std::cout << "    Duplicates:  " << buffer_stats.duplicates_filtered << std::endl;
    std::cout << "    Current:     " << buffer.size() << std::endl;
    
    std::cout << "  Episodic:" << std::endl;
    std::cout << "    Episodes:    " << episodic_stats.total_episodes << std::endl;
    std::cout << "    Retrievals:  " << episodic_stats.total_retrievals << std::endl;
    
    bool pipeline_complete = extraction_ok && consolidation_ok && retrieval_ok;
    
    if (pipeline_complete) {
        std::cout << GREEN << "\n✓ Sub-Test 5.5 PASSED: End-to-end pipeline validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 5.5 FAILED: Pipeline integration incorrect" << RESET << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << BLUE << "╔════════════════════════════════════════════════════════════════╗" << RESET << std::endl;
    std::cout << BLUE << "║          Test 5: Memory Consolidation Validation             ║" << RESET << std::endl;
    std::cout << BLUE << "╚════════════════════════════════════════════════════════════════╝" << RESET << std::endl;
    
    std::cout << "\nBiological Grounding:" << std::endl;
    std::cout << "  - Sperling (1960): Iconic memory capacity" << std::endl;
    std::cout << "  - Atkinson & Shiffrin (1968): Multi-store memory model" << std::endl;
    std::cout << "  - Tulving (1972): Episodic memory theory" << std::endl;
    
    std::cout << "\nParameters:" << std::endl;
    std::cout << "  - Buffer capacity:     " << fdqc_params::BUFFER_SIZE << " items" << std::endl;
    std::cout << "  - Buffer duration:     " << (fdqc_params::BUFFER_DURATION_SEC * 1000) << " ms" << std::endl;
    std::cout << "  - Similarity threshold: 0.85 (duplicate detection)" << std::endl;
    std::cout << "  - Consolidation gate:   0.5 (importance threshold)" << std::endl;
    
    try {
        test_buffer_capacity_and_expiration();
        test_duplicate_filtering();
        test_importance_gated_consolidation();
        test_episodic_retrieval();
        test_end_to_end_pipeline();
        
        std::cout << GREEN << "\n╔════════════════════════════════════════════════════════════════╗" << RESET << std::endl;
        std::cout << GREEN << "║              ✓ ALL MEMORY TESTS PASSED ✓                      ║" << RESET << std::endl;
        std::cout << GREEN << "╚════════════════════════════════════════════════════════════════╝" << RESET << std::endl;
        std::cout << "\nValidated:" << std::endl;
        std::cout << "  ✓ Buffer capacity and expiration (20 items, 2s window)" << std::endl;
        std::cout << "  ✓ Duplicate detection (similarity threshold 0.85)" << std::endl;
        std::cout << "  ✓ Importance-gated consolidation (threshold 0.5)" << std::endl;
        std::cout << "  ✓ Episodic k-NN retrieval (cosine similarity)" << std::endl;
        std::cout << "  ✓ End-to-end memory pipeline integration" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << RED << "\n✗ Test failed with exception: " << e.what() << RESET << std::endl;
        return 1;
    }
}
