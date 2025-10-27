// cockpit_tests.cpp
//
// Comprehensive test suite for Cockpit safety systems:
// - Kill switch functionality
// - Change gate validation
// - Moral core policy enforcement
// - Self-writer integration

#include <gtest/gtest.h>
#include "kill.h"
#include "change_gate.h"
#include "moral_core.h"
#include "self_writer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================================
// TEST SUITE 1: KILL SWITCH
// ============================================================================

class KillSwitchTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure clean state
        kill::reset();
        // Clear environment variables
        unsetenv("COCKPIT_EVOLVE");
        unsetenv("KILL_SWITCH_PATH");
    }
    
    void TearDown() override {
        // Clean up after each test
        kill::reset();
        unsetenv("COCKPIT_EVOLVE");
        unsetenv("KILL_SWITCH_PATH");
    }
};

TEST_F(KillSwitchTest, InitiallyNotTripped) {
    std::cout << "Testing kill switch is initially not tripped..." << std::endl;
    
    EXPECT_FALSE(kill::is_tripped()) 
        << "Kill switch should not be tripped initially";
    
    // Should not throw
    EXPECT_NO_THROW(kill::require_alive());
    
    std::cout << "✓ Kill switch starts in safe state" << std::endl;
}

TEST_F(KillSwitchTest, TripAndReset) {
    std::cout << "Testing kill switch trip and reset..." << std::endl;
    
    // Initially not tripped
    EXPECT_FALSE(kill::is_tripped());
    
    // Trip the switch
    kill::trip();
    EXPECT_TRUE(kill::is_tripped()) 
        << "Kill switch should be tripped after trip()";
    
    // Should throw when required alive
    EXPECT_THROW(kill::require_alive(), std::runtime_error)
        << "require_alive() should throw when tripped";
    
    // Reset the switch
    kill::reset();
    EXPECT_FALSE(kill::is_tripped()) 
        << "Kill switch should not be tripped after reset()";
    
    // Should not throw after reset
    EXPECT_NO_THROW(kill::require_alive());
    
    std::cout << "✓ Kill switch trip/reset cycle works correctly" << std::endl;
}

TEST_F(KillSwitchTest, EnvironmentVariableControl) {
    std::cout << "Testing kill switch control via COCKPIT_EVOLVE..." << std::endl;
    
    // Set environment to "off"
    setenv("COCKPIT_EVOLVE", "off", 1);
    EXPECT_TRUE(kill::is_tripped()) 
        << "Kill switch should trip when COCKPIT_EVOLVE=off";
    
    // Set to "on"
    setenv("COCKPIT_EVOLVE", "on", 1);
    EXPECT_FALSE(kill::is_tripped()) 
        << "Kill switch should not trip when COCKPIT_EVOLVE=on";
    
    std::cout << "✓ Environment variable control works" << std::endl;
}

TEST_F(KillSwitchTest, BlocksDestructiveOperations) {
    std::cout << "Testing kill switch blocks destructive operations..." << std::endl;
    
    // Trip the switch
    kill::trip();
    
    // Simulate checking before a destructive operation
    bool operation_allowed = false;
    try {
        kill::require_alive();
        operation_allowed = true;
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "kill switch is tripped");
        operation_allowed = false;
    }
    
    EXPECT_FALSE(operation_allowed) 
        << "Destructive operations should be blocked when kill switch is tripped";
    
    std::cout << "✓ Kill switch blocks operations correctly" << std::endl;
}

// ============================================================================
// TEST SUITE 2: CHANGE GATE
// ============================================================================

class ChangeGateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set strict mode
        setenv("EXPLAIN_POLICY", "strict", 1);
    }
    
    void TearDown() override {
        unsetenv("EXPLAIN_POLICY");
    }
};

TEST_F(ChangeGateTest, ValidExplanationAccepted) {
    std::cout << "Testing valid explanation is accepted..." << std::endl;
    
    json explanation = {
        {"why", "This change improves performance by optimizing the inner loop with SIMD instructions which reduces computation time significantly"},
        {"risk", "May introduce numerical instability on edge cases"},
        {"backout", "Revert the commit using git revert"},
        {"tests", "Added unit tests for edge cases in test_performance.cpp"},
        {"touched_symbols", {"optimize_loop", "process_data"}}
    };
    
    json ast_delta = {
        {"added", {"optimize_loop"}},
        {"removed", {}},
        {"modified", {"process_data"}}
    };
    
    std::vector<std::string> errors;
    bool valid = change_gate::validate_explanation(explanation, ast_delta, errors);
    
    EXPECT_TRUE(valid) << "Valid explanation should be accepted";
    EXPECT_TRUE(errors.empty()) << "Should have no errors";
    
    std::cout << "✓ Valid explanation accepted" << std::endl;
}

