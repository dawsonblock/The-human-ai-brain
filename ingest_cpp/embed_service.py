from flask import Flask, request, jsonify
import numpy as np
import sys

app = Flask(__name__)

@app.post("/embed")
def embed():
    text = request.json.get("text", "")
    # Generate deterministic embedding based on text hash
    rng = np.random.default_rng(abs(hash(text)) % (2**32))
    vec = rng.standard_normal(1024).tolist()
    return jsonify({"embedding": vec})

@app.get("/health")
def health():
    return jsonify({"status": "healthy", "model": "stub-embedder-v1"})

if __name__ == "__main__":
    print("🧮 Embedding service starting on http://0.0.0.0:8081", file=sys.stderr, flush=True)
    app.run(host="0.0.0.0", port=8081, debug=False)
