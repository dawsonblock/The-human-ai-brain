#!/bin/bash
# Comprehensive end-to-end integration test

set -e

echo "========================================================================"
echo "  Human-AI Brain Document Ingestion - Full Integration Test"
echo "========================================================================"
echo ""

# Configuration
TEST_DIR="test_data"
OUTPUT_DIR="$TEST_DIR/test_output_$(date +%Y%m%d_%H%M%S)"
CONFIG_FILE="config.example.json"

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_step() {
    echo -e "${BLUE}➤${NC} $1"
}

log_success() {
    echo -e "${GREEN}✓${NC} $1"
}

log_error() {
    echo -e "${RED}✗${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Check if services are running
check_services() {
    log_step "Checking required services..."
    
    SERVICES_OK=true
    
    if curl -s http://localhost:8000/v1/models > /dev/null 2>&1; then
        log_success "OCR service is running (port 8000)"
    else
        log_warning "OCR service not running (using mock)"
        if [ ! -f "mock_ocr_service.py" ]; then
            log_error "mock_ocr_service.py not found"
            exit 1
        fi
    fi
    
    if curl -s http://localhost:8081/health > /dev/null 2>&1; then
        log_success "Embedding service is running (port 8081)"
    else
        log_warning "Embedding service not running"
        if [ ! -f "embed_service.py" ]; then
            log_error "embed_service.py not found"
            exit 1
        fi
        log_step "Starting embedding service..."
        python3 embed_service.py > embed_service.log 2>&1 &
        echo $! > embed_service.pid
        sleep 2
        if curl -s http://localhost:8081/health > /dev/null 2>&1; then
            log_success "Embedding service started"
        else
            log_error "Failed to start embedding service"
            exit 1
        fi
    fi
    
    echo ""
}

# Step 1: Build executables
build_project() {
    log_step "Step 1: Building project..."
    
    if [ ! -d "build" ]; then
        mkdir build
        cd build
        cmake .. > /dev/null 2>&1
        cd ..
    fi
    
    cmake --build build > /dev/null 2>&1
    
    if [ -f "build/build_jsonl" ] && [ -f "build/rag_index" ]; then
        log_success "Build successful"
        echo "  - build_jsonl: $(du -h build/build_jsonl | cut -f1)"
        echo "  - rag_index: $(du -h build/rag_index | cut -f1)"
    else
        log_error "Build failed"
        exit 1
    fi
    echo ""
}

# Step 2: Process PDFs
process_pdfs() {
    log_step "Step 2: Processing PDF documents..."
    
    if [ ! -d "$TEST_DIR/pdfs" ]; then
        log_error "Test PDF directory not found: $TEST_DIR/pdfs"
        exit 1
    fi
    
    PDF_COUNT=$(find "$TEST_DIR/pdfs" -name "*.pdf" | wc -l)
    echo "  Found $PDF_COUNT PDF files"
    
    ./build/build_jsonl "$CONFIG_FILE" "$TEST_DIR/pdfs" "$OUTPUT_DIR"
    
    if [ -f "$OUTPUT_DIR/train/ocr_sft.jsonl" ]; then
        CHUNK_COUNT=$(wc -l < "$OUTPUT_DIR/train/ocr_sft.jsonl")
        log_success "Generated $CHUNK_COUNT training chunks"
    else
        log_error "Failed to generate training data"
        exit 1
    fi
    echo ""
}

# Step 3: Validate output
validate_output() {
    log_step "Step 3: Validating output..."
    
    # Check markdown files
    MD_COUNT=$(find "$OUTPUT_DIR/ocr_md" -name "*.md" 2>/dev/null | wc -l)
    if [ $MD_COUNT -gt 0 ]; then
        log_success "Generated $MD_COUNT markdown files"
    else
        log_warning "No markdown files generated"
    fi
    
    # Check JSONL format
    if python3 << PYEOF
import json
import sys

try:
    with open("$OUTPUT_DIR/train/ocr_sft.jsonl", "r") as f:
        for i, line in enumerate(f, 1):
            data = json.loads(line)
            assert "instruction" in data, f"Missing 'instruction' at line {i}"
            assert "input" in data, f"Missing 'input' at line {i}"
            assert "output" in data, f"Missing 'output' at line {i}"
    print("✓ All JSONL records are valid")
    sys.exit(0)
except Exception as e:
    print(f"✗ JSONL validation failed: {e}")
    sys.exit(1)
PYEOF
    then
        log_success "JSONL format validation passed"
    else
        log_error "JSONL format validation failed"
        exit 1
    fi
    echo ""
}

# Step 4: Test Qdrant indexing (optional)
test_qdrant() {
    log_step "Step 4: Testing Qdrant indexing (optional)..."
    
    if [ ! -f "qdrant_cloud.json" ]; then
        log_warning "qdrant_cloud.json not found, skipping Qdrant test"
        echo ""
        return
    fi
    
    log_step "Indexing to Qdrant..."
    ./build/rag_index qdrant_cloud.json "$OUTPUT_DIR/train/ocr_sft.jsonl"
    
    log_success "Qdrant indexing test completed"
    echo ""
}

# Step 5: Generate report
generate_report() {
    log_step "Step 5: Generating test report..."
    
    REPORT_FILE="$OUTPUT_DIR/TEST_REPORT.md"
    
    cat > "$REPORT_FILE" << REPORT
# Integration Test Report

**Date**: $(date '+%Y-%m-%d %H:%M:%S')
**Output Directory**: $OUTPUT_DIR

## Test Results

### PDF Processing
- PDFs Processed: $(find "$TEST_DIR/pdfs" -name "*.pdf" | wc -l)
- Pages Rendered: $(find "$OUTPUT_DIR/tmp" -name "*.png" 2>/dev/null | wc -l)
- Markdown Files: $(find "$OUTPUT_DIR/ocr_md" -name "*.md" 2>/dev/null | wc -l)

### Training Data
- Total Chunks: $(wc -l < "$OUTPUT_DIR/train/ocr_sft.jsonl")
- Output File: \`$OUTPUT_DIR/train/ocr_sft.jsonl\`

### File Sizes
\`\`\`
$(du -h "$OUTPUT_DIR" | tail -1)
\`\`\`

### Sample Training Record
\`\`\`json
$(head -1 "$OUTPUT_DIR/train/ocr_sft.jsonl" | python3 -m json.tool)
\`\`\`

### Build Information
- build_jsonl: $(du -h build/build_jsonl | cut -f1)
- rag_index: $(du -h build/rag_index | cut -f1)

## Status

✅ All tests passed successfully!

REPORT
    
    log_success "Test report generated: $REPORT_FILE"
    echo ""
}

# Main execution
main() {
    check_services
    build_project
    process_pdfs
    validate_output
    test_qdrant
    generate_report
    
    echo "========================================================================"
    echo -e "${GREEN}✓ Integration test completed successfully!${NC}"
    echo "========================================================================"
    echo ""
    echo "Output directory: $OUTPUT_DIR"
    echo "Test report: $OUTPUT_DIR/TEST_REPORT.md"
    echo ""
}

# Run main
main "$@"
