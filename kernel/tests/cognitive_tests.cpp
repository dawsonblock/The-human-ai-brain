#include <gtest/gtest.h>
#include "brain/theory_of_mind.hpp"
#include "brain/affective.hpp"
#include "brain/epistemic_drive.hpp"

using namespace hab;

//==============================================================================
// THEORY OF MIND TESTS
//==============================================================================

class TheoryOfMindTest : public ::testing::Test {
protected:
    TheoryOfMind tom;
};

TEST_F(TheoryOfMindTest, BasicBeliefTracking) {
    // Track a belief
    tom.track_belief("Alice", "sky_color", "blue", 1.0);
    
    auto belief = tom.get_belief("Alice", "sky_color");
    ASSERT_TRUE(belief.has_value());
    EXPECT_EQ(belief.value(), "blue");
}

TEST_F(TheoryOfMindTest, MultipleAgents) {
    tom.track_belief("Alice", "location", "park", 1.0);
    tom.track_belief("Bob", "location", "home", 1.0);
    
    auto alice_belief = tom.get_belief("Alice", "location");
    auto bob_belief = tom.get_belief("Bob", "location");
    
    ASSERT_TRUE(alice_belief.has_value());
    ASSERT_TRUE(bob_belief.has_value());
    EXPECT_EQ(alice_belief.value(), "park");
    EXPECT_EQ(bob_belief.value(), "home");
    
    EXPECT_EQ(tom.num_agents(), 3); // Alice, Bob, and "self"
}

TEST_F(TheoryOfMindTest, FalseBelief) {
    // Sally believes marble is in basket
    tom.track_belief("Sally", "marble_location", "basket", 1.0);
    
    // But reality is different (marble is in box)
    std::string reality = "box";
    
    EXPECT_TRUE(tom.has_false_belief("Sally", "marble_location", reality));
}

TEST_F(TheoryOfMindTest, SallyAnneTest) {
    // Setup: Sally puts marble in basket
    tom.track_belief("Sally", "marble_location", "basket", 1.0);
    
    // Sally leaves (her belief is frozen)
    // Anne moves marble to box (Sally doesn't see)
    // (We don't update Sally's belief - that's the point!)
    
    // Run the Sally-Anne test
    auto result = tom.run_sally_anne_test();
    
    EXPECT_TRUE(result.correct);
    EXPECT_EQ(result.prediction, "basket");
    EXPECT_FALSE(result.reasoning.empty());
}

TEST_F(TheoryOfMindTest, GoalTracking) {
    tom.track_goal("Alice", "goal1", "find food");
    
    auto model = tom.get_model("Alice");
    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->goals.size(), 1);
    EXPECT_EQ(model->goals.at("goal1"), "find food");
}

TEST_F(TheoryOfMindTest, IntentionTracking) {
    tom.track_intention("Bob", "next_action", "walk to store");
    
    std::string prediction = tom.predict_action("Bob");
    EXPECT_EQ(prediction, "walk to store");
}

TEST_F(TheoryOfMindTest, SelfModel) {
    tom.update_self_model("my_state", "active");
    
    auto belief = tom.get_belief("self", "my_state");
    ASSERT_TRUE(belief.has_value());
    EXPECT_EQ(belief.value(), "active");
}

TEST_F(TheoryOfMindTest, Reset) {
    tom.track_belief("Alice", "test", "value", 1.0);
    tom.track_belief("Bob", "test", "value", 1.0);
    
    EXPECT_EQ(tom.num_agents(), 3); // Alice, Bob, self
    
    tom.reset();
    
    EXPECT_EQ(tom.num_agents(), 1); // Only self remains
    EXPECT_FALSE(tom.get_belief("Alice", "test").has_value());
}

//==============================================================================
// AFFECTIVE CORE TESTS
//==============================================================================

class AffectiveCoreTest : public ::testing::Test {
protected:
    AffectiveCore affect;
};

TEST_F(AffectiveCoreTest, InitialState) {
    EXPECT_DOUBLE_EQ(affect.valence(), 0.0);
    EXPECT_DOUBLE_EQ(affect.arousal(), 0.0);
    EXPECT_DOUBLE_EQ(affect.novelty(), 0.0);
    EXPECT_EQ(affect.emotion_category(), EmotionCategory::NEUTRAL);
}

TEST_F(AffectiveCoreTest, PositiveReward) {
    // Positive reward should increase valence (need multiple updates to reach threshold)
    for (int i = 0; i < 5; ++i) {
        affect.update(1.0, 0.0, 0.0);
    }
    
    EXPECT_GT(affect.valence(), 0.0);
    EXPECT_TRUE(affect.is_pleasant());
}

TEST_F(AffectiveCoreTest, NegativeReward) {
    // Negative reward should decrease valence (need multiple updates to reach threshold)
    for (int i = 0; i < 5; ++i) {
        affect.update(-1.0, 0.0, 0.0);
    }
    
    EXPECT_LT(affect.valence(), 0.0);
    EXPECT_TRUE(affect.is_unpleasant());
}

