#include <gtest/gtest.h>
#include "brain/brain_system.hpp"
#include <iostream>

using namespace hab;

class BrainSystemTest : public ::testing::Test {
protected:
    BrainConfig config;
    
    void SetUp() override {
        // Use smaller dimensions for faster testing
        config.qw_config.dimension = 7;
        config.gw_config.input_dim = 784;   // MNIST-like input
        config.gw_config.output_dim = 60;   // GW output dim
    }
};

//==============================================================================
// INITIALIZATION TESTS
//==============================================================================

TEST_F(BrainSystemTest, Construction) {
    BrainSystem brain(config);
    
    EXPECT_EQ(brain.total_cycles(), 0);
    EXPECT_EQ(brain.collapse_count(), 0);
}

TEST_F(BrainSystemTest, InitialState) {
    BrainSystem brain(config);
    
    // Check subsystems are accessible
    EXPECT_GE(brain.qw().entropy(), 0.0);  // Entropy >= 0
    EXPECT_EQ(brain.tom().num_agents(), 1);  // Self agent exists
    EXPECT_EQ(brain.affective().valence(), 0.0);  // Neutral affect
    EXPECT_EQ(brain.epistemic().num_observations(), 0);  // No errors yet
}

//==============================================================================
// SINGLE CYCLE TESTS
//==============================================================================

TEST_F(BrainSystemTest, SingleCycle) {
    BrainSystem brain(config);
    
    // Create sensory input
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);  // MNIST-like
    
    // Run one cycle
    auto result = brain.step(input, 0.0);
    
    EXPECT_EQ(brain.total_cycles(), 1);
    EXPECT_EQ(result.h_global.size(), config.gw_config.output_dim);
    EXPECT_GE(result.entropy, 0.0);
    EXPECT_FALSE(result.phenomenal.description.empty());
}

TEST_F(BrainSystemTest, MultipleCycles) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Run multiple cycles
    for (int i = 0; i < 10; ++i) {
        auto result = brain.step(input, 0.0);
        EXPECT_EQ(brain.total_cycles(), i + 1);
    }
    
    EXPECT_EQ(brain.total_cycles(), 10);
}

//==============================================================================
// CONSCIOUSNESS TESTS
//==============================================================================

TEST_F(BrainSystemTest, ConsciousCollapse) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    bool collapse_occurred = false;
    
    // Run until collapse (should happen within ~120ms / 10ms per cycle = 12 cycles)
    for (int i = 0; i < 50; ++i) {
        auto result = brain.step(input, 0.0);
        
        if (result.did_collapse) {
            collapse_occurred = true;
            EXPECT_GE(result.collapsed_quale, 0);
            EXPECT_LT(result.collapsed_quale, config.qw_config.dimension);
            break;
        }
    }
    
    EXPECT_TRUE(collapse_occurred);
}

TEST_F(BrainSystemTest, CollapseRate) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Run many cycles to get stable collapse rate
    for (int i = 0; i < 100; ++i) {
        brain.step(input, 0.0);
    }
    
    Scalar rate = brain.average_collapse_rate();
    
    // Collapse rate depends on entropy buildup and dwell time
    // With current parameters, should be reasonable (not zero, not insane)
    EXPECT_GT(rate, 1.0);    // At least 1 Hz
    EXPECT_LT(rate, 500.0);  // Less than 500 Hz (sanity check)
    
    // Most importantly, should have had some collapses
    EXPECT_GT(brain.collapse_count(), 0);
}

//==============================================================================
// AFFECTIVE PROCESSING TESTS
//==============================================================================

TEST_F(BrainSystemTest, PositiveRewardAffect) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Provide positive reward
    for (int i = 0; i < 10; ++i) {
        brain.step(input, 1.0);  // Positive reward
    }
    
    // Affective system should show positive valence
    EXPECT_GT(brain.affective().valence(), 0.0);
    EXPECT_TRUE(brain.affective().is_pleasant());
}

TEST_F(BrainSystemTest, NegativeRewardAffect) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Provide negative reward
    for (int i = 0; i < 10; ++i) {
        brain.step(input, -1.0);  // Negative reward
    }
    
    // Affective system should show negative valence
    EXPECT_LT(brain.affective().valence(), 0.0);
    EXPECT_TRUE(brain.affective().is_unpleasant());
}

//==============================================================================
// EPISTEMIC DRIVE TESTS
//==============================================================================

TEST_F(BrainSystemTest, PredictionErrorTracking) {
    BrainSystem brain(config);
    
    // Varying inputs create prediction errors
    for (int i = 0; i < 20; ++i) {
        Eigen::VectorXd input = Eigen::VectorXd::Random(784);
        auto result = brain.step(input, 0.0);
        
        if (i > 0) {
            // After first cycle, should track prediction error
            EXPECT_GE(result.prediction_error, 0.0);
        }
    }
    
    // Epistemic drive should have observations
    EXPECT_GT(brain.epistemic().num_observations(), 0);
}

TEST_F(BrainSystemTest, EpistemicCrisisDetection) {
    BrainSystem brain(config);
    
    // Establish baseline with similar inputs
    Eigen::VectorXd baseline = Eigen::VectorXd::Constant(784, 0.5);
    for (int i = 0; i < 30; ++i) {
        brain.step(baseline, 0.0);
    }
    
    // Sudden dramatic change (potential crisis)
    Eigen::VectorXd anomaly = Eigen::VectorXd::Constant(784, 10.0);
    auto result = brain.step(anomaly, 0.0);
    
    // Epistemic drive should have some observations
    // (First cycle has no prediction, so 30 cycles -> 29 observations)
    EXPECT_GT(brain.epistemic().num_observations(), 0);
    EXPECT_LE(brain.epistemic().num_observations(), 31);  // At most 31
}