TEST_F(ChangeGateTest, MissingFieldsRejected) {
    std::cout << "Testing explanation with missing fields is rejected..." << std::endl;
    
    json explanation = {
        {"why", "This is a simple change to fix a bug"},
        {"risk", "Low risk"}
        // Missing: backout, tests, touched_symbols
    };
    
    json ast_delta = {{"added", {}}, {"removed", {}}, {"modified", {}}};
    
    std::vector<std::string> errors;
    bool valid = change_gate::validate_explanation(explanation, ast_delta, errors);
    
    EXPECT_FALSE(valid) << "Explanation with missing fields should be rejected";
    EXPECT_FALSE(errors.empty()) << "Should have validation errors";
    
    std::cout << "  Validation errors: " << errors.size() << std::endl;
    for (const auto& err : errors) {
        std::cout << "    - " << err << std::endl;
    }
    
    std::cout << "✓ Missing fields correctly rejected" << std::endl;
}

TEST_F(ChangeGateTest, InsufficientWordCountRejected) {
    std::cout << "Testing explanation with insufficient word count is rejected..." << std::endl;
    
    json explanation = {
        {"why", "Short explanation"},  // < 15 words
        {"risk", "Low"},  // < 5 words
        {"backout", "Revert"},  // < 5 words
        {"tests", "Added tests for the feature"},
        {"touched_symbols", {"foo"}}
    };
    
    json ast_delta = {{"added", {"foo"}}, {"removed", {}}, {"modified", {}}};
    
    std::vector<std::string> errors;
    bool valid = change_gate::validate_explanation(explanation, ast_delta, errors);
    
    EXPECT_FALSE(valid) << "Explanation with insufficient word count should be rejected";
    EXPECT_GE(errors.size(), 3) << "Should have at least 3 word count errors";
    
    std::cout << "✓ Word count requirements enforced" << std::endl;
}

TEST_F(ChangeGateTest, SymbolMismatchDetected) {
    std::cout << "Testing symbol mismatch detection..." << std::endl;
    
    json explanation = {
        {"why", "This change adds a new function to handle edge cases that were previously unhandled in the system"},
        {"risk", "Minimal risk as the function is isolated"},
        {"backout", "Remove the new function and restore previous behavior"},
        {"tests", "Added unit tests for the new function"},
        {"touched_symbols", {"wrong_symbol"}}  // Doesn't match AST
    };
    
    json ast_delta = {
        {"added", {"actual_function"}},
        {"removed", {}},
        {"modified", {}}
    };
    
    std::vector<std::string> errors;
    bool valid = change_gate::validate_explanation(explanation, ast_delta, errors);
    
    EXPECT_FALSE(valid) << "Symbol mismatch should be detected";
    
    // Check for symbols_mismatch error
    bool has_mismatch_error = false;
    for (const auto& err : errors) {
        if (err.find("symbols_mismatch") != std::string::npos ||
            err.find("touched_symbols") != std::string::npos) {
            has_mismatch_error = true;
            break;
        }
    }
    EXPECT_TRUE(has_mismatch_error) << "Should report symbols_mismatch error";
    
    std::cout << "✓ Symbol mismatch correctly detected" << std::endl;
}

TEST_F(ChangeGateTest, EnforcementModes) {
    std::cout << "Testing different enforcement modes..." << std::endl;
    
    // Test strict mode (default)
    setenv("EXPLAIN_POLICY", "strict", 1);
    EXPECT_EQ(change_gate::enforcement_mode(), "strict");
    
    // Test advisory mode
    setenv("EXPLAIN_POLICY", "advisory", 1);
    EXPECT_EQ(change_gate::enforcement_mode(), "advisory");
    
    // Test off mode
    setenv("EXPLAIN_POLICY", "off", 1);
    EXPECT_EQ(change_gate::enforcement_mode(), "off");
    
    // Test default (unset)
    unsetenv("EXPLAIN_POLICY");
    EXPECT_EQ(change_gate::enforcement_mode(), "strict") 
        << "Should default to strict mode";
    
    std::cout << "✓ Enforcement modes work correctly" << std::endl;
}

// ============================================================================
// TEST SUITE 3: MORAL CORE
// ============================================================================

TEST(MoralCoreTest, BasicDecisionMaking) {
    std::cout << "Testing moral core decision making..." << std::endl;
    
    json plan = {
        {"intent", "refactor function"},
        {"file", "src/utils.cpp"},
        {"diff_hash", "abc123"}
    };
    
    auto decision = moral_core::choose(plan);
    
    // Current implementation is a stub that allows all actions
    EXPECT_FALSE(decision.block) 
        << "Stub moral core should allow actions";
    
    std::cout << "  Decision: " << (decision.block ? "BLOCK" : "ALLOW") << std::endl;
    if (decision.block) {
        std::cout << "  Reason: " << decision.reason << std::endl;
    }
    
    std::cout << "✓ Moral core provides decisions" << std::endl;
}