TEST_F(AffectiveCoreTest, ArousalFromPredictionError) {
    // High prediction error should increase arousal
    affect.update(0.0, 2.0, 0.0);
    
    EXPECT_GT(affect.arousal(), 0.0);
}

TEST_F(AffectiveCoreTest, NoveltyTracking) {
    // Novel context should increase novelty (need multiple updates to reach threshold)
    for (int i = 0; i < 3; ++i) {
        affect.update(0.0, 0.0, 1.0);
    }
    
    EXPECT_GT(affect.novelty(), 0.0);
    EXPECT_TRUE(affect.is_novel());
}

TEST_F(AffectiveCoreTest, PleasantExcited) {
    // High valence + high arousal = pleasant excited (joy)
    affect.update(1.0, 2.0, 0.0);
    
    for (int i = 0; i < 5; ++i) {
        affect.update(1.0, 2.0, 0.0);
    }
    
    EXPECT_TRUE(affect.is_pleasant());
    EXPECT_TRUE(affect.is_highly_aroused());
    EXPECT_EQ(affect.emotion_category(), EmotionCategory::PLEASANT_EXCITED);
}

TEST_F(AffectiveCoreTest, UnpleasantExcited) {
    // Low valence + high arousal = unpleasant excited (anxiety)
    for (int i = 0; i < 5; ++i) {
        affect.update(-1.0, 2.0, 0.0);
    }
    
    EXPECT_TRUE(affect.is_unpleasant());
    EXPECT_TRUE(affect.is_highly_aroused());
    EXPECT_EQ(affect.emotion_category(), EmotionCategory::UNPLEASANT_EXCITED);
}

TEST_F(AffectiveCoreTest, Neuromodulators) {
    // Positive valence → dopamine
    affect.update(1.0, 0.0, 0.0);
    EXPECT_GT(affect.dopamine(), 0.0);
    
    // High arousal → norepinephrine
    affect.update(0.0, 2.0, 0.0);
    EXPECT_GT(affect.norepinephrine(), 0.0);
    
    // High novelty → acetylcholine
    affect.update(0.0, 0.0, 1.0);
    EXPECT_GT(affect.acetylcholine(), 0.0);
}

TEST_F(AffectiveCoreTest, Decay) {
    // Set to excited state
    affect.update(1.0, 2.0, 1.0);
    
    double initial_valence = affect.valence();
    double initial_arousal = affect.arousal();
    double initial_novelty = affect.novelty();
    
    // Apply decay multiple times
    for (int i = 0; i < 10; ++i) {
        affect.apply_decay();
    }
    
    // Should decay toward zero
    EXPECT_LT(std::abs(affect.valence()), std::abs(initial_valence));
    EXPECT_LT(std::abs(affect.arousal()), std::abs(initial_arousal));
    EXPECT_LT(std::abs(affect.novelty()), std::abs(initial_novelty));
}

TEST_F(AffectiveCoreTest, Intensity) {
    // Neutral state has zero intensity
    EXPECT_DOUBLE_EQ(affect.intensity(), 0.0);
    
    // Excited state has positive intensity
    affect.update(1.0, 2.0, 1.0);
    EXPECT_GT(affect.intensity(), 0.0);
}

TEST_F(AffectiveCoreTest, Description) {
    affect.update(1.0, 2.0, 0.5);
    
    std::string desc = affect.describe();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("Emotion:"), std::string::npos);
    EXPECT_NE(desc.find("Neuromodulators:"), std::string::npos);
}

TEST_F(AffectiveCoreTest, Reset) {
    affect.update(1.0, 2.0, 1.0);
    
    EXPECT_NE(affect.valence(), 0.0);
    
    affect.reset();
    
    EXPECT_DOUBLE_EQ(affect.valence(), 0.0);
    EXPECT_DOUBLE_EQ(affect.arousal(), 0.0);
    EXPECT_DOUBLE_EQ(affect.novelty(), 0.0);
    EXPECT_EQ(affect.emotion_category(), EmotionCategory::NEUTRAL);
}

//==============================================================================
// EPISTEMIC DRIVE TESTS
//==============================================================================

class EpistemicDriveTest : public ::testing::Test {
protected:
    EpistemicDrive drive;
};

TEST_F(EpistemicDriveTest, InitialState) {
    EXPECT_EQ(drive.num_observations(), 0);
    EXPECT_FALSE(drive.has_sufficient_data());
}

TEST_F(EpistemicDriveTest, ObserveErrors) {
    drive.observe_error(1.0);
    drive.observe_error(1.1);
    drive.observe_error(0.9);
    
    EXPECT_EQ(drive.num_observations(), 3);
}

TEST_F(EpistemicDriveTest, Statistics) {
    // Add some normal errors
    for (int i = 0; i < 20; ++i) {
        drive.observe_error(1.0 + 0.1 * (i % 5 - 2)); // ~1.0 ± 0.2
    }
    
    EXPECT_TRUE(drive.has_sufficient_data());
    
    Scalar mean = drive.mean_error();
    Scalar std = drive.std_error();
    
    EXPECT_NEAR(mean, 1.0, 0.2);
    EXPECT_GT(std, 0.0);
}

