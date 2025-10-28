# Human-AI Brain v3.4.0 Production Runbook

**Version:** 3.4.0-prod  
**Last Updated:** 2025-10-28  
**Audience:** DevOps, SRE, Platform Engineers

---

## 📋 Quick Reference

| Component | Port | Health Check | Metrics |
|-----------|------|--------------|---------|
| **gRPC Server** | 50051 | `grpc_health_probe -addr=:50051` | `/metrics:9090` |
| **REST Gateway** | 8080 | `curl localhost:8080/status` | N/A |
| **Prometheus** | 9090 | `curl localhost:9090/metrics` | Self |
| **Qdrant** | 6333 | `curl localhost:6333/health` | `:6333/metrics` |

---

## 🚀 Deployment Methods

### 1️⃣ Local Development

#### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get update && sudo apt-get install -y \
    cmake g++ pkg-config \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    libeigen3-dev libssl-dev libfaiss-dev \
    libprometheus-cpp-dev

# macOS
brew install cmake grpc protobuf eigen openssl faiss prometheus-cpp
```

#### Build
```bash
# Clone repository
git clone https://github.com/dawsonblock/The-human-ai-brain.git
cd The-human-ai-brain

# Configure (Release build with optimizations)
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GRPC_SERVER=ON \
    -DBUILD_TESTS=ON \
    -DENABLE_OPENMP=ON \
    -DCMAKE_CXX_FLAGS="-march=native -O3 -flto"

# Build
cmake --build build -j$(nproc)

# Install (optional)
sudo cmake --install build
```

#### Run
```bash
# Start server
./build/kernel/brain_server \
    --config configs/quantum.yaml \
    --port 50051 \
    --metrics-port 9090

# Start in background
./build/kernel/brain_server --config configs/quantum.yaml &

# Check status
grpc_health_probe -addr=localhost:50051
```

#### Test
```bash
# Run all tests
cd build && ctest --output-on-failure

# Run specific test suite
./build/kernel/tiered_memory_tests
./build/kernel/cognitive_tests

# Benchmark
./build/tools/bench_entropy --duration=10s
```

---

### 2️⃣ Docker Deployment

#### Build Image
```bash
# Standard build
docker build -t brain-ai:v3.4.0 .

# With build args
docker build \
    --build-arg CMAKE_BUILD_TYPE=Release \
    --build-arg ENABLE_GPU=ON \
    -t brain-ai:v3.4.0-gpu .

# Multi-platform
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    -t brain-ai:v3.4.0 \
    --push .
```

#### Run Container
```bash
# Basic run
docker run -d \
    --name brain-server \
    -p 50051:50051 \
    -p 9090:9090 \
    brain-ai:v3.4.0

# With security options
docker run -d \
    --name brain-server \
    --user 65534:65534 \
    --read-only \
    --tmpfs /tmp:rw,noexec,nosuid,size=100m \
    --security-opt seccomp=docker/seccomp.json \
    --security-opt no-new-privileges=true \
    --cap-drop=ALL \
    -p 50051:50051 \
    -p 9090:9090 \
    -v $(pwd)/configs:/etc/brain:ro \
    brain-ai:v3.4.0

# With GPU support
docker run -d \
    --name brain-server \
    --gpus all \
    -p 50051:50051 \
    -p 9090:9090 \
    brain-ai:v3.4.0-gpu
```

#### Docker Compose
```bash
# Start full stack
docker-compose up -d

# View logs
docker-compose logs -f brain-server

# Stop
docker-compose down

# Restart specific service
docker-compose restart brain-server
```

**docker-compose.yml:**
```yaml
version: '3.8'

services:
  brain-server:
    image: brain-ai:v3.4.0
    container_name: brain-server
    ports:
      - "50051:50051"
      - "9090:9090"
    volumes:
      - ./configs:/etc/brain:ro
      - ./data:/data
    security_opt:
      - seccomp:docker/seccomp.json
      - no-new-privileges:true
    user: "65534:65534"
    read_only: true
    tmpfs:
      - /tmp:rw,noexec,nosuid,size=100m
    healthcheck:
      test: ["CMD", "grpc_health_probe", "-addr=:50051"]
      interval: 10s
      timeout: 5s
      retries: 3
    restart: unless-stopped

  qdrant:
    image: qdrant/qdrant:latest
    container_name: qdrant
    ports:
      - "6333:6333"
    volumes:
      - qdrant_data:/qdrant/storage
    restart: unless-stopped

  prometheus:
    image: prom/prometheus:latest
    container_name: prometheus
    ports:
      - "9091:9090"
    volumes:
      - ./configs/prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus_data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
    restart: unless-stopped

