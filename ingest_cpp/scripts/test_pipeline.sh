#!/bin/bash
# Test the complete ingestion pipeline with sample data

set -e

echo "🧪 Testing Human-AI Brain Ingestion Pipeline"
echo "============================================="

# Check if build exists
if [ ! -f "build/build_jsonl" ] || [ ! -f "build/rag_index" ]; then
    echo "❌ Binaries not found. Please build first:"
    echo "   mkdir build && cd build"
    echo "   cmake .. && cmake --build . -j\$(nproc)"
    exit 1
fi

# Create test directories
echo ""
echo "📁 Creating test directories..."
mkdir -p test_data/pdfs
mkdir -p test_data/output

# Check if services are running
echo ""
echo "🏥 Checking services..."

if ! curl -s http://localhost:6333/collections >/dev/null 2>&1; then
    echo "❌ Qdrant not running. Start services first:"
    echo "   ./scripts/start_services.sh"
    exit 1
fi
echo "✅ Qdrant is running"

if ! curl -s http://localhost:8081/health >/dev/null 2>&1; then
    echo "❌ Embedding service not running. Start services first:"
    echo "   ./scripts/start_services.sh"
    exit 1
fi
echo "✅ Embedding service is running"

if ! curl -s http://localhost:8000/v1/models >/dev/null 2>&1; then
    echo "⚠️  OCR service not running - will skip OCR tests"
    OCR_AVAILABLE=false
else
    echo "✅ OCR service is running"
    OCR_AVAILABLE=true
fi

# Test configuration
echo ""
echo "📝 Creating test configuration..."
cat > test_data/config.json << EOF
{
  "ocr_url": "http://localhost:8000/v1/chat/completions",
  "ocr_model": "deepseek-ai/DeepSeek-OCR",
  "prompt": "Return clean Markdown for this page.",
  "qdrant_url": "http://localhost:6333",
  "qdrant_collection": "test_docs",
  "embed_url": "http://localhost:8081/embed",
  "dpi": 150,
  "chunk_size": 500,
  "chunk_overlap": 50
}
EOF
echo "✅ Test config created"

# If no PDFs exist, create a dummy one (requires tools)
if [ -z "$(ls -A test_data/pdfs/*.pdf 2>/dev/null)" ]; then
    echo ""
    echo "📄 No test PDFs found. Please add PDF files to test_data/pdfs/"
    echo "   Or generate a test PDF with:"
    echo "   echo 'Test Document' | pandoc -o test_data/pdfs/test.pdf"
    echo ""
    echo "For now, creating a simple test JSONL manually..."
    
    mkdir -p test_data/output/train
    cat > test_data/output/train/test.jsonl << 'EOF'
{"instruction":"Read and summarize the content in 5 bullets.","input":"This is a test document chunk for the ingestion pipeline.","output":""}
{"instruction":"Read and summarize the content in 5 bullets.","input":"Another test chunk with different content.","output":""}
EOF
    echo "✅ Created test JSONL"
    
    # Skip to RAG indexing test
    echo ""
    echo "🔍 Testing RAG indexing..."
    if ./build/rag_index test_data/config.json test_data/output/train/test.jsonl; then
        echo "✅ RAG indexing test passed"
    else
        echo "❌ RAG indexing test failed"
        exit 1
    fi
    
    # Verify indexed data
    echo ""
    echo "🔎 Verifying indexed data..."
    COLLECTION_INFO=$(curl -s http://localhost:6333/collections/test_docs)
    POINT_COUNT=$(echo $COLLECTION_INFO | grep -o '"points_count":[0-9]*' | cut -d: -f2)
    echo "   Points indexed: $POINT_COUNT"
    
    if [ "$POINT_COUNT" -ge 2 ]; then
        echo "✅ Verification passed"
    else
        echo "❌ Expected at least 2 points, got $POINT_COUNT"
        exit 1
    fi
    
else
    # Full pipeline test with PDFs
    if [ "$OCR_AVAILABLE" = true ]; then
        echo ""
        echo "📄 Testing full pipeline with PDFs..."
        if ./build/build_jsonl test_data/config.json test_data/pdfs test_data/output; then
            echo "✅ PDF processing test passed"
        else
            echo "❌ PDF processing test failed"
            exit 1
        fi
        
        echo ""
        echo "🔍 Testing RAG indexing..."
        if ./build/rag_index test_data/config.json test_data/output/train/ocr_sft.jsonl; then
            echo "✅ RAG indexing test passed"
        else
            echo "❌ RAG indexing test failed"
            exit 1
        fi
    else
        echo ""
        echo "⚠️  Skipping PDF tests (OCR service not available)"
    fi
fi

# Cleanup
echo ""
echo "🧹 Cleaning up test collection..."
curl -X DELETE http://localhost:6333/collections/test_docs >/dev/null 2>&1 || true

echo ""
echo "============================================="
echo "✅ All tests passed!"
echo ""
echo "Next steps:"
echo "  1. Add your PDFs to a directory"
echo "  2. Run: ./build/build_jsonl config.example.json /path/to/pdfs ./output"
echo "  3. Run: ./build/rag_index config.example.json ./output/train/ocr_sft.jsonl"
echo "============================================="
