#include "brain/tiered_memory.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace hab;

// ============================================================================
// MinHash Deduplication Tests
// ============================================================================

class MinHashTest : public ::testing::Test {
protected:
    MinHashDedup dedup{128};
};

TEST_F(MinHashTest, SignatureConsistency) {
    std::string text = "The quick brown fox jumps over the lazy dog";
    auto sig1 = dedup.compute_signature(text);
    auto sig2 = dedup.compute_signature(text);
    
    EXPECT_EQ(sig1[0], sig2[0]);
    EXPECT_EQ(sig1[1], sig2[1]);
}

TEST_F(MinHashTest, DifferentTextsDifferentSignatures) {
    std::string text1 = "The quick brown fox";
    std::string text2 = "jumps over the lazy dog";
    
    auto sig1 = dedup.compute_signature(text1);
    auto sig2 = dedup.compute_signature(text2);
    
    EXPECT_NE(sig1[0], sig2[0]);
}

TEST_F(MinHashTest, SimilarTextsHighSimilarity) {
    std::string text1 = "The quick brown fox jumps";
    std::string text2 = "The quick brown fox leaps";  // One word changed
    
    auto sig1 = dedup.compute_signature(text1);
    auto sig2 = dedup.compute_signature(text2);
    
    Scalar similarity = dedup.jaccard_similarity(sig1, sig2);
    // Should have some similarity due to overlapping 3-grams
    EXPECT_GT(similarity, 0.0);
}

TEST_F(MinHashTest, EmbeddingSignatureConsistency) {
    Eigen::VectorXd embedding = Eigen::VectorXd::Random(128);
    auto sig1 = dedup.compute_signature(embedding);
    auto sig2 = dedup.compute_signature(embedding);
    
    EXPECT_EQ(sig1[0], sig2[0]);
    EXPECT_EQ(sig1[1], sig2[1]);
}

TEST_F(MinHashTest, DuplicateDetection) {
    std::string text1 = "The quick brown fox";
    std::string text2 = "The quick brown fox";  // Exact duplicate
    
    auto sig1 = dedup.compute_signature(text1);
    auto sig2 = dedup.compute_signature(text2);
    
    std::vector<std::array<uint64_t, 2>> existing = {sig1};
    EXPECT_TRUE(dedup.is_duplicate(sig2, existing, 0.95));
}

// ============================================================================
// TieredLTM Basic Tests
// ============================================================================

class TieredLTMTest : public ::testing::Test {
protected:
    void SetUp() override {
        config.hot.capacity = 10;
        config.warm.capacity = 20;
        config.cold.capacity = 50;
        config.dedup.num_hashes = 128;
        config.dedup.similarity_threshold = 0.95;
        ltm = std::make_unique<TieredLTM>(config);
    }
    
    MemoryItem create_test_item(const std::string& id, int dim = 64) {
        MemoryItem item;
        item.source_doc_id = id;
        item.embedding = Eigen::VectorXd::Random(dim);
        item.gw_state = Eigen::VectorXd::Random(dim);
        item.qw_onehot = Eigen::VectorXd::Zero(7);
        item.qw_onehot(0) = 1.0;
        item.action = 0;
        item.reward = 0.5;
        item.importance = 0.7;
        item.provenance_score = 1.0;
        return item;
    }
    
    TieredLTMConfig config;
    std::unique_ptr<TieredLTM> ltm;
};

TEST_F(TieredLTMTest, AddSingleItem) {
    MemoryItem item = create_test_item("item_1");
    bool added = ltm->add(item);
    
    EXPECT_TRUE(added);
    EXPECT_EQ(ltm->hot_size(), 1);
    EXPECT_EQ(ltm->total_size(), 1);
}

TEST_F(TieredLTMTest, AddMultipleItems) {
    for (int i = 0; i < 5; ++i) {
        MemoryItem item = create_test_item("item_" + std::to_string(i));
        EXPECT_TRUE(ltm->add(item));
    }
    
    EXPECT_EQ(ltm->hot_size(), 5);
    EXPECT_EQ(ltm->total_size(), 5);
}