volumes:
  qdrant_data:
  prometheus_data:
```

---

### 3️⃣ Kubernetes Deployment

#### Deploy
```bash
# Create namespace
kubectl create namespace brain-ai

# Apply all manifests
kubectl apply -f k8s/ -n brain-ai

# Or individually
kubectl apply -f k8s/configmap.yaml -n brain-ai
kubectl apply -f k8s/secret.yaml -n brain-ai
kubectl apply -f k8s/deployment.yaml -n brain-ai
kubectl apply -f k8s/service.yaml -n brain-ai
kubectl apply -f k8s/hpa.yaml -n brain-ai
```

#### Verify Deployment
```bash
# Check pods
kubectl get pods -n brain-ai -l app=brain-server

# Check services
kubectl get svc -n brain-ai

# Check HPA
kubectl get hpa -n brain-ai

# Describe deployment
kubectl describe deployment brain-server -n brain-ai

# View logs
kubectl logs -f deployment/brain-server -n brain-ai

# Exec into pod
kubectl exec -it deployment/brain-server -n brain-ai -- /bin/sh
```

#### Scaling
```bash
# Manual scale
kubectl scale deployment brain-server --replicas=5 -n brain-ai

# Check HPA status
kubectl get hpa brain-server-hpa -n brain-ai --watch

# Edit HPA
kubectl edit hpa brain-server-hpa -n brain-ai
```

#### Port Forwarding
```bash
# Forward gRPC port
kubectl port-forward svc/brain-server 50051:50051 -n brain-ai

# Forward metrics port
kubectl port-forward svc/brain-server 9090:9090 -n brain-ai

# Test locally
grpc_health_probe -addr=localhost:50051
curl http://localhost:9090/metrics
```

#### Rolling Update
```bash
# Update image
kubectl set image deployment/brain-server \
    brain-server=brain-ai:v3.4.1 -n brain-ai

# Check rollout status
kubectl rollout status deployment/brain-server -n brain-ai

# Rollback if needed
kubectl rollout undo deployment/brain-server -n brain-ai

# View rollout history
kubectl rollout history deployment/brain-server -n brain-ai
```

---

## 🔧 Configuration

### quantum.yaml
```yaml
quantum:
  dimension: 7
  dt: 0.01
  decoherence_rate: 0.05
  entropy_threshold: 1.9459
  max_dwell_ms: 120.0
  collapse_rate_target_hz: 8.2
  trace_tolerance: 1.0e-9
  eigenvalue_floor: 1.0e-9

memory:
  stm_capacity: 96
  ltm:
    hot:
      capacity: 50000
      index: HNSW
      latency_budget_ms: 10
    warm:
      capacity: 300000
      index: IVF_PQ
      latency_budget_ms: 40
    cold:
      capacity: 2000000
      format: parquet
      async_mode: true

observability:
  metrics_port: 9090
  log_level: INFO
  enable_tracing: false
  
security:
  enable_signature_validation: true
  kill_switch_env: KILL
  seccomp_profile: /etc/brain/seccomp.json
```

### Environment Variables
```bash
# Kill switch
export KILL=1              # Trigger immediate shutdown

# Logging
export LOG_LEVEL=DEBUG     # DEBUG, INFO, WARN, ERROR

# Performance
export OMP_NUM_THREADS=8   # OpenMP thread count
export EIGEN_NUM_THREADS=8 # Eigen parallel threads

# Metrics
export METRICS_PORT=9090   # Prometheus metrics port

# Security
export CONFIG_PUBKEY=/etc/brain/pubkey.pem
```

---

## 📊 Monitoring & Observability

### Prometheus Metrics

**Key Metrics:**
```
# Quantum metrics
brain_entropy_current                  # Current von Neumann entropy
brain_collapse_rate_hz                 # Collapse frequency
brain_step_latency_seconds             # Step processing time

# Memory metrics
brain_memory_hot_size                  # Hot tier item count
brain_memory_warm_size                 # Warm tier item count
brain_memory_cold_size                 # Cold tier item count
brain_memory_hit_ratio                 # Cache hit rate

# Service metrics
brain_grpc_requests_total              # Total gRPC requests
brain_grpc_request_duration_seconds    # Request latency histogram
brain_grpc_errors_total                # Error count by method
```

**Query Examples:**
```promql
# Average step latency over 5m
rate(brain_step_latency_seconds_sum[5m]) / 
rate(brain_step_latency_seconds_count[5m])

# Collapse rate
rate(brain_collapse_total[1m]) * 60

# Memory usage over time
sum(brain_memory_hot_size + brain_memory_warm_size)

