# Integration Guide: Ingestion Pipeline + Brain System 🧠🔗

How to connect the document ingestion pipeline with the Human-AI Brain FDQC system.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Document Sources                          │
│              (PDFs, Papers, Documentation)                   │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│              INGESTION PIPELINE (C++)                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ PDF→PNG  │→ │   OCR    │→ │ Chunking │→ │  JSONL   │   │
│  │ Poppler  │  │ DeepSeek │  │ Markdown │  │  Output  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
│                                                   │          │
│                                                   ▼          │
│                                              ┌──────────┐   │
│                                              │  Qdrant  │   │
│                                              │ RAG Index│   │
│                                              └──────────┘   │
└─────────────────────────────────────────────────────────────┘
                       │                           │
                       ▼                           ▼
┌─────────────────────────────────────────────────────────────┐
│              HUMAN-AI BRAIN FDQC (C++20)                     │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                    BrainTrainer                       │   │
│  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐    │   │
│  │  │  Adam  │  │  SGD   │  │RMSProp │  │  Loss  │    │   │
│  │  └────────┘  └────────┘  └────────┘  └────────┘    │   │
│  │              Training on JSONL Data                  │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                   BrainSystem                         │   │
│  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐    │   │
│  │  │   QW   │  │   GW   │  │  ToM   │  │ Affect │    │   │
│  │  │  7D    │  │  60D   │  │ Belief │  │ Emotion│    │   │
│  │  └────────┘  └────────┘  └────────┘  └────────┘    │   │
│  │              Inference with RAG Context              │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## Integration Points

### 1. Training Data Flow

The ingestion pipeline generates JSONL files that can be directly consumed by the brain's training system.

#### A. Generate Training Data

```bash
# Process consciousness research papers
cd ingest_cpp
./build/build_jsonl config.json \
  /data/consciousness_papers \
  ./brain_training_data

# Output: brain_training_data/train/ocr_sft.jsonl
```

#### B. Train the Brain

```bash
cd ../kernel
./build/kernel/interactive_demo  # Or your training executable
```

**Training Configuration** (`kernel/training_config.json`):
```json
{
  "data_path": "../ingest_cpp/brain_training_data/train/ocr_sft.jsonl",
  "num_epochs": 10,
  "batch_size": 32,
  "learning_rate": 0.001,
  "optimizer": "adam",
  "mode": "supervised"
}
```

### 2. RAG-Enhanced Inference

Use Qdrant for retrieval-augmented generation during brain inference.

#### A. Index Documents

```bash
cd ingest_cpp
./build/rag_index config.json ./brain_training_data/train/ocr_sft.jsonl
```

#### B. Query During Inference

**C++ Implementation** (add to `kernel/include/brain/rag_retriever.hpp`):

```cpp
#pragma once
#include <string>
#include <vector>
#include <Eigen/Dense>

struct RAGConfig {
    std::string qdrant_url = "http://localhost:6333";
    std::string collection = "docs";
    std::string embed_url = "http://localhost:8081/embed";
    int top_k = 5;
};

struct RetrievedContext {
    std::string text;
    double score;
};

class RAGRetriever {
public:
    explicit RAGRetriever(const RAGConfig& config);
    
    // Embed query and retrieve relevant contexts
    std::vector<RetrievedContext> retrieve(const std::string& query);
    
    // Combine contexts into single input
    std::string format_context(const std::vector<RetrievedContext>& contexts);
    
private:
    RAGConfig config_;
    // HTTP client implementation
};
```

**Usage in BrainSystem**:

```cpp
#include "brain/brain_system.hpp"
#include "brain/rag_retriever.hpp"

void process_with_rag(BrainSystem& brain, RAGRetriever& rag, 
                     const std::string& user_query) {
    // 1. Retrieve relevant contexts
    auto contexts = rag.retrieve(user_query);
    
    // 2. Format as brain input
    std::string context_str = rag.format_context(contexts);
    std::string full_input = context_str + "\n\nQuery: " + user_query;
    
    // 3. Convert to vector (you'll need your text encoding)
    Eigen::VectorXd input_vec = encode_text(full_input);
    
    // 4. Process through brain
    auto result = brain.step(input_vec);
    
    // 5. Generate response
    auto report = brain.generate_phenomenal_report();
    std::cout << report.description << std::endl;
}
```

