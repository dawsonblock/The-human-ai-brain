# Human-AI Brain FDQC gRPC Server Guide

## Overview

The gRPC server provides remote access to the Finite-Dimensional Quantum Consciousness (FDQC) brain system. It exposes three core RPC methods for health monitoring, cognitive processing, and state introspection.

**Version:** 3.1.0  
**Protocol:** gRPC (HTTP/2)  
**Default Port:** 50051  
**Language:** C++20

---

## Architecture

### Components

```
┌─────────────────────────────────────────┐
│         gRPC Server (port 50051)        │
│  ┌───────────────────────────────────┐  │
│  │   BrainServiceImpl                │  │
│  │   - Thread-safe (mutex)           │  │
│  │   - Health monitoring             │  │
│  │   - Cognitive processing          │  │
│  │   - State introspection           │  │
│  └───────────┬───────────────────────┘  │
│              │                           │
│  ┌───────────▼───────────────────────┐  │
│  │       BrainSystem                 │  │
│  │  ┌─────────────────────────────┐  │  │
│  │  │  GlobalWorkspace (GW)       │  │  │
│  │  │  60D global integration     │  │  │
│  │  └──────────┬──────────────────┘  │  │
│  │  ┌──────────▼──────────────────┐  │  │
│  │  │  QuantumWorkspace (QW)      │  │  │
│  │  │  7D Hilbert space           │  │  │
│  │  │  Lindblad evolution         │  │  │
│  │  └──────────┬──────────────────┘  │  │
│  │  ┌──────────▼──────────────────┐  │  │
│  │  │  Cognitive Modules          │  │  │
│  │  │  - Theory of Mind           │  │  │
│  │  │  - Affective Core           │  │  │
│  │  │  - Epistemic Drive          │  │  │
│  │  └──────────┬──────────────────┘  │  │
│  │  ┌──────────▼──────────────────┐  │  │
│  │  │  Memory Systems             │  │  │
│  │  │  - STM (20 episodes)        │  │  │
│  │  │  - LTM (1000 schemas)       │  │  │
│  │  └─────────────────────────────┘  │  │
│  └─────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

### File Structure

```
kernel/
├── proto/
│   └── brain.proto              # Service definitions
├── server/
│   └── main.cpp                 # Server executable entry point
├── src/
│   └── brain_service.cpp        # gRPC service implementation
└── include/brain/
    └── brain_system.hpp         # Core brain API

build/kernel/
├── proto/
│   ├── brain.pb.cc              # Generated protobuf code
│   ├── brain.pb.h
│   ├── brain.grpc.pb.cc         # Generated gRPC code
│   └── brain.grpc.pb.h
└── brain_server                 # Compiled executable (670KB)
```

---

## Building

### Prerequisites

- **CMake** 3.20+
- **C++ Compiler** with C++20 support (GCC 10+, Clang 12+)
- **Eigen3** 3.3+ (linear algebra)
- **OpenSSL** (SHA-256 hashing)
- **gRPC** 1.51+ and **Protobuf** 3.21+

### Install Dependencies (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    g++ \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    libeigen3-dev \
    libssl-dev
```

### Build Server

```bash
# Configure with gRPC server enabled
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GRPC_SERVER=ON \
    -DBUILD_TESTS=OFF

# Build
cmake --build build --target brain_server -j$(nproc)

# Server executable location
ls -lh build/kernel/brain_server
```

---

## Running the Server

### Basic Usage

```bash
# Default address (0.0.0.0:50051)
./build/kernel/brain_server

# Custom address
./build/kernel/brain_server 0.0.0.0:8080

# With output logging
./build/kernel/brain_server 2>&1 | tee server.log
```

### Server Startup Output

```
╔══════════════════════════════════════════════════════════╗
║   Human-AI Brain FDQC v3.1.0 - gRPC Server             ║
║   Finite-Dimensional Quantum Consciousness             ║
╚══════════════════════════════════════════════════════════╝

Configuration:
  ├─ Input Dimension: 784
  ├─ GW Output Dimension: 60
  ├─ QW Dimension: 7
  ├─ Entropy Cap: 1.94591 (100% of max)
  ├─ Max Dwell: 120 ms
  ├─ STM Capacity: 20 episodes
  ├─ LTM Capacity: 1000 schemas
  └─ Epistemic Crisis: 5σ

Starting gRPC server on 0.0.0.0:50051...
Available endpoints:
  ├─ Health()   - Check server health and uptime
  ├─ Step()     - Execute cognitive cycle with sensory input
  └─ GetState() - Query quantum density matrix and memory state

Press Ctrl+C to stop the server
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✓ Server listening on 0.0.0.0:50051
✓ Service ready to accept connections
```