# Error rate
rate(brain_grpc_errors_total[5m])
```

### Grafana Dashboards

**Import Dashboard:**
```bash
# Use provided JSON
kubectl apply -f k8s/grafana-dashboard.yaml

# Or import manually
# Dashboard ID: TBD (upload to grafana.com)
```

**Key Panels:**
1. Step Latency (P50, P95, P99)
2. Collapse Rate vs Target (8.2 Hz)
3. Memory Tier Distribution
4. gRPC Request Rate
5. Error Rate
6. CPU/Memory Usage

### Structured Logging

**Log Format (JSON):**
```json
{
  "timestamp": "2025-10-28T10:30:15.123Z",
  "level": "INFO",
  "component": "QuantumWorkspace",
  "message": "Collapse triggered",
  "context": {
    "entropy": 1.9512,
    "threshold": 1.9459,
    "dwell_time_ms": 118.5,
    "dimension": 7
  }
}
```

**View Logs:**
```bash
# Docker
docker logs -f brain-server | jq .

# Kubernetes
kubectl logs -f deployment/brain-server -n brain-ai | jq .

# Filter by level
kubectl logs deployment/brain-server -n brain-ai | jq 'select(.level=="ERROR")'
```

---

## 🚨 Operations

### Health Checks

#### gRPC Health Probe
```bash
# Install grpc_health_probe
wget -qO /usr/local/bin/grpc_health_probe \
    https://github.com/grpc-ecosystem/grpc-health-probe/releases/latest/download/grpc_health_probe-linux-amd64
chmod +x /usr/local/bin/grpc_health_probe

# Check health
grpc_health_probe -addr=localhost:50051

# With timeout
grpc_health_probe -addr=localhost:50051 -connect-timeout=5s -rpc-timeout=5s
```

#### HTTP Status Endpoint
```bash
# Get status
curl http://localhost:9090/status

# Response
{
  "status": "SERVING",
  "version": "3.4.0",
  "uptime_seconds": 3600,
  "metrics": {
    "total_steps": 1000000,
    "total_collapses": 122000,
    "memory_total": 2350000
  }
}
```

### Graceful Shutdown

```bash
# Docker
docker stop brain-server  # SIGTERM, 10s grace period

# Kubernetes
kubectl delete pod <pod-name> -n brain-ai  # SIGTERM, 30s grace period

# Manual kill
kill -SIGTERM <pid>
```

### Backup & Restore

#### Backup
```bash
# Backup memory state
kubectl exec deployment/brain-server -n brain-ai -- \
    /usr/local/bin/brain_cli backup --output /data/backup.tar.gz

# Copy from pod
kubectl cp brain-ai/brain-server-xxxxx:/data/backup.tar.gz ./backup.tar.gz

# Backup config
kubectl get configmap brain-config -n brain-ai -o yaml > config-backup.yaml
```

#### Restore
```bash
# Copy to pod
kubectl cp ./backup.tar.gz brain-ai/brain-server-xxxxx:/data/backup.tar.gz

# Restore
kubectl exec deployment/brain-server -n brain-ai -- \
    /usr/local/bin/brain_cli restore --input /data/backup.tar.gz
```

---

## 🐛 Troubleshooting

### Common Issues

#### 1. Server Won't Start

**Symptoms:**
- Container exits immediately
- "Config validation failed" error

**Solutions:**
```bash
# Validate config
./build/tools/validate_config configs/quantum.yaml

# Check signature
openssl dgst -sha256 -verify pubkey.pem -signature quantum.yaml.sig quantum.yaml

# Disable signature check (dev only)
export SKIP_SIG_VALIDATION=1
```

#### 2. High Step Latency

**Symptoms:**
- Step latency >10ms
- Prometheus alerts firing

**Solutions:**
```bash
# Check CPU usage
kubectl top pods -n brain-ai

# Scale up
kubectl scale deployment brain-server --replicas=5 -n brain-ai

# Check for resource limits
kubectl describe pod <pod-name> -n brain-ai | grep -A 5 Resources

# Profile performance
./build/tools/profile_server --duration=60s
```

#### 3. Memory Leaks

**Symptoms:**
- RSS growing over time
- OOM kills

**Solutions:**
```bash
# Check memory usage
kubectl top pods -n brain-ai

# Run valgrind (local)
valgrind --leak-check=full --show-leak-kinds=all \
    ./build/kernel/brain_server --config configs/quantum.yaml

