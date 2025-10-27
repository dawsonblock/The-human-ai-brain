#!/bin/bash
# test.sh - Comprehensive test runner for Human-AI Brain System
set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
TEST_FILTER="${TEST_FILTER:-*}"
VERBOSE="${VERBOSE:-0}"
COVERAGE="${COVERAGE:-0}"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Human-AI Brain Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found: $BUILD_DIR${NC}"
    echo "Please run ./scripts/build.sh first"
    exit 1
fi

cd "$BUILD_DIR"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test executable
run_test() {
    local test_name=$1
    local test_path=$2
    
    if [ ! -f "$test_path" ]; then
        echo -e "${YELLOW}⊘ Skipping $test_name (not built)${NC}"
        return
    fi
    
    echo -e "${BLUE}Running $test_name...${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ "$VERBOSE" = "1" ]; then
        if "$test_path" --gtest_filter="$TEST_FILTER"; then
            PASSED_TESTS=$((PASSED_TESTS + 1))
            echo -e "${GREEN}✓ $test_name passed${NC}"
        else
            FAILED_TESTS=$((FAILED_TESTS + 1))
            echo -e "${RED}✗ $test_name failed${NC}"
        fi
    else
        if "$test_path" --gtest_filter="$TEST_FILTER" > /dev/null 2>&1; then
            PASSED_TESTS=$((PASSED_TESTS + 1))
            echo -e "${GREEN}✓ $test_name passed${NC}"
        else
            FAILED_TESTS=$((FAILED_TESTS + 1))
            echo -e "${RED}✗ $test_name failed${NC}"
            # Show output on failure
            "$test_path" --gtest_filter="$TEST_FILTER"
        fi
    fi
    echo ""
}

# Run kernel tests
echo -e "${YELLOW}=== Kernel Tests ===${NC}"
run_test "Minimal Tests" "kernel/minimal_test"
run_test "Quantum Workspace Tests" "kernel/qw_tests"
run_test "Cognitive Module Tests" "kernel/cognitive_tests"
run_test "Brain System Tests" "kernel/brain_system_tests"
echo ""

# Run Cockpit tests
echo -e "${YELLOW}=== Cockpit Safety Tests ===${NC}"
run_test "Cockpit Tests" "Cockpit-main/cockpit_tests"
echo ""

# Run CTest for additional tests
echo -e "${YELLOW}=== Running CTest ===${NC}"
if ctest --output-on-failure -R "$TEST_FILTER"; then
    echo -e "${GREEN}✓ CTest passed${NC}"
else
    echo -e "${RED}✗ CTest failed${NC}"
fi
echo ""

# Coverage report if enabled
if [ "$COVERAGE" = "1" ]; then
    echo -e "${YELLOW}=== Generating Coverage Report ===${NC}"
    if command -v lcov &> /dev/null; then
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --list coverage.info
        
        if command -v genhtml &> /dev/null; then
            genhtml coverage.info --output-directory coverage_html
            echo -e "${GREEN}✓ Coverage report generated in coverage_html/${NC}"
        fi
    else
        echo -e "${YELLOW}⊘ lcov not found, skipping coverage${NC}"
    fi
    echo ""
fi

# Summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo "Total Test Suites: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
else
    echo "Failed: 0"
fi
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