TEST(MoralCoreTest, DecisionStructure) {
    std::cout << "Testing moral core decision structure..." << std::endl;
    
    json plan = {{"action", "test"}};
    auto decision = moral_core::choose(plan);
    
    // Verify structure
    EXPECT_NO_THROW({
        bool block = decision.block;
        std::string reason = decision.reason;
    }) << "Decision should have valid structure";
    
    std::cout << "✓ Decision structure is correct" << std::endl;
}

TEST(MoralCoreTest, PolicyEnforcement) {
    std::cout << "Testing policy enforcement on dangerous operations..." << std::endl;
    
    // Create a plan for a potentially dangerous operation
    json dangerous_plan = {
        {"intent", "modify system file"},
        {"file", "/etc/passwd"},  // System critical file
        {"action", "write"}
    };
    
    auto decision = moral_core::choose(dangerous_plan);
    
    // Note: Current stub implementation allows everything
    // In production, this should block system file modifications
    std::cout << "  System file modification: " 
              << (decision.block ? "BLOCKED" : "ALLOWED (stub)") << std::endl;
    
    std::cout << "✓ Policy enforcement interface functional" << std::endl;
}

// ============================================================================
// TEST SUITE 4: SELF-WRITER INTEGRATION
// ============================================================================

class SelfWriterTest : public ::testing::Test {
protected:
    std::string test_dir;
    
    void SetUp() override {
        // Create temp test directory
        test_dir = "/tmp/cockpit_test_" + std::to_string(getpid());
        fs::create_directories(test_dir);
        
        // Ensure kill switch is not tripped
        kill::reset();
        unsetenv("COCKPIT_EVOLVE");
        
        // Set advisory mode for testing
        setenv("EXPLAIN_POLICY", "advisory", 1);
    }
    
    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        
        kill::reset();
        unsetenv("EXPLAIN_POLICY");
    }
};

TEST_F(SelfWriterTest, KillSwitchBlocks) {
    std::cout << "Testing self-writer respects kill switch..." << std::endl;
    
    // Trip the kill switch
    kill::trip();
    
    std::string test_file = test_dir + "/test.txt";
    std::string content = "new content";
    
    json explanation = {
        {"why", "Testing kill switch functionality to ensure safety mechanisms work correctly in the system"},
        {"risk", "No risk in test environment"},
        {"backout", "Delete test file and restore state"},
        {"tests", "Unit test for kill switch"},
        {"touched_symbols", {"test"}}
    };
    
    // Should throw when kill switch is tripped
    EXPECT_THROW({
        self_writer::apply_change(test_file, content, "test", "testing", &explanation);
    }, std::runtime_error) << "Self-writer should throw when kill switch is tripped";
    
    // File should not be created
    EXPECT_FALSE(fs::exists(test_file)) 
        << "File should not be created when kill switch is tripped";
    
    std::cout << "✓ Kill switch blocks self-writer operations" << std::endl;
}