# Heap profiling
HEAPPROFILE=/tmp/heap.prof ./build/kernel/brain_server
google-pprof --text /tmp/heap.prof.*
```

#### 4. gRPC Connection Refused

**Symptoms:**
- Clients can't connect
- "Connection refused" errors

**Solutions:**
```bash
# Check if port is listening
netstat -tuln | grep 50051

# Check firewall
sudo ufw status
sudo ufw allow 50051/tcp

# Check service
kubectl get svc brain-server -n brain-ai
kubectl describe svc brain-server -n brain-ai

# Port forward and test
kubectl port-forward svc/brain-server 50051:50051 -n brain-ai
grpc_health_probe -addr=localhost:50051
```

#### 5. Qdrant Connection Fails

**Symptoms:**
- "Failed to connect to Qdrant" errors
- RAG retrieval failures

**Solutions:**
```bash
# Check Qdrant health
curl http://localhost:6333/health

# Check collections
curl http://localhost:6333/collections

# Restart Qdrant
kubectl rollout restart deployment/qdrant -n brain-ai

# Check network policy
kubectl get networkpolicy -n brain-ai
```

### Debug Mode

```bash
# Enable debug logging
kubectl set env deployment/brain-server LOG_LEVEL=DEBUG -n brain-ai

# View detailed logs
kubectl logs -f deployment/brain-server -n brain-ai | grep -i error

# Exec into container
kubectl exec -it deployment/brain-server -n brain-ai -- /bin/sh

# Check internal state
./brain_cli status
./brain_cli dump-state > state.json
```

---

## 🔒 Security

### Signature Validation

```bash
# Generate key pair
openssl genpkey -algorithm Ed25519 -out privkey.pem
openssl pkey -in privkey.pem -pubout -out pubkey.pem

# Sign config
openssl dgst -sha256 -sign privkey.pem -out quantum.yaml.sig quantum.yaml

# Verify (done automatically by server)
openssl dgst -sha256 -verify pubkey.pem -signature quantum.yaml.sig quantum.yaml
```

### Kill Switch

```bash
# Trigger kill switch
kubectl set env deployment/brain-server KILL=1 -n brain-ai

# Verify shutdown
kubectl get pods -n brain-ai

# Remove kill switch
kubectl set env deployment/brain-server KILL- -n brain-ai
```

### Security Scanning

```bash
# Scan Docker image
trivy image brain-ai:v3.4.0

# Scan for secrets
trufflehog filesystem --directory=.

# Scan dependencies
syft brain-ai:v3.4.0 -o spdx-json | grype
```

---

## 📈 Performance Tuning

### CPU Optimization

```bash
# Set thread counts
export OMP_NUM_THREADS=$(nproc)
export EIGEN_NUM_THREADS=$(nproc)

# CPU affinity (Docker)
docker run --cpuset-cpus="0-7" brain-ai:v3.4.0

# CPU limits (Kubernetes)
kubectl set resources deployment brain-server \
    --limits=cpu=8,memory=4Gi \
    --requests=cpu=4,memory=2Gi \
    -n brain-ai
```

### Memory Tuning

```bash
# Increase memory limits
kubectl set resources deployment brain-server \
    --limits=memory=8Gi \
    -n brain-ai

# Tune FAISS parameters
# Edit configs/quantum.yaml
memory:
  ltm:
    hot:
      hnsw_M: 64  # Increase for better recall
```

### GPU Acceleration (Optional)

```bash
# Use GPU image
kubectl set image deployment/brain-server \
    brain-server=brain-ai:v3.4.0-gpu -n brain-ai

# Verify GPU usage
kubectl exec deployment/brain-server -n brain-ai -- nvidia-smi
```

---

## 📞 Support

### Getting Help

- **Documentation:** https://github.com/dawsonblock/The-human-ai-brain/wiki
- **Issues:** https://github.com/dawsonblock/The-human-ai-brain/issues
- **Discussions:** https://github.com/dawsonblock/The-human-ai-brain/discussions

### Reporting Issues

**Include:**
1. Version: `./brain_server --version`
2. Config: `quantum.yaml`
3. Logs: Last 100 lines
4. Metrics: Prometheus snapshot
5. Environment: OS, CPU, RAM, GPU

---

## 📋 Checklist

### Pre-Deployment
- [ ] Config validated
- [ ] Tests passing
- [ ] Security scan clean
- [ ] Backup created
- [ ] Monitoring configured

### Post-Deployment
- [ ] Health checks passing
- [ ] Metrics flowing to Prometheus
- [ ] Logs structured and indexed
- [ ] Alerts configured
- [ ] Load test passed
- [ ] Rollback plan ready

---

**Runbook Version:** 1.0  
**Brain Version:** 3.4.0-prod  
**Maintained By:** Platform Team
