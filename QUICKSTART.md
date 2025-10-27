# Quick Start Guide - Human-AI Brain System

Get up and running with the Human-AI Brain FDQC v3 system in minutes.

## Prerequisites

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake ninja-build \
    libeigen3-dev libprotobuf-dev protobuf-compiler \
    protobuf-compiler-grpc libgrpc++-dev \
    libssl-dev libcurl4-openssl-dev libgtest-dev
```

### macOS (Homebrew):
```bash
brew install cmake ninja eigen protobuf grpc openssl curl googletest
```

## Build & Run

### Option 1: Quick Build (Recommended)
```bash
# Clone and build
git clone https://github.com/dawsonblock/The-human-ai-brain.git
cd The-human-ai-brain

# Build using script
./scripts/build.sh

# Run tests
./scripts/test.sh

# Start server
cd build/kernel
./brain_server
```

### Option 2: CMake Presets
```bash
# Configure with preset
cmake --preset release

# Build
cmake --build build -j$(nproc)

# Test
ctest --test-dir build --output-on-failure

# Run
./build/kernel/brain_server
```

### Option 3: Docker
```bash
# Build and run with Docker Compose
docker-compose up -d

# Check logs
docker-compose logs -f brain-server

# Access gRPC server at localhost:50051
```

## Available CMake Presets

| Preset | Description |
|--------|-------------|
| `default` | Standard release build with all features |
| `debug` | Debug build with symbols |
| `release` | Optimized release with LTO |
| `asan` | Debug + AddressSanitizer |
| `tsan` | Debug + ThreadSanitizer |
| `ubsan` | Debug + UndefinedBehaviorSanitizer |
| `ci` | CI/CD optimized build |
| `minimal` | Minimal build without optional features |

Example:
```bash
cmake --preset asan
cmake --build build
./build/kernel/minimal_test
```

## Quick Tests

### Run all tests:
```bash
./scripts/test.sh
```

### Run specific test suite:
```bash
cd build
./kernel/minimal_test       # Physics validation
./kernel/qw_tests          # Quantum workspace
./kernel/cognitive_tests   # Cognitive modules
./Cockpit-main/cockpit_tests  # Safety systems
```

### Test with coverage:
```bash
COVERAGE=1 ./scripts/test.sh
# Open coverage_html/index.html in browser
```

## Interactive Demo

```bash
# Run the interactive demo
./build/kernel/interactive_demo

# Follow prompts to test cognitive cycles
```

## gRPC Server

### Start server:
```bash
./build/kernel/brain_server
# Listening on 0.0.0.0:50051
```

### Test with grpcurl:
```bash
# Health check
grpcurl -plaintext localhost:50051 hab.Brain/Health

# Cognitive step
grpcurl -plaintext -d '{"input": [0.1, 0.2, ...]}' \
    localhost:50051 hab.Brain/Step

# Get quantum state
grpcurl -plaintext -d '{}' \
    localhost:50051 hab.Brain/GetState
```

## Docker Compose Profiles

```bash
# Core services only
docker-compose up -d

# With development environment
docker-compose --profile dev up

# Full stack (incl. RAG pipeline)
docker-compose --profile full up -d

# With monitoring (Prometheus + Grafana)
docker-compose --profile monitoring up -d
```

Access points:
- **Brain gRPC**: localhost:50051
- **Grafana**: http://localhost:3000 (admin/admin)
- **Prometheus**: http://localhost:9090
- **Ingest Service**: http://localhost:8080

## Deployment

### Local deployment:
```bash
./scripts/deploy.sh local
cd runtime && ./brain_server
```

### Docker deployment:
```bash
./scripts/deploy.sh docker
```

### Kubernetes:
```bash
DOCKER_REGISTRY=myregistry.io ./scripts/deploy.sh kubernetes
```

### systemd service:
```bash
sudo ./scripts/deploy.sh systemd
sudo systemctl status brain-server
```

## Configuration

Edit `configs/quantum.yaml`:
```yaml
quantum:
  dimension: 7
  dt: 0.01
  decoherence_rate: 0.05
  entropy_threshold: 1.9459  # ln(7)
  max_dwell_ms: 120.0
```

## Troubleshooting

### Build fails with "protoc not found":
```bash
sudo apt-get install protobuf-compiler protobuf-compiler-grpc
# or
brew install protobuf grpc
```

### Tests fail with segmentation fault:
```bash
# Run with AddressSanitizer
cmake --preset asan
cmake --build build
./build/kernel/minimal_test
```

### Server won't start:
```bash
# Check if port is in use
lsof -i :50051

# Try different port
# Edit server/main.cpp and change server_address
```

## Next Steps

- Read [README.md](README.md) for complete documentation
- Review [IMPLEMENTATION_27_ITEMS.md](IMPLEMENTATION_27_ITEMS.md) for architecture details
- Check [configs/](configs/) for configuration options
- Explore [demos/](kernel/demos/) for example code
- See [tests/](kernel/tests/) for test examples

## Support

- GitHub Issues: https://github.com/dawsonblock/The-human-ai-brain/issues
- Documentation: See README.md
- Implementation Guide: See IMPLEMENTATION_27_ITEMS.md

## License

See [LICENSE](LICENSE) file for details.