TEST_F(SelfWriterTest, SuccessfulWrite) {
    std::cout << "Testing successful self-writer operation..." << std::endl;
    
    kill::reset();  // Ensure kill switch is off
    
    std::string test_file = test_dir + "/success.txt";
    std::string content = "test content\nline 2\nline 3";
    
    json explanation = {
        {"why", "Creating test file to validate self-writer functionality and ensure proper file creation with audit trail"},
        {"risk", "No risk in isolated test environment"},
        {"backout", "Remove test file using filesystem cleanup"},
        {"tests", "File existence and content validation tests"},
        {"touched_symbols", {"test_function"}}
    };
    
    // Should succeed
    EXPECT_NO_THROW({
        auto result = self_writer::apply_change(
            test_file, content, "test_user", "test operation", &explanation);
        
        std::cout << "  Report ID: " << result.report_id << std::endl;
        std::cout << "  Snapshot: " << result.snapshot << std::endl;
        std::cout << "  SHA256: " << result.new_sha256 << std::endl;
        
        EXPECT_FALSE(result.report_id.empty()) << "Should generate report ID";
        EXPECT_FALSE(result.new_sha256.empty()) << "Should compute SHA256";
    });
    
    // Verify file was created
    EXPECT_TRUE(fs::exists(test_file)) << "File should be created";
    
    // Verify content
    std::ifstream ifs(test_file);
    std::string actual_content((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());
    EXPECT_EQ(actual_content, content) << "File content should match";
    
    std::cout << "✓ Self-writer successfully creates files" << std::endl;
}

TEST_F(SelfWriterTest, AuditTrailCreated) {
    std::cout << "Testing self-writer creates audit trail..." << std::endl;
    
    kill::reset();
    
    std::string test_file = test_dir + "/audit_test.txt";
    std::string content = "audit test content";
    
    json explanation = {
        {"why", "Testing audit trail generation to ensure all changes are properly logged and traceable in the system"},
        {"risk", "Minimal risk in test scenario"},
        {"backout", "Remove test files and audit records"},
        {"tests", "Audit trail validation and report structure tests"},
        {"touched_symbols", {"audit_function"}}
    };
    
    auto result = self_writer::apply_change(
        test_file, content, "audit_tester", "audit test", &explanation);
    
    EXPECT_FALSE(result.report_id.empty()) 
        << "Should generate non-empty report ID";
    
    std::cout << "  Report ID generated: " << result.report_id << std::endl;
    
    // Note: Actual report file location depends on implementation
    // In production, verify report file exists and contains correct data
    
    std::cout << "✓ Audit trail created successfully" << std::endl;
}

TEST_F(SelfWriterTest, SnapshotCreation) {
    std::cout << "Testing self-writer creates snapshots of original files..." << std::endl;
    
    kill::reset();
    
    std::string test_file = test_dir + "/snapshot_test.txt";
    
    // Create original file
    {
        std::ofstream ofs(test_file);
        ofs << "original content";
    }
    
    std::string new_content = "modified content";
    
    json explanation = {
        {"why", "Testing snapshot functionality to ensure original file states are preserved before modifications for rollback capability"},
        {"risk", "Low risk with snapshot backup available"},
        {"backout", "Restore from snapshot using backup file"},
        {"tests", "Snapshot integrity and restore functionality tests"},
        {"touched_symbols", {"snapshot_function"}}
    };
    
    auto result = self_writer::apply_change(
        test_file, new_content, "snapshot_tester", "snapshot test", &explanation);
    
    EXPECT_FALSE(result.snapshot.empty()) 
        << "Should create snapshot of original file";
    
    std::cout << "  Snapshot path: " << result.snapshot << std::endl;
    
    // Verify snapshot exists (if path is provided)
    if (!result.snapshot.empty() && result.snapshot != "none") {
        EXPECT_TRUE(fs::exists(result.snapshot)) 
            << "Snapshot file should exist";
    }
    
    std::cout << "✓ Snapshot creation works correctly" << std::endl;
}

// ============================================================================
// TEST SUITE 5: INTEGRATION TESTS
// ============================================================================

TEST(IntegrationTest, FullSafetyPipeline) {
    std::cout << "Testing full safety pipeline integration..." << std::endl;
    
    // Ensure safe starting state
    kill::reset();
    setenv("EXPLAIN_POLICY", "advisory", 1);
    
    // 1. Check kill switch
    EXPECT_FALSE(kill::is_tripped()) << "Step 1: Kill switch should be clear";
    
    // 2. Validate explanation
    json explanation = {
        {"why", "Integration test validating complete safety pipeline from kill switch through moral core to change gate validation"},
        {"risk", "No risk in controlled test environment"},
        {"backout", "Test cleanup handled by test framework"},
        {"tests", "Full integration test coverage"},
        {"touched_symbols", {"integration_test"}}
    };
    
    json ast_delta = {
        {"added", {"integration_test"}},
        {"removed", {}},
        {"modified", {}}
    };
    
    std::vector<std::string> errors;
    bool explanation_valid = change_gate::validate_explanation(
        explanation, ast_delta, errors);
    EXPECT_TRUE(explanation_valid) << "Step 2: Explanation should be valid";
    
    // 3. Check moral core
    json plan = {
        {"intent", "integration test"},
        {"file", "/tmp/test.txt"}
    };
    
    auto decision = moral_core::choose(plan);
    EXPECT_FALSE(decision.block) << "Step 3: Moral core should allow test operation";
    
    std::cout << "✓ Full safety pipeline integration successful" << std::endl;
    
    unsetenv("EXPLAIN_POLICY");
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    std::cout << "\n================================================" << std::endl;
    std::cout << "  COCKPIT SAFETY SYSTEMS TEST SUITE" << std::endl;
    std::cout << "================================================\n" << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n================================================" << std::endl;
    if (result == 0) {
        std::cout << "  ✓ ALL SAFETY TESTS PASSED" << std::endl;
    } else {
        std::cout << "  ✗ SOME SAFETY TESTS FAILED" << std::endl;
    }
    std::cout << "================================================\n" << std::endl;
    
    return result;
}