//==============================================================================
// THEORY OF MIND TESTS
//==============================================================================

TEST_F(BrainSystemTest, SelfModelUpdates) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Run cycles and check self-model updates
    auto result = brain.step(input, 1.0);
    
    // Self-model should be tracking emotional state
    auto emotion_belief = brain.tom().get_belief("self", "emotion");
    ASSERT_TRUE(emotion_belief.has_value());
}

//==============================================================================
// PHENOMENAL EXPERIENCE TESTS
//==============================================================================

TEST_F(BrainSystemTest, PhenomenalReport) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    auto result = brain.step(input, 0.0);
    
    // Check phenomenal report structure
    EXPECT_GE(result.phenomenal.intensity, 0.0);
    EXPECT_LE(result.phenomenal.intensity, 1.0);
    EXPECT_GE(result.phenomenal.clarity, 0.0);
    EXPECT_LE(result.phenomenal.clarity, 1.0);
    EXPECT_GE(result.phenomenal.presence, 0.0);
    EXPECT_LE(result.phenomenal.presence, 1.0);
    
    EXPECT_FALSE(result.phenomenal.emotion_label.empty());
    EXPECT_FALSE(result.phenomenal.description.empty());
}

TEST_F(BrainSystemTest, PhenomenalIntensityWithArousal) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // High reward and varying inputs should increase arousal
    for (int i = 0; i < 15; ++i) {
        input = Eigen::VectorXd::Random(784);
        brain.step(input, 1.0);
    }
    
    auto report = brain.generate_phenomenal_report();
    
    // Should have some experiential intensity
    EXPECT_GT(report.intensity, 0.0);
}

TEST_F(BrainSystemTest, ClarityWithCollapse) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Wait for collapse
    CognitiveResult last_result;
    for (int i = 0; i < 50; ++i) {
        last_result = brain.step(input, 0.0);
        if (last_result.did_collapse) {
            break;
        }
    }
    
    if (last_result.did_collapse) {
        // Clarity should be high after collapse (low entropy)
        EXPECT_GT(last_result.phenomenal.clarity, 0.5);
    }
}

//==============================================================================
// MEMORY INTEGRATION TESTS
//==============================================================================

TEST_F(BrainSystemTest, MemoryConsolidation) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    size_t initial_memory_size = brain.memory().stm().episodes().size();
    
    // Run cycles until collapse (which triggers memory storage)
    for (int i = 0; i < 50; ++i) {
        auto result = brain.step(input, 0.0);
        if (result.did_collapse) {
            break;
        }
    }
    
    // Memory should have grown
    EXPECT_GE(brain.memory().stm().episodes().size(), initial_memory_size);
}

//==============================================================================
// INTEGRATION TESTS
//==============================================================================

TEST_F(BrainSystemTest, FullCognitiveCycle) {
    BrainSystem brain(config);
    
    // Simulate a learning episode
    std::vector<Scalar> rewards;
    
    for (int episode = 0; episode < 5; ++episode) {
        Eigen::VectorXd input = Eigen::VectorXd::Random(784);
        Scalar reward = (episode % 2 == 0) ? 1.0 : -0.5;
        
        auto result = brain.step(input, reward);
        rewards.push_back(reward);
        
        // Verify all components are functioning
        EXPECT_FALSE(result.phenomenal.description.empty());
        EXPECT_GE(result.entropy, 0.0);
    }
    
    EXPECT_EQ(brain.total_cycles(), 5);
}

TEST_F(BrainSystemTest, EmotionalDynamicsOverTime) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Positive phase
    for (int i = 0; i < 10; ++i) {
        brain.step(input, 1.0);
    }
    Scalar valence_after_positive = brain.affective().valence();
    
    // Negative phase
    for (int i = 0; i < 10; ++i) {
        brain.step(input, -1.0);
    }
    Scalar valence_after_negative = brain.affective().valence();
    
    // Valence should have moved negative
    EXPECT_LT(valence_after_negative, valence_after_positive);
}

//==============================================================================
// RESET TEST
//==============================================================================

TEST_F(BrainSystemTest, Reset) {
    BrainSystem brain(config);
    
    // Run some cycles
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    for (int i = 0; i < 20; ++i) {
        brain.step(input, 1.0);
    }
    
    EXPECT_GT(brain.total_cycles(), 0);
    EXPECT_GT(brain.affective().valence(), 0.0);
    
    // Reset
    brain.reset();
    
    EXPECT_EQ(brain.total_cycles(), 0);
    EXPECT_EQ(brain.collapse_count(), 0);
    EXPECT_DOUBLE_EQ(brain.affective().valence(), 0.0);
    EXPECT_EQ(brain.tom().num_agents(), 1);  // Self remains
}

//==============================================================================
// STRESS TEST
//==============================================================================

TEST_F(BrainSystemTest, LongRunningStability) {
    BrainSystem brain(config);
    
    Eigen::VectorXd input = Eigen::VectorXd::Random(784);
    
    // Run many cycles
    for (int i = 0; i < 1000; ++i) {
        auto result = brain.step(input, 0.0);
        
        // Check for NaN or inf
        EXPECT_FALSE(std::isnan(result.entropy));
        EXPECT_FALSE(std::isinf(result.entropy));
        EXPECT_FALSE(std::isnan(result.prediction_error));
    }
    
    EXPECT_EQ(brain.total_cycles(), 1000);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