### 3. Continuous Learning Pipeline

Set up a continuous learning loop that processes new documents and updates the brain.

**Pipeline Script** (`scripts/continuous_learning.sh`):

```bash
#!/bin/bash
# Continuous learning pipeline

WATCH_DIR="/data/new_papers"
PROCESSED_DIR="/data/processed"

while true; do
    # Check for new PDFs
    NEW_PDFS=$(find $WATCH_DIR -name "*.pdf" -mtime -1)
    
    if [ -n "$NEW_PDFS" ]; then
        echo "Found new documents, processing..."
        
        # 1. Ingest new documents
        ./ingest_cpp/build/build_jsonl \
            config.json \
            $WATCH_DIR \
            ./new_data
        
        # 2. Index for RAG
        ./ingest_cpp/build/rag_index \
            config.json \
            ./new_data/train/ocr_sft.jsonl
        
        # 3. Train brain incrementally
        ./kernel/build/kernel/trainer_executable \
            --config incremental_config.json \
            --data ./new_data/train/ocr_sft.jsonl \
            --checkpoint ./checkpoints/latest.ckpt
        
        # 4. Move processed files
        mv $WATCH_DIR/*.pdf $PROCESSED_DIR/
        
        echo "Processing complete!"
    fi
    
    # Wait before next check
    sleep 3600  # 1 hour
done
```

### 4. gRPC Service Integration

Extend the brain gRPC server to include RAG retrieval.

**Add to `kernel/proto/brain.proto`**:

```protobuf
service Brain {
    // Existing endpoints
    rpc Health(HealthReq) returns (HealthResp);
    rpc Step(StepReq) returns (StepResp);
    rpc GetState(StateReq) returns (StateResp);
    
    // New RAG endpoints
    rpc QueryWithRAG(RAGQueryReq) returns (RAGQueryResp);
    rpc IndexDocument(IndexDocReq) returns (IndexDocResp);
}

message RAGQueryReq {
    string query = 1;
    int32 top_k = 2;
    bool use_rag = 3;  // Enable/disable RAG
}

message RAGQueryResp {
    repeated string contexts = 1;
    repeated double scores = 2;
    StepResp brain_response = 3;
}
```

**Implementation** (`kernel/src/brain_service.cpp`):

```cpp
grpc::Status BrainServiceImpl::QueryWithRAG(
    grpc::ServerContext* context,
    const hab::RAGQueryReq* request,
    hab::RAGQueryResp* response) 
{
    std::lock_guard<std::mutex> lock(brain_mutex_);
    
    // Retrieve contexts if RAG enabled
    if (request->use_rag()) {
        auto contexts = rag_retriever_.retrieve(request->query());
        for (const auto& ctx : contexts) {
            response->add_contexts(ctx.text);
            response->add_scores(ctx.score);
        }
    }
    
    // Process through brain
    // ... (rest of implementation)
    
    return grpc::Status::OK;
}
```

## Example Workflows

### Workflow 1: Paper Collection → Training

```bash
# 1. Collect papers
mkdir -p /data/consciousness_papers
# Add PDFs...

# 2. Process documents
cd ingest_cpp
./build/build_jsonl config.json /data/consciousness_papers ./training_data

# 3. Train brain
cd ../kernel
./build/kernel/interactive_demo
# Select mode 7: Training Demo
# Point to: ../ingest_cpp/training_data/train/ocr_sft.jsonl

# 4. Save checkpoint
# Brain automatically saves to ./checkpoints/
```

### Workflow 2: RAG-Enhanced Chatbot