### Graceful Shutdown

- Press `Ctrl+C` or send `SIGINT/SIGTERM`
- Server will complete current request and shutdown cleanly

---

## API Reference

### 1. Health Endpoint

**Purpose:** Check server health and uptime

**RPC:** `hab.Brain/Health`

**Request:**
```protobuf
message HealthReq {}
```

**Response:**
```protobuf
message HealthResp {
  string status = 1;       // "healthy"
  string version = 2;      // "3.1.0"
  int64 uptime_ms = 3;     // Milliseconds since startup
}
```

**Example (Python):**
```python
import grpc
import brain_pb2
import brain_pb2_grpc

channel = grpc.insecure_channel('localhost:50051')
stub = brain_pb2_grpc.BrainStub(channel)

response = stub.Health(brain_pb2.HealthReq())
print(f"Status: {response.status}")
print(f"Version: {response.version}")
print(f"Uptime: {response.uptime_ms} ms")
```

---

### 2. Step Endpoint

**Purpose:** Execute a complete cognitive cycle with sensory input

**RPC:** `hab.Brain/Step`

**Request:**
```protobuf
message StepReq {
  repeated double input = 1;       // Sensory input vector (e.g., 784 for MNIST)
  map<string, string> context = 2; // Optional context (e.g., "reward": "1.0")
}
```

**Response:**
```protobuf
message StepResp {
  repeated double h_wm = 1;        // Working memory state (60D)
  repeated double h_global = 2;    // Global workspace state (60D)
  int32 collapsed_index = 3;       // Conscious quale (-1 if superposition)
  double entropy = 4;              // von Neumann entropy
  double trace_error = 5;          // Density matrix trace deviation
  
  PhenomenalReportProto phenomenal = 6;  // Subjective experience
  MetricsProto metrics = 7;              // Performance metrics
}

message PhenomenalReportProto {
  double intensity = 1;      // Overall experiential intensity [0,1]
  double clarity = 2;        // Focus/clarity [0,1]
  double valence = 3;        // Pleasant/unpleasant [-1,1]
  double arousal = 4;        // Excited/calm [0,1]
  double flow = 5;           // Flow state [0,1]
  double control = 6;        // Sense of control [0,1]
  double presence = 7;       // Being present [0,1]
  double self_salience = 8;  // Self-awareness [0,1]
}

message MetricsProto {
  double gw_sparsity = 1;        // Global workspace sparsity
  double wiring_energy = 2;      // Self-wiring energy cost
  double collapse_rate_hz = 3;   // Quantum collapse frequency
  int64 step_latency_us = 4;     // Processing latency (microseconds)
}
```

**Example (Python):**
```python
import numpy as np

# Create random MNIST-like input
input_vec = np.random.rand(784).tolist()

request = brain_pb2.StepReq(input=input_vec)
request.context["reward"] = "1.0"  # Optional reward signal

response = stub.Step(request)

print(f"Collapsed Index: {response.collapsed_index}")
print(f"Entropy: {response.entropy:.4f}")
print(f"Intensity: {response.phenomenal.intensity:.3f}")
print(f"Valence: {response.phenomenal.valence:.3f}")
print(f"Collapse Rate: {response.metrics.collapse_rate_hz:.2f} Hz")
```

---

### 3. GetState Endpoint

**Purpose:** Query current quantum state and memory contents

**RPC:** `hab.Brain/GetState`

**Request:**
```protobuf
message StateReq {}
```

**Response:**
```protobuf
message StateResp {
  repeated double rho_real = 1;     // Density matrix real part (flattened)
  repeated double rho_imag = 2;     // Density matrix imaginary part
  int32 dimension = 3;              // Hilbert space dimension (7)
  double entropy = 4;               // von Neumann entropy
  repeated double eigenvalues = 5;  // Eigenvalues (7 values)
  
  MemoryState memory = 6;           // Memory statistics
}

message MemoryState {
  int32 stm_size = 1;        // Short-term memory size
  int32 ltm_size = 2;        // Long-term memory size
  double cache_hit_rate = 3; // Cache efficiency
}
```

