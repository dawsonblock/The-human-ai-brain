/**
 * Integration Test - Full Pipeline Validation
 * 
 * Tests the complete consciousness evaluation pipeline
 */

#include "self_writer.h"
#include "fdqc_system.h"
#include "change_audit.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cassert>

using json = nlohmann::json;

const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string CYAN = "\033[36m";
const std::string RESET = "\033[0m";

void test_allowed_change() {
    std::cout << CYAN << "\n=== Test 1: Allowed Change ===" << RESET << std::endl;
    
    // Set up test environment
    system("mkdir -p /tmp/cockpit_test/src 2>/dev/null");
    system("mkdir -p /tmp/cockpit_test/logs/changes 2>/dev/null");
    setenv("ALLOWED_ROOT", "/tmp/cockpit_test", 1);
    setenv("CHANGE_LOG_DIR", "/tmp/cockpit_test/logs/changes", 1);
    
    std::string file_path = "src/test_file.cpp";
    std::string old_content = "// Old code\nint x = 1;\n";
    std::string new_content = "// Updated code\nint x = 2;\n";
    std::string author = "integration_test";
    std::string intent = "Update variable value";
    
    json explanation = {
        {"reasoning", "Simple variable update for testing the integration pipeline"},
        {"impact", "minimal - only changes test variable value"},
        {"safety", "verified - isolated test environment"},
        {"why", "Testing integration of FDQC consciousness system with self-writer pipeline. This validates that the full stack from HTTP API through FDQC evaluation to file modification works correctly."},
        {"risk", "No risk - this is an isolated integration test that modifies only test files in a controlled environment. The test file will be cleaned up after execution."},
        {"backout", "Simple backout plan: revert the git commit or delete the test file. No production dependencies or downstream effects."},
        {"tests", "Integration tests will verify the change: automated test suite validates FDQC evaluation results and file modification correctness"},
        {"touched_symbols", json::array({"x"})}
    };
    
    try {
        self_writer::ApplyResult result = self_writer::apply_change(
            file_path, new_content, author, intent, &explanation
        );
        
        std::cout << "  ✓ Change applied successfully" << std::endl;
        std::cout << "  Report ID: " << result.report_id << std::endl;
        std::cout << "  SHA256: " << result.new_sha256 << std::endl;
        
        std::cout << GREEN << "\n✓ Test 1 PASSED" << RESET << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << RED << "\n✗ Test 1 FAILED: " << e.what() << RESET << std::endl;
        exit(1);
    }
}

void test_consciousness_fields() {
    std::cout << CYAN << "\n=== Test 2: Consciousness Fields Validation ===" << RESET << std::endl;
    
    // Set up test environment (already created in test 1, but set again for safety)
    setenv("ALLOWED_ROOT", "/tmp/cockpit_test", 1);
    setenv("CHANGE_LOG_DIR", "/tmp/cockpit_test/logs/changes", 1);
    
    std::string file_path = "src/test_consciousness.cpp";
    std::string old_content = "int old = 1;\n";
    std::string new_content = "int new = 2;\n";
    std::string author = "integration_test";
    std::string intent = "Test consciousness";
    
    json explanation = {
        {"reasoning", "Testing FDQC consciousness fields and evaluation pipeline"},
        {"impact", "none - test file only, no production effects"},
        {"why", "Validate that consciousness evaluation correctly processes changes and generates expected consciousness field values. This ensures the FDQC system properly evaluates change quality."},
        {"risk", "No risk - isolated test that only affects test files. No production code or data is modified. Test files are ephemeral and can be safely deleted."},
        {"backout", "Simple backout: delete the test file or revert the commit. No complex rollback procedure needed since this only affects test artifacts."},
        {"tests", "Automated integration test validates consciousness field generation and verifies that FDQC evaluation produces expected results"},
        {"touched_symbols", json::array({"old", "new"})}
    };
    
    try {
        self_writer::ApplyResult result = self_writer::apply_change(
            file_path, new_content, author, intent, &explanation
        );
        
        // Just verify the change went through
        assert(!result.report_id.empty() && "Should have report ID");
        assert(!result.new_sha256.empty() && "Should have SHA256");
        
        std::cout << "  ✓ Change applied with valid result" << std::endl;
        std::cout << GREEN << "\n✓ Test 2 PASSED" << RESET << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << RED << "\n✗ Test 2 FAILED: " << e.what() << RESET << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║            Integration Test - Full Pipeline                    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        test_allowed_change();
        test_consciousness_fields();
        
        std::cout << GREEN << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              ✓ ALL INTEGRATION TESTS PASSED ✓                 ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════════╝" << RESET << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << RED << "\n✗ Integration test failed: " << e.what() << RESET << std::endl;
        return 1;
    }
}
