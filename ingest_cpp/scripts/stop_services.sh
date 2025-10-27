#!/bin/bash
# Stop all ingestion pipeline services

set -e

echo "🛑 Stopping Human-AI Brain Ingestion Pipeline Services"
echo "======================================================="

# Stop embedding service
if [ -f /tmp/embed_stub.pid ]; then
    PID=$(cat /tmp/embed_stub.pid)
    if kill -0 $PID 2>/dev/null; then
        echo "Stopping embedding service (PID: $PID)..."
        kill $PID
        rm /tmp/embed_stub.pid
        echo "✅ Embedding service stopped"
    else
        echo "⚠️  Embedding service not running"
        rm /tmp/embed_stub.pid
    fi
else
    echo "⚠️  Embedding service PID file not found"
fi

# Stop Qdrant
if docker ps | grep -q qdrant; then
    echo "Stopping Qdrant..."
    docker stop qdrant
    docker rm qdrant
    echo "✅ Qdrant stopped"
else
    echo "⚠️  Qdrant container not running"
fi

echo ""
echo "======================================================="
echo "✅ All services stopped"
echo ""
echo "Note: OCR service must be stopped manually if running"
echo "======================================================="