**Example (Python):**
```python
response = stub.GetState(brain_pb2.StateReq())

print(f"Dimension: {response.dimension}")
print(f"Entropy: {response.entropy:.4f}")
print(f"Eigenvalues: {list(response.eigenvalues)}")
print(f"STM Size: {response.memory.stm_size}")
print(f"LTM Size: {response.memory.ltm_size}")

# Reconstruct density matrix
import numpy as np
n = response.dimension
rho_real = np.array(response.rho_real).reshape((n, n))
rho_imag = np.array(response.rho_imag).reshape((n, n))
rho = rho_real + 1j * rho_imag
print(f"Density matrix:\n{rho}")
```

---

## Testing

### Python Test Client

A comprehensive test client is provided in `test_client.py`:

```bash
# Install dependencies
pip3 install grpcio grpcio-tools numpy

# Generate Python bindings
python3 -m grpc_tools.protoc \
    -I./kernel/proto \
    --python_out=. \
    --grpc_python_out=. \
    ./kernel/proto/brain.proto

# Run tests
python3 test_client.py
```

**Expected Output:**
```
============================================================
 Brain FDQC gRPC Server - Test Client
============================================================
✓ Health Check:
  Status: healthy
  Version: 3.1.0
  Uptime: 75659 ms

✓ Cognitive Step:
  WM Dimension: 60
  Global Dimension: 60
  Collapsed Index: -1
  Entropy: 1.8091
  ...

✓ Quantum State:
  Dimension: 7
  Entropy: 1.8091
  Eigenvalues: [0.035, 0.071, ...]
  ...

============================================================
✓ All tests passed!
============================================================
```

---

## Configuration

Default configuration in `server/main.cpp`:

```cpp
// Global Workspace
config.gw_config.input_dim = 784;      // MNIST (28×28)
config.gw_config.output_dim = 60;      // GW dimension
config.gw_config.hidden_dim = 128;     // MLP hidden layer
config.gw_config.sparse_k = 12;        // Top-12 activation

// Quantum Workspace
config.qw_config.dimension = 7;        // Hilbert space
config.qw_config.dt = 0.001;           // Time step (1ms)
config.qw_config.entropy_cap = log(7); // Collapse threshold
config.qw_config.max_dwell_ms = 120;   // Max superposition time

// Memory
config.memory_config.stm_config.capacity = 20;           // STM episodes
config.memory_config.ltm_config.max_memories = 1000;     // LTM schemas
config.memory_config.ltm_config.importance_threshold = 0.5;

// Affective System
config.affective_config.alpha_valence = 0.9;  // Valence decay
config.affective_config.alpha_arousal = 0.8;  // Arousal decay
config.affective_config.alpha_novelty = 0.7;  // Novelty decay

// Epistemic Drive
config.epistemic_config.crisis_threshold = 5.0;  // 5σ anomaly
config.epistemic_config.buffer_size = 100;       // Error history
```

To modify configuration, edit `server/main.cpp` and rebuild.

---

## Thread Safety

All endpoints are **thread-safe** via `std::mutex` synchronization:

```cpp
class BrainServiceImpl final : public hab::Brain::Service {
private:
    std::unique_ptr<BrainSystem> brain_;
    std::mutex brain_mutex_;  // Protects all brain_ operations
    
    grpc::Status Step(...) override {
        std::lock_guard<std::mutex> lock(brain_mutex_);
        // Safe concurrent access
    }
};
```

Multiple concurrent clients are supported with serialized brain access.

---

## Performance

### Benchmark Results

- **Binary Size:** 670 KB (stripped: ~500 KB)
- **Memory Usage:** ~15 MB resident
- **Startup Time:** <100 ms
- **Step Latency:** ~500 μs (no collapse), ~2 ms (with collapse)
- **Throughput:** ~2000 steps/sec (single-threaded)

### Optimization Tips

