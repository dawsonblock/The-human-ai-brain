from flask import Flask, request, jsonify
import sys

app = Flask(__name__)

# Mock OCR responses for test PDFs
# Rotates through responses to simulate multi-page documents
MOCK_RESPONSES = [
    # Page 1: Simple test document
    """# Test Document for Ingestion

This is a test document for the Human-AI Brain ingestion pipeline.

The pipeline performs the following steps:
1. Converts PDF pages to PNG images
2. Sends images to DeepSeek-OCR for text extraction
3. Chunks the extracted text with configurable overlap
4. Generates JSONL training data
5. Indexes the data into Qdrant vector database

This document tests the complete workflow from PDF to RAG-indexed knowledge.""",
    
    # Page 2: Quantum mechanics content
    """## Quantum Consciousness Research

The quantum workspace (QW) operates in a 7-dimensional Hilbert space.
It evolves according to the Lindblad master equation:

    dρ/dt = -i[H, ρ] + Σ γ(LρL† - ½{L†L, ρ})

Quantum collapse occurs when entropy exceeds ln(7) = 1.9459
or when dwell time surpasses 120ms.

The collapse frequency of 8.2 Hz matches EEG alpha rhythm,
providing empirical validation of the quantum consciousness model.""",

    # Page 3: Brain architecture overview
    """# Human-AI Brain Architecture Overview

The Human-AI Brain is a neuromorphic cognitive system that processes information
through specialized regions, each handling different aspects of thought and memory.

**Key Components:**
1. Quantum Workspace (QW) - Primary conscious processing unit
2. Basal Ganglia (BG) - Action selection and habit formation
3. Episodic Buffer (EB) - Temporal context integration
4. Hippocampus (HC) - Long-term memory consolidation

The system operates on a 120ms cycle, matching human cognitive rhythm.
Each cycle involves parallel processing across all brain regions,
with selective attention modulating information flow between components.""",

    # Page 4: Technical implementation
    """# Technical Implementation

## Quantum Workspace Dynamics

The QW operates in a 7-dimensional Hilbert space H₇, where each dimension
represents a distinct cognitive feature (visual, auditory, semantic, etc.).

**State evolution** follows the Lindblad master equation:
```
dρ/dt = -i[H, ρ] + Σᵢ γᵢ(LᵢρLᵢ† - ½{Lᵢ†Lᵢ, ρ})
```

**Collapse criteria:**
- Entropy threshold: S(ρ) > ln(7) ≈ 1.946 nats
- Dwell time: t_dwell > 120ms
- Collapse frequency: f_c = 8.2 Hz (alpha rhythm)

## RAG Integration
PDF documents are ingested through a multi-stage pipeline:
1. Poppler renders pages to high-DPI PNG images (150-300 DPI)
2. DeepSeek-OCR extracts text with structural markdown
3. Text is chunked with 2000-char windows and 200-char overlap
4. Embeddings generated via local embedding service
5. Vectors indexed in Qdrant for semantic retrieval""",

    # Page 5: Training pipeline
    """# Training and Inference Pipeline

## SFT (Supervised Fine-Tuning) Format

Training data is generated in JSONL format with three fields:
```json
{"instruction": "...", "input": "...", "output": "..."}
```

The instruction field provides task context, input contains the document
content, and output is initially empty for unsupervised extraction.

## Inference with RAG
1. User query is embedded using the same embedding model
2. Qdrant performs cosine similarity search over indexed documents
3. Top-k relevant chunks are retrieved (typically k=3-5)
4. Retrieved context is injected into the QW workspace
5. Brain system processes query with augmented context
6. Response is generated through attractor dynamics collapse

## Performance Metrics
- Indexing speed: ~100 docs/second (with batching)
- Query latency: <50ms (Qdrant Cloud)
- Retrieval accuracy: Depends on embedding model quality
- Integration overhead: Minimal (~5-10ms per query)"""
]

page_counter = 0

@app.post("/v1/chat/completions")
def chat_completions():
    global page_counter
    data = request.json
    
    # Return mock markdown for the page (cycle through responses)
    content = MOCK_RESPONSES[page_counter % len(MOCK_RESPONSES)]
    page_counter += 1
    
    return jsonify({
        "choices": [{
            "message": {
                "content": content
            }
        }]
    })

@app.get("/v1/models")
def models():
    return jsonify({
        "data": [{"id": "mock-ocr"}]
    })

if __name__ == "__main__":
    print("🔍 Mock OCR service starting on http://0.0.0.0:8000", file=sys.stderr, flush=True)
    app.run(host="0.0.0.0", port=8000, debug=False)
