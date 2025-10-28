# Kubernetes Deployment Guide - v3.4.0

This directory contains production-ready Kubernetes manifests for deploying the Human-AI Brain System.

## Prerequisites

- Kubernetes cluster (v1.25+)
- `kubectl` configured with cluster access
- Prometheus Adapter (optional, for custom metrics scaling)
- Seccomp profile support in cluster

## Quick Start

### 1. Create Namespace

```bash
kubectl apply -f namespace.yaml
```

### 2. Apply Configurations

```bash
# Apply all manifests
kubectl apply -f configmap.yaml
kubectl apply -f deployment.yaml
kubectl apply -f service.yaml
kubectl apply -f hpa.yaml
```

### 3. Verify Deployment

```bash
# Check pod status
kubectl get pods -n brain-prod

# Check services
kubectl get svc -n brain-prod

# Check HPA
kubectl get hpa -n brain-prod

# View logs
kubectl logs -n brain-prod -l app=brain-system --tail=100
```

## Architecture

### Components

| Component | Purpose | Port |
|-----------|---------|------|
| brain-server | Core quantum workspace & gRPC API | 50051 |
| metrics | Prometheus metrics endpoint | 9090 |

### Resource Allocation

**Per Pod:**
- CPU Request: 500m
- CPU Limit: 2000m
- Memory Request: 512Mi
- Memory Limit: 2Gi

**Cluster Total (3-10 replicas):**
- CPU: 1.5-20 cores
- Memory: 1.5-20 GB

## Security

### Seccomp Profile

The deployment uses a custom seccomp profile (`brain-seccomp.json`) to restrict syscalls. Copy the profile to all nodes:

```bash
# On each node:
sudo mkdir -p /var/lib/kubelet/seccomp
sudo cp security/seccomp-profile.json /var/lib/kubelet/seccomp/brain-seccomp.json
```

### Read-Only Root Filesystem

The container runs with a read-only root filesystem. Writable directories:
- `/tmp` - Temporary files (100Mi limit)
- `/app/logs` - Application logs (1Gi limit)

### Non-Root User

The container runs as user `65534` (nobody) with no privilege escalation.

## Configuration

### Edit ConfigMap

```bash
kubectl edit configmap brain-config -n brain-prod
```

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `BRAIN_CONFIG` | `/app/config/quantum.yaml` | Config file path |
| `BRAIN_LOG_LEVEL` | `info` | Log level (debug/info/warn/error) |
| `BRAIN_METRICS_PORT` | `9090` | Metrics HTTP port |
| `BRAIN_GRPC_PORT` | `50051` | gRPC service port |
| `CONFIG_SIGNATURE_REQUIRED` | `0` | Enable Ed25519 signature validation |
| `SECCOMP_ENABLED` | `1` | Enable seccomp filtering |
| `KILL` | `0` | Emergency kill-switch |

### Kill-Switch Activation

To gracefully shutdown all pods:

```bash
kubectl patch configmap brain-config -n brain-prod -p '{"data":{"kill_switch":"1"}}'
kubectl rollout restart deployment brain-system -n brain-prod
```

## Scaling

### Manual Scaling

```bash
kubectl scale deployment brain-system -n brain-prod --replicas=5
```

### Auto-Scaling

The HPA automatically scales between 3-10 replicas based on:
- CPU utilization (target: 70%)
- Memory utilization (target: 80%)
- Custom metric: `brain_step_latency_ms_avg` (target: <5ms)

**Scale-up:** Fast (30s stabilization, +100% or +2 pods)  
**Scale-down:** Gradual (300s stabilization, -50% or -1 pod)

## Monitoring

### Prometheus Integration

The deployment is annotated for Prometheus scraping:

```yaml
annotations:
  prometheus.io/scrape: "true"
  prometheus.io/port: "9090"
  prometheus.io/path: "/metrics"
```

### Key Metrics

- `brain_step_latency_ms` - Step execution latency
- `brain_entropy` - Current quantum entropy
- `brain_collapses_total` - Total collapse events
- `brain_steps_total` - Total simulation steps
- `memory_retrieve_latency_ms` - Memory retrieval latency
- `memory_hot_tier_hits_total` - Cache hits

### Access Metrics

```bash
# Port-forward to metrics endpoint
kubectl port-forward -n brain-prod svc/brain-system-metrics 9090:9090

# View metrics
curl http://localhost:9090/metrics
```

## Health Checks

### Liveness Probe
- Endpoint: `http://localhost:9090/health`
- Initial delay: 10s
- Period: 30s
- Failure threshold: 3

### Readiness Probe
- Endpoint: `http://localhost:9090/health`
- Initial delay: 5s
- Period: 10s
- Failure threshold: 3

## Troubleshooting

### Pods Not Starting

```bash
# Check pod events
kubectl describe pod -n brain-prod -l app=brain-system

# Check logs
kubectl logs -n brain-prod -l app=brain-system --tail=100
```

### Common Issues

#### Seccomp Profile Not Found

```
Error: failed to load seccomp profile "brain-seccomp.json"
```

**Solution:** Copy seccomp profile to all nodes at `/var/lib/kubelet/seccomp/brain-seccomp.json`

#### Image Pull Errors

```
Error: ErrImagePull
```

**Solution:** Check image registry authentication:

```bash
kubectl create secret docker-registry ghcr-secret \
  --docker-server=ghcr.io \
  --docker-username=<username> \
  --docker-password=<token> \
  -n brain-prod

# Update deployment to use secret
kubectl patch serviceaccount default -n brain-prod \
  -p '{"imagePullSecrets":[{"name":"ghcr-secret"}]}'
```

#### High CPU Usage

```bash
# Check current resource usage
kubectl top pods -n brain-prod

# Increase resource limits
kubectl edit deployment brain-system -n brain-prod
```

### Logs

```bash
# Follow logs
kubectl logs -n brain-prod -l app=brain-system -f

# Logs from specific pod
kubectl logs -n brain-prod <pod-name>

# Logs from all replicas
kubectl logs -n brain-prod -l app=brain-system --all-containers=true
```

## Rolling Updates

### Update Image

```bash
kubectl set image deployment/brain-system -n brain-prod \
  brain-server=ghcr.io/dawsonblock/the-human-ai-brain/brain:v3.4.1
```

### Rollback

```bash
# View rollout history
kubectl rollout history deployment brain-system -n brain-prod

# Rollback to previous version
kubectl rollout undo deployment brain-system -n brain-prod

# Rollback to specific revision
kubectl rollout undo deployment brain-system -n brain-prod --to-revision=2
```

## Cleanup

```bash
# Delete all resources
kubectl delete -f hpa.yaml
kubectl delete -f service.yaml
kubectl delete -f deployment.yaml
kubectl delete -f configmap.yaml
kubectl delete -f namespace.yaml
```

## Production Checklist

- [ ] Seccomp profiles installed on all nodes
- [ ] Resource limits appropriate for workload
- [ ] Prometheus metrics collection configured
- [ ] HPA metrics adapter installed (for custom metrics)
- [ ] Image registry authentication configured
- [ ] Backup strategy for logs/data
- [ ] Monitoring alerts configured
- [ ] Runbook procedures documented
- [ ] Disaster recovery plan tested

## Support

For issues or questions:
- GitHub Issues: https://github.com/dawsonblock/The-human-ai-brain/issues
- Documentation: [RUNBOOK.md](../RUNBOOK.md)
