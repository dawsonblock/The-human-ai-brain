# Quick Start Guide 🚀

Get the document ingestion pipeline running in 5 minutes.

## Prerequisites

- Linux or macOS
- C++17 compiler
- CMake 3.20+
- Docker (for Qdrant)
- Python 3.8+ (for embedding service)

## 1. Install Dependencies

### Ubuntu/Debian
```bash
sudo apt-get update && sudo apt-get install -y \
  build-essential cmake \
  libpoppler-cpp-dev libcurl4-openssl-dev libssl-dev pkg-config
```

### macOS
```bash
brew install cmake poppler curl openssl pkg-config
```

## 2. Build

```bash
cd ingest_cpp
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
cd ..
```

**Expected output**: `build_jsonl` and `rag_index` executables in `build/`

## 3. Start Services

```bash
# Start Qdrant + Embedding stub
./scripts/start_services.sh
```

**What this does**:
- Starts Qdrant vector database on port 6333
- Starts embedding service stub on port 8081
- Shows instructions for OCR service (optional)

## 4. Test the Pipeline

```bash
# Run automated tests
./scripts/test_pipeline.sh
```

**Expected output**: All tests passing ✅

## 5. Process Your PDFs

```bash
# Copy your configuration
cp config.example.json config.json

# Edit config.json if needed (URLs, DPI, chunk size)

# Process documents
./build/build_jsonl config.json /path/to/your/pdfs ./output

# Index for RAG
./build/rag_index config.json ./output/train/ocr_sft.jsonl
```

## 6. Verify Results

```bash
# Check generated files
ls -lh output/ocr_md/          # Markdown files
ls -lh output/train/           # JSONL training data

# Check Qdrant collection
curl http://localhost:6333/collections/docs
```

## 7. Stop Services (when done)

```bash
./scripts/stop_services.sh
```

## Troubleshooting

### Build fails: "poppler-cpp not found"
```bash
# Ubuntu
sudo apt-get install libpoppler-cpp-dev

# macOS
brew install poppler
```

### Service port already in use
```bash
# Check what's using the port
lsof -i :6333  # Qdrant
lsof -i :8081  # Embedding

# Kill the process or use stop_services.sh
./scripts/stop_services.sh
```

### No PDFs processed
- Ensure PDFs are in the input directory
- Check file permissions (`chmod 644 *.pdf`)
- Look for errors in output

## Next Steps

1. **Add OCR**: Start DeepSeek-OCR server for real document processing
2. **Real Embeddings**: Replace stub with sentence-transformers
3. **Train Brain**: Use generated JSONL with the kernel trainer
4. **Query RAG**: Integrate Qdrant search with brain inference

## Need Help?

See full documentation in [README.md](README.md)
