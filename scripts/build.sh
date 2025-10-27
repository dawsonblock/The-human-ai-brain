#!/bin/bash
# build.sh - Build script for Human-AI Brain System
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
PRESET="${PRESET:-default}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"
ENABLE_TESTS="${ENABLE_TESTS:-ON}"
ENABLE_GRPC="${ENABLE_GRPC:-ON}"
NUM_JOBS="${NUM_JOBS:-$(nproc)}"
USE_PRESETS="${USE_PRESETS:-1}"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Human-AI Brain Build Script${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Configuration:"
echo "  CMake Preset: $PRESET"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Directory: $BUILD_DIR"
echo "  Enable Tests: $ENABLE_TESTS"
echo "  Enable gRPC: $ENABLE_GRPC"
echo "  Parallel Jobs: $NUM_JOBS"
echo "  Use Presets: $USE_PRESETS"
echo ""

# Check for required tools
echo -e "${YELLOW}Checking dependencies...${NC}"
for cmd in cmake ninja gcc g++; do
    if ! command -v $cmd &> /dev/null; then
        echo -e "${RED}Error: $cmd not found${NC}"
        exit 1
    fi
done
echo -e "${GREEN}✓ All required tools found${NC}"
echo ""

# Clean build directory if requested
if [ "${CLEAN_BUILD:-0}" = "1" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "${GREEN}✓ Build directory cleaned${NC}"
    echo ""
fi

# Configure
echo -e "${YELLOW}Configuring CMake...${NC}"
if [ "$USE_PRESETS" = "1" ] && [ -f "CMakePresets.json" ]; then
    echo "Using CMake preset: $PRESET"
    cmake --preset "$PRESET"
else
    echo "Using manual configuration"
    cmake -S . -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DBUILD_TESTS="$ENABLE_TESTS" \
        -DBUILD_GRPC_SERVER="$ENABLE_GRPC" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ CMake configuration successful${NC}"
else
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
fi
echo ""

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build "$BUILD_DIR" -j"$NUM_JOBS"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi
echo ""

# Run tests if enabled
if [ "$ENABLE_TESTS" = "ON" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    cd "$BUILD_DIR"
    ctest --output-on-failure --verbose
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ All tests passed${NC}"
    else
        echo -e "${RED}✗ Some tests failed${NC}"
        exit 1
    fi
    cd ..
    echo ""
fi

# Summary
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Build Summary${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Binaries:"
echo "  - $BUILD_DIR/kernel/brain_server"
echo "  - $BUILD_DIR/kernel/interactive_demo"
if [ "$ENABLE_TESTS" = "ON" ]; then
    echo "  - $BUILD_DIR/kernel/minimal_test"
    echo "  - $BUILD_DIR/kernel/qw_tests"
    echo "  - $BUILD_DIR/kernel/cognitive_tests"
    echo "  - $BUILD_DIR/kernel/brain_system_tests"
fi
echo ""
echo -e "${GREEN}✓ Build completed successfully!${NC}"