```bash
# 1. Index knowledge base
cd ingest_cpp
./build/rag_index config.json ./knowledge_base.jsonl

# 2. Start brain gRPC server with RAG
cd ../kernel
./build/kernel/brain_server 0.0.0.0:50051

# 3. Query from Python client
python3 << 'EOF'
import grpc
import brain_pb2
import brain_pb2_grpc

channel = grpc.insecure_channel('localhost:50051')
stub = brain_pb2_grpc.BrainStub(channel)

# Query with RAG
response = stub.QueryWithRAG(
    brain_pb2.RAGQueryReq(
        query="What is quantum consciousness?",
        top_k=5,
        use_rag=True
    )
)

print("Retrieved contexts:")
for ctx, score in zip(response.contexts, response.scores):
    print(f"  [{score:.3f}] {ctx[:100]}...")

print("\nBrain response:")
print(response.brain_response.phenomenal_report.description)
EOF
```

### Workflow 3: Batch Processing

```bash
# Process multiple document sets in parallel
cd ingest_cpp

# Create batch configs
for dataset in papers books articles; do
    cat > config_${dataset}.json << EOF
{
  "qdrant_collection": "${dataset}_docs",
  "chunk_size": 2000,
  "chunk_overlap": 200
}
EOF
done

# Process in parallel
parallel -j 3 './build/build_jsonl config_{}.json /data/{} ./output_{}' \
  ::: papers books articles

# Merge JSONL files
cat output_*/train/ocr_sft.jsonl > complete_training.jsonl

# Train on merged data
cd ../kernel
./trainer --data ../ingest_cpp/complete_training.jsonl
```

## Performance Optimization

### 1. Parallel PDF Processing

Modify `build_jsonl.cpp` to use thread pool:

```cpp
#include <thread>
#include <future>

// In main():
std::vector<std::future<void>> futures;
for (auto& pdf : pdf_files) {
    futures.push_back(std::async(std::launch::async, [&]() {
        process_pdf(pdf, cfg);
    }));
}
for (auto& f : futures) f.get();
```

### 2. Batch Embedding

Instead of one-by-one, batch embed multiple chunks:

```cpp
// Modify rag_index.cpp
std::vector<std::string> text_batch;
for (size_t i = 0; i < 32 && !jl.eof(); ++i) {
    text_batch.push_back(chunk);
}
auto embeddings = embed_batch(text_batch);  // Single API call
```

### 3. Caching

Add Redis for embedding cache:

```cpp
auto cache_key = sha256(text);
if (auto cached = redis_get(cache_key)) {
    return cached;
}
auto embedding = compute_embedding(text);
redis_set(cache_key, embedding, 3600);  // 1 hour TTL
```

## Monitoring & Metrics

### Integration Metrics to Track

1. **Ingestion Pipeline**:
   - PDFs processed per hour
   - OCR accuracy (if ground truth available)
   - Chunk statistics (avg size, overlap efficiency)

2. **Training**:
   - Loss on ingested data vs. synthetic data
   - Convergence speed
   - Memory consolidation rate

3. **RAG Performance**:
   - Retrieval latency (p50, p95, p99)
   - Relevance score distribution
   - Brain inference improvement with vs. without RAG

4. **End-to-End**:
   - Document → Indexed time
   - Query → Response time
   - System resource utilization

## Troubleshooting

### Issue: Training loss not decreasing

**Possible causes**:
- Chunk size too large → reduce to 1000-1500
- Instruction mismatch → align with brain's expected format
- Learning rate too high → try 1e-4 or 1e-5

**Solution**:
```bash
# Regenerate with smaller chunks
./build/build_jsonl config.json --chunk-size 1000 ...

# Adjust training config
vim kernel/training_config.json
# Set learning_rate: 0.0001
```

### Issue: RAG returning irrelevant contexts

**Possible causes**:
- Embedding model mismatch
- Collection not properly indexed
- Query embedding quality

**Solution**:
```python
# Test embedding service directly
import requests
response = requests.post("http://localhost:8081/embed", 
                        json={"text": "test query"})
print(response.json())  # Check dimensionality and values

# Rebuild index with better embeddings
./build/rag_index config_fixed.json data.jsonl
```

## Future Enhancements

1. **Active Learning**: Brain requests specific documents for learning gaps
2. **Semantic Chunking**: Use brain's understanding to create better chunks
3. **Multi-Modal**: Integrate images from PDFs directly
4. **Distributed Processing**: Scale to millions of documents
5. **Fine-grained RAG**: Retrieve at sentence/phrase level

## Contributing

See main project [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

## License

MIT License - Same as parent project
