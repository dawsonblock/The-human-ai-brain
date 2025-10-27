#!/bin/bash
# Validation script for document ingestion pipeline setup

set -e

echo "🔍 Validating Human-AI Brain Document Ingestion Pipeline Setup"
echo "================================================================"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check counters
PASSED=0
FAILED=0
WARNINGS=0

check_command() {
    if command -v "$1" &> /dev/null; then
        echo -e "${GREEN}✓${NC} $1 is installed"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗${NC} $1 is NOT installed"
        ((FAILED++))
        return 1
    fi
}

check_library() {
    if pkg-config --exists "$1" 2>/dev/null; then
        VERSION=$(pkg-config --modversion "$1")
        echo -e "${GREEN}✓${NC} $1 is installed (version: $VERSION)"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗${NC} $1 is NOT installed"
        ((FAILED++))
        return 1
    fi
}

check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓${NC} $1 exists"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}✗${NC} $1 is missing"
        ((FAILED++))
        return 1
    fi
}

check_directory() {
    if [ -d "$1" ]; then
        echo -e "${GREEN}✓${NC} $1 exists"
        ((PASSED++))
        return 0
    else
        echo -e "${YELLOW}⚠${NC} $1 is missing"
        ((WARNINGS++))
        return 1
    fi
}

check_service() {
    if curl -s -o /dev/null -w "%{http_code}" "$1" | grep -q "200\|404"; then
        echo -e "${GREEN}✓${NC} Service at $1 is reachable"
        ((PASSED++))
        return 0
    else
        echo -e "${YELLOW}⚠${NC} Service at $1 is NOT reachable"
        ((WARNINGS++))
        return 1
    fi
}

echo ""
echo "1. Build Tools"
echo "--------------"
check_command cmake
check_command make
check_command g++
check_command pkg-config

echo ""
echo "2. System Libraries"
echo "-------------------"
check_library poppler-cpp
check_library libcurl
check_library openssl

echo ""
echo "3. Project Structure"
echo "--------------------"
check_directory "build"
check_directory "include/brain"
check_directory "src"
check_directory "scripts"
check_file "CMakeLists.txt"
check_file "config.example.json"
check_file "third_party/nlohmann/json.hpp"

echo ""
echo "4. Built Executables"
echo "--------------------"
if check_file "build/build_jsonl"; then
    SIZE=$(du -h build/build_jsonl | cut -f1)
    echo "   Size: $SIZE"
fi

if check_file "build/rag_index"; then
    SIZE=$(du -h build/rag_index | cut -f1)
    echo "   Size: $SIZE"
fi

echo ""
echo "5. Services (Optional)"
echo "----------------------"
check_service "http://localhost:8000/v1/models" || echo "   Install: python -m vllm.entrypoints.openai.api_server --model deepseek-ai/DeepSeek-OCR"
check_service "http://localhost:8081/embed" || echo "   Install: python embed_service.py"
check_service "http://localhost:6333" || echo "   Install: docker run -d -p 6333:6333 qdrant/qdrant"

echo ""
echo "6. Configuration Files"
echo "----------------------"
if [ -f "config.example.json" ]; then
    echo -e "${GREEN}✓${NC} config.example.json exists"
    # Validate JSON
    if python3 -c "import json; json.load(open('config.example.json'))" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} config.example.json is valid JSON"
        ((PASSED+=2))
    else
        echo -e "${RED}✗${NC} config.example.json is invalid JSON"
        ((FAILED++))
    fi
fi

if [ -f "qdrant_cloud.json" ]; then
    echo -e "${GREEN}✓${NC} qdrant_cloud.json exists"
    if python3 -c "import json; json.load(open('qdrant_cloud.json'))" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} qdrant_cloud.json is valid JSON"
        ((PASSED+=2))
    else
        echo -e "${RED}✗${NC} qdrant_cloud.json is invalid JSON"
        ((FAILED++))
    fi
fi

echo ""
echo "================================================================"
echo "Summary:"
echo "  Passed:   $PASSED"
echo "  Failed:   $FAILED"
echo "  Warnings: $WARNINGS"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All critical checks passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some critical checks failed. Please review above.${NC}"
    exit 1
fi