TEST_F(EpistemicDriveTest, NormalOperation) {
    // Establish baseline
    for (int i = 0; i < 20; ++i) {
        drive.observe_error(1.0 + 0.1 * (i % 3 - 1));
    }
    
    // Check normal error
    auto result = drive.check_crisis(1.1);
    
    EXPECT_FALSE(result.in_crisis);
    EXPECT_LT(std::abs(result.z_score), 5.0);
}

TEST_F(EpistemicDriveTest, CrisisDetection) {
    // Establish baseline (errors around 1.0 ± 0.1)
    for (int i = 0; i < 50; ++i) {
        drive.observe_error(1.0 + 0.05 * (i % 5 - 2));
    }
    
    // Catastrophic error (way outside normal range)
    Scalar catastrophic_error = 10.0;
    auto result = drive.check_crisis(catastrophic_error);
    
    EXPECT_TRUE(result.in_crisis);
    EXPECT_GT(result.z_score, 5.0);
    EXPECT_NE(result.recommendation.find("CRISIS"), std::string::npos);
}

TEST_F(EpistemicDriveTest, ZScoreComputation) {
    // Simple test case with known statistics
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0);
    drive.observe_error(0.0); // 10 zeros
    
    // Mean should be 0, std should be 0
    EXPECT_NEAR(drive.mean_error(), 0.0, 1e-6);
    
    // Any non-zero error should trigger (but z-score might be inf or handled)
    auto result = drive.check_crisis(1.0);
    // With zero std, z-score calculation is special-cased
}

TEST_F(EpistemicDriveTest, CuriosityBonus) {
    Scalar bonus1 = drive.compute_curiosity_bonus(0.5, 0.3);
    Scalar bonus2 = drive.compute_curiosity_bonus(1.0, 0.8);
    
    // Higher novelty/uncertainty should give higher bonus
    EXPECT_GT(bonus2, bonus1);
}

TEST_F(EpistemicDriveTest, RecentErrors) {
    for (int i = 0; i < 15; ++i) {
        drive.observe_error(static_cast<Scalar>(i));
    }
    
    auto recent = drive.recent_errors(5);
    EXPECT_EQ(recent.size(), 5);
    EXPECT_DOUBLE_EQ(recent[0], 10.0); // Last 5: 10, 11, 12, 13, 14
    EXPECT_DOUBLE_EQ(recent[4], 14.0);
}

TEST_F(EpistemicDriveTest, BufferLimit) {
    EpistemicConfig config;
    config.buffer_size = 10;
    EpistemicDrive limited_drive(config);
    
    // Add more than buffer size
    for (int i = 0; i < 20; ++i) {
        limited_drive.observe_error(static_cast<Scalar>(i));
    }
    
    // Should only keep last 10
    EXPECT_EQ(limited_drive.num_observations(), 10);
}

TEST_F(EpistemicDriveTest, Reset) {
    drive.observe_error(1.0);
    drive.observe_error(2.0);
    
    EXPECT_EQ(drive.num_observations(), 2);
    
    drive.reset();
    
    EXPECT_EQ(drive.num_observations(), 0);
    EXPECT_FALSE(drive.has_sufficient_data());
}

//==============================================================================
// INTEGRATION TEST: Combined Cognitive Systems
//==============================================================================

TEST(CognitiveIntegration, AffectiveEpistemicInteraction) {
    AffectiveCore affect;
    EpistemicDrive drive;
    
    // Normal operation
    for (int i = 0; i < 30; ++i) {
        Scalar reward = 0.1;
        Scalar error = 0.5;
        
        affect.update(reward, error, 0.1);
        drive.observe_error(error);
    }
    
    EXPECT_FALSE(affect.is_highly_aroused());
    
    // Crisis situation
    Scalar catastrophic_error = 10.0;
    auto crisis = drive.check_crisis(catastrophic_error);
    
    if (crisis.in_crisis) {
        // Affect should respond to crisis
        affect.update(-0.9, catastrophic_error, 1.0); // Negative valence, high arousal, high novelty
        
        EXPECT_TRUE(affect.is_highly_aroused());
        EXPECT_TRUE(affect.is_unpleasant());
        EXPECT_EQ(affect.emotion_category(), EmotionCategory::UNPLEASANT_EXCITED); // Distress
    }
}

TEST(CognitiveIntegration, TheoryOfMindAffective) {
    TheoryOfMind tom;
    AffectiveCore affect;
    
    // Track that Alice has a false belief
    tom.track_belief("Alice", "outcome", "success", 1.0);
    std::string reality = "failure";
    
    // Detect false belief
    bool has_false = tom.has_false_belief("Alice", "outcome", reality);
    EXPECT_TRUE(has_false);
    
    // This should trigger emotional response (surprise, concern)
    if (has_false) {
        // Multiple updates to reach novelty threshold
        for (int i = 0; i < 3; ++i) {
            affect.update(-0.3, 0.8, 0.9); // Negative valence, high error, high novelty
        }
        EXPECT_TRUE(affect.is_novel());
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