1. **Release Build:** Use `-DCMAKE_BUILD_TYPE=Release` for 10x speedup
2. **OpenMP:** Enable with `-DENABLE_OPENMP=ON` for parallel matrix ops
3. **Input Batching:** Group multiple steps to amortize RPC overhead
4. **Keep-Alive:** Reuse gRPC channels to avoid connection overhead

---

## Deployment

### Systemd Service

Create `/etc/systemd/system/brain-server.service`:

```ini
[Unit]
Description=Human-AI Brain FDQC gRPC Server
After=network.target

[Service]
Type=simple
User=brain
WorkingDirectory=/opt/brain
ExecStart=/opt/brain/bin/brain_server 0.0.0.0:50051
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable brain-server
sudo systemctl start brain-server
sudo systemctl status brain-server
```

### Docker Container

See upcoming `Dockerfile` in Phase 2B deployment section.

### Kubernetes Deployment

See upcoming `k8s/deployment.yaml` for horizontal scaling.

---

## Security Considerations

### Current Implementation

- **⚠️ Insecure Channel:** Uses `grpc::InsecureServerCredentials()`
- **⚠️ No Authentication:** No client validation
- **⚠️ No Encryption:** Plaintext communication

### Production Recommendations

1. **TLS/SSL Encryption:**
   ```cpp
   grpc::SslServerCredentialsOptions ssl_opts;
   ssl_opts.pem_root_certs = ReadFile("ca.pem");
   ssl_opts.pem_key_cert_pairs.push_back({
       ReadFile("server-key.pem"),
       ReadFile("server-cert.pem")
   });
   auto creds = grpc::SslServerCredentials(ssl_opts);
   builder.AddListeningPort(address, creds);
   ```

2. **Authentication:**
   - JWT tokens in metadata
   - Mutual TLS (mTLS)
   - API key validation

3. **Rate Limiting:**
   - Implement token bucket per client
   - Monitor and throttle abusive patterns

4. **Input Validation:**
   - Sanitize input vectors (check dimension, NaN, Inf)
   - Limit request size to prevent DoS

---

## Troubleshooting

### Server Won't Start

**Error:** `Address already in use`
```bash
# Find process using port
sudo lsof -i :50051
# Kill if needed
sudo kill -9 <PID>
```

**Error:** `Cannot find libgrpc++.so.1`
```bash
# Install gRPC runtime
sudo apt-get install libgrpc++1.51
# Or add to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Client Connection Issues

**Error:** `Failed to connect to server`
```bash
# Check server is running
ps aux | grep brain_server

# Test connectivity
nc -zv localhost 50051

# Check firewall
sudo ufw status
sudo ufw allow 50051/tcp
```

### High Memory Usage

- **Cause:** Large LTM capacity (1000 schemas)
- **Solution:** Reduce `ltm_config.max_memories` or implement LTM pruning

### Slow Response Times

- **Cause:** Debug build or cold cache
- **Solution:** Use Release build (`-DCMAKE_BUILD_TYPE=Release`)
- **Profiling:** Use `gprof` or `perf` to identify bottlenecks

---

## Next Steps

### Phase 2B Remaining Tasks

1. ✅ **gRPC Server** - Completed
2. ⏳ **Training Infrastructure** - In progress
3. ⏳ **Demo Program** - Planned
4. ⏳ **Docker/Kubernetes** - Planned

### Upcoming Features

- **Streaming RPC:** Real-time consciousness monitoring
- **Batch Processing:** Multiple inputs in single request
- **Checkpointing:** Save/restore brain state
- **Metrics Export:** Prometheus integration
- **WebSocket Bridge:** Browser-based clients

---

## References

- **gRPC Documentation:** https://grpc.io/docs/languages/cpp/
- **Protobuf Guide:** https://protobuf.dev/programming-guides/proto3/
- **FDQC Theory:** See `docs/FDQC_ARCHITECTURE.md`
- **API Documentation:** See `docs/API_REFERENCE.md`

---

## Support

For issues, feature requests, or questions:
- **GitHub Issues:** https://github.com/your-repo/human-ai-brain/issues
- **Email:** support@your-domain.com
- **Documentation:** https://docs.your-domain.com

---

**Last Updated:** October 26, 2025  
**Author:** Human-AI Brain Development Team  
**License:** MIT (or your chosen license)