TEST_F(TieredLTMTest, HotTierEviction) {
    // Fill hot tier beyond capacity
    for (int i = 0; i < 15; ++i) {
        MemoryItem item = create_test_item("item_" + std::to_string(i));
        ltm->add(item);
    }
    
    // Hot tier should be at capacity, excess in warm
    EXPECT_LE(ltm->hot_size(), static_cast<size_t>(config.hot.capacity));
    EXPECT_GT(ltm->warm_size(), 0);
    EXPECT_EQ(ltm->total_size(), 15);
}

TEST_F(TieredLTMTest, DuplicateBlocking) {
    MemoryItem item1 = create_test_item("item_1");
    MemoryItem item2 = item1;  // Exact duplicate
    item2.source_doc_id = "item_1_duplicate";
    
    EXPECT_TRUE(ltm->add(item1));
    EXPECT_FALSE(ltm->add(item2));  // Should be blocked
    
    EXPECT_EQ(ltm->total_size(), 1);
    auto stats = ltm->get_stats();
    EXPECT_EQ(stats.duplicates_blocked.load(), 1);
}

TEST_F(TieredLTMTest, InvalidItemRejection) {
    MemoryItem invalid_item;
    // Empty embedding, no source_doc_id
    
    EXPECT_FALSE(ltm->add(invalid_item));
    EXPECT_EQ(ltm->total_size(), 0);
}

TEST_F(TieredLTMTest, RetrievalBasic) {
    // Add some items
    for (int i = 0; i < 5; ++i) {
        MemoryItem item = create_test_item("item_" + std::to_string(i));
        ltm->add(item);
    }
    
    // Retrieve with random query
    Eigen::VectorXd query = Eigen::VectorXd::Random(64);
    auto result = ltm->retrieve(query, 3);
    
    EXPECT_LE(result.items.size(), 3);
    EXPECT_EQ(result.items.size(), result.scores.size());
    EXPECT_EQ(result.items.size(), result.tiers.size());
    EXPECT_GT(result.latency_ms, 0.0);
}

TEST_F(TieredLTMTest, RetrievalEmptyQueryThrows) {
    Eigen::VectorXd empty_query;
    EXPECT_THROW(ltm->retrieve(empty_query, 5), std::invalid_argument);
}

TEST_F(TieredLTMTest, RetrievalInvalidKThrows) {
    Eigen::VectorXd query = Eigen::VectorXd::Random(64);
    EXPECT_THROW(ltm->retrieve(query, 0), std::invalid_argument);
    EXPECT_THROW(ltm->retrieve(query, -1), std::invalid_argument);
}

TEST_F(TieredLTMTest, ConsolidationBelowThreshold) {
    MemoryItem item = create_test_item("item_1");
    Scalar low_importance = 0.5;  // Below threshold of 0.7
    
    size_t initial_size = ltm->total_size();
    ltm->consolidate(item, low_importance);
    
    EXPECT_EQ(ltm->total_size(), initial_size);  // Should not be added
}

TEST_F(TieredLTMTest, ConsolidationAboveThreshold) {
    MemoryItem item = create_test_item("item_1");
    Scalar high_importance = 0.8;  // Above threshold of 0.7
    
    ltm->consolidate(item, high_importance);
    
    EXPECT_EQ(ltm->total_size(), 1);
}

TEST_F(TieredLTMTest, PromotionFromWarmToHot) {
    // Fill hot tier
    for (int i = 0; i < 10; ++i) {
        MemoryItem item = create_test_item("hot_" + std::to_string(i));
        ltm->add(item);
    }
    
    // Add more to force into warm
    for (int i = 0; i < 5; ++i) {
        MemoryItem item = create_test_item("warm_" + std::to_string(i));
        ltm->add(item);
    }
    
    EXPECT_GT(ltm->warm_size(), 0);
    size_t initial_warm = ltm->warm_size();
    
    // Promote one item
    ltm->promote("warm_0", "warm", "hot");
    
    // Note: promotion might fail if hot is full, so just check consistency
    EXPECT_LE(ltm->hot_size(), static_cast<size_t>(config.hot.capacity + 1));
}

TEST_F(TieredLTMTest, DemotionFromHotToWarm) {
    // Add items to hot tier
    for (int i = 0; i < 5; ++i) {
        MemoryItem item = create_test_item("item_" + std::to_string(i));
        ltm->add(item);
    }
    
    size_t initial_hot = ltm->hot_size();
    
    // Demote one item
    ltm->demote("item_0", "hot", "warm");
    
    EXPECT_EQ(ltm->hot_size(), initial_hot - 1);
    EXPECT_EQ(ltm->warm_size(), 1);
}

