#!/bin/bash
# Start all required services for the ingestion pipeline

set -e

echo "🚀 Starting Human-AI Brain Ingestion Pipeline Services"
echo "======================================================="

# Check if ports are already in use
check_port() {
    local port=$1
    local service=$2
    if lsof -Pi :$port -sTCP:LISTEN -t >/dev/null 2>&1; then
        echo "⚠️  Port $port already in use (may be $service already running)"
        return 1
    fi
    return 0
}

# Start Qdrant
echo ""
echo "📦 Starting Qdrant Vector Database..."
if check_port 6333 "Qdrant"; then
    docker run -d --name qdrant \
        -p 6333:6333 \
        -v qdrant_data:/qdrant/storage \
        qdrant/qdrant
    echo "✅ Qdrant started on http://localhost:6333"
else
    echo "   Using existing Qdrant instance"
fi

# Start embedding service stub
echo ""
echo "🧮 Starting Embedding Service (stub)..."
if check_port 8081 "Embedding Service"; then
    cat > /tmp/embed_stub.py << 'EOF'
from flask import Flask, request, jsonify
import numpy as np
import sys

app = Flask(__name__)

@app.post("/embed")
def embed():
    t = request.json.get("text", "")
    rng = np.random.default_rng(abs(hash(t)) % (2**32))
    vec = rng.standard_normal(1024).tolist()
    return jsonify({"embedding": vec})

@app.get("/health")
def health():
    return jsonify({"status": "healthy"})

if __name__ == "__main__":
    print("🟢 Embedding stub server starting on http://0.0.0.0:8081", file=sys.stderr, flush=True)
    app.run(host="0.0.0.0", port=8081, debug=False)
EOF
    
    # Check if Flask is installed
    if ! python3 -c "import flask" 2>/dev/null; then
        echo "   Installing Flask..."
        pip3 install flask numpy --quiet
    fi
    
    nohup python3 /tmp/embed_stub.py > /tmp/embed_stub.log 2>&1 &
    echo $! > /tmp/embed_stub.pid
    sleep 2
    echo "✅ Embedding service started on http://localhost:8081"
    echo "   (Using random embeddings stub - replace with real service later)"
else
    echo "   Using existing embedding service"
fi

# Instructions for OCR service (requires GPU)
echo ""
echo "🔍 DeepSeek-OCR Service (Manual Start Required)"
echo "================================================"
echo "This service requires GPU and must be started manually:"
echo ""
echo "  # Install vLLM"
echo "  pip install vllm"
echo ""
echo "  # Start server"
echo "  python -m vllm.entrypoints.openai.api_server \\"
echo "    --model deepseek-ai/DeepSeek-OCR \\"
echo "    --host 0.0.0.0 \\"
echo "    --port 8000"
echo ""
echo "For now, continuing without OCR service..."

# Wait for services to be ready
echo ""
echo "⏳ Waiting for services to be ready..."
sleep 3

# Health checks
echo ""
echo "🏥 Health Checks"
echo "================"

# Check Qdrant
if curl -s http://localhost:6333/collections >/dev/null 2>&1; then
    echo "✅ Qdrant: healthy"
else
    echo "❌ Qdrant: not responding"
fi

# Check embedding service
if curl -s http://localhost:8081/health >/dev/null 2>&1; then
    echo "✅ Embedding Service: healthy"
else
    echo "❌ Embedding Service: not responding"
fi

# Check OCR service
if curl -s http://localhost:8000/v1/models >/dev/null 2>&1; then
    echo "✅ DeepSeek-OCR: healthy"
else
    echo "⚠️  DeepSeek-OCR: not running (start manually if needed)"
fi

echo ""
echo "======================================================="
echo "✅ Services started!"
echo ""
echo "To stop services:"
echo "  ./scripts/stop_services.sh"
echo ""
echo "To view logs:"
echo "  tail -f /tmp/embed_stub.log"
echo "  docker logs -f qdrant"
echo "======================================================="