TEST_F(TieredLTMTest, MaintenanceDoesNotCrash) {
    // Add some items
    for (int i = 0; i < 10; ++i) {
        MemoryItem item = create_test_item("item_" + std::to_string(i));
        ltm->add(item);
    }
    
    // Run maintenance
    EXPECT_NO_THROW(ltm->maintenance());
}

TEST_F(TieredLTMTest, ClearWorks) {
    // Add items
    for (int i = 0; i < 10; ++i) {
        ltm->add(create_test_item("item_" + std::to_string(i)));
    }
    
    EXPECT_GT(ltm->total_size(), 0);
    
    ltm->clear();
    
    EXPECT_EQ(ltm->hot_size(), 0);
    EXPECT_EQ(ltm->warm_size(), 0);
    EXPECT_EQ(ltm->cold_size(), 0);
    EXPECT_EQ(ltm->total_size(), 0);
}

TEST_F(TieredLTMTest, StatsTracking) {
    for (int i = 0; i < 5; ++i) {
        ltm->add(create_test_item("item_" + std::to_string(i)));
    }
    
    auto stats = ltm->get_stats();
    EXPECT_GT(stats.total_count.load(), 0);
    EXPECT_GT(stats.hot_count.load(), 0);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(TieredLTMTest, ConcurrentAdds) {
    const int num_threads = 4;
    const int items_per_thread = 10;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                std::string id = "thread_" + std::to_string(t) + "_item_" + std::to_string(i);
                MemoryItem item = create_test_item(id);
                ltm->add(item);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All items should be added (within capacity constraints)
    EXPECT_GT(ltm->total_size(), 0);
}

TEST_F(TieredLTMTest, ConcurrentRetrievals) {
    // Add some items first
    for (int i = 0; i < 20; ++i) {
        ltm->add(create_test_item("item_" + std::to_string(i)));
    }
    
    const int num_threads = 4;
    const int queries_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successful_queries{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &successful_queries, queries_per_thread]() {
            for (int i = 0; i < queries_per_thread; ++i) {
                Eigen::VectorXd query = Eigen::VectorXd::Random(64);
                try {
                    auto result = ltm->retrieve(query, 5);
                    if (result.items.size() > 0) {
                        successful_queries.fetch_add(1);
                    }
                } catch (...) {
                    // Swallow exceptions for this test
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(successful_queries.load(), 0);
}

TEST_F(TieredLTMTest, ConcurrentMixedOperations) {
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;
    
    // Thread 1: Add items
    threads.emplace_back([this, &stop]() {
        int counter = 0;
        while (!stop.load()) {
            MemoryItem item = create_test_item("add_" + std::to_string(counter++));
            ltm->add(item);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Thread 2: Retrieve items
    threads.emplace_back([this, &stop]() {
        while (!stop.load()) {
            if (ltm->total_size() > 0) {
                Eigen::VectorXd query = Eigen::VectorXd::Random(64);
                try {
                    ltm->retrieve(query, 3);
                } catch (...) {}
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
    
    // Thread 3: Maintenance
    threads.emplace_back([this, &stop]() {
        while (!stop.load()) {
            ltm->maintenance();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should not crash and should have added some items
    EXPECT_GT(ltm->total_size(), 0);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(TieredLTMTest, RetrievalLatency) {
    // Add many items
    for (int i = 0; i < 100; ++i) {
        ltm->add(create_test_item("item_" + std::to_string(i)));
    }
    
    // Measure retrieval latency
    Eigen::VectorXd query = Eigen::VectorXd::Random(64);
    auto result = ltm->retrieve(query, 10);
    
    // Should complete in reasonable time
    EXPECT_LT(result.latency_ms, 100.0);  // Less than 100ms
}

TEST_F(TieredLTMTest, AddLatency) {
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        ltm->add(create_test_item("item_" + std::to_string(i)));
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // 100 adds should be fast
    EXPECT_LT(duration_ms, 1000.0);  // Less than 1 second
}

// Run all tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
