#!/bin/bash
# deploy.sh - Deployment script for Human-AI Brain System
set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
DEPLOYMENT_MODE="${1:-local}"
DOCKER_REGISTRY="${DOCKER_REGISTRY:-localhost:5000}"
IMAGE_TAG="${IMAGE_TAG:-latest}"
COMPOSE_PROFILE="${COMPOSE_PROFILE:-default}"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Human-AI Brain Deployment${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Mode: $DEPLOYMENT_MODE"
echo "Registry: $DOCKER_REGISTRY"
echo "Image Tag: $IMAGE_TAG"
echo "Compose Profile: $COMPOSE_PROFILE"
echo ""

deploy_local() {
    echo -e "${YELLOW}Deploying locally...${NC}"
    
    # Build
    echo "Building binaries..."
    ./scripts/build.sh
    
    # Create runtime directories
    mkdir -p runtime/{data,logs,audit}
    
    # Copy binaries
    echo "Installing binaries..."
    cp build/kernel/brain_server runtime/
    cp build/kernel/interactive_demo runtime/
    
    # Copy configs
    echo "Installing configs..."
    cp -r configs runtime/
    
    echo -e "${GREEN}✓ Local deployment complete${NC}"
    echo ""
    echo "To start the server:"
    echo "  cd runtime && ./brain_server"
}

deploy_docker() {
    echo -e "${YELLOW}Deploying with Docker...${NC}"
    
    # Build Docker image
    echo "Building Docker image..."
    docker build -t brain-system:$IMAGE_TAG .
    
    if [ "$DOCKER_REGISTRY" != "localhost:5000" ]; then
        echo "Tagging and pushing to registry..."
        docker tag brain-system:$IMAGE_TAG $DOCKER_REGISTRY/brain-system:$IMAGE_TAG
        docker push $DOCKER_REGISTRY/brain-system:$IMAGE_TAG
    fi
    
    # Start services
    echo "Starting Docker Compose services..."
    docker-compose --profile $COMPOSE_PROFILE up -d
    
    echo -e "${GREEN}✓ Docker deployment complete${NC}"
    echo ""
    echo "Services running:"
    docker-compose ps
}

deploy_kubernetes() {
    echo -e "${YELLOW}Deploying to Kubernetes...${NC}"
    
    # Check kubectl
    if ! command -v kubectl &> /dev/null; then
        echo -e "${RED}Error: kubectl not found${NC}"
        exit 1
    fi
    
    # Build and push image
    echo "Building and pushing Docker image..."
    docker build -t $DOCKER_REGISTRY/brain-system:$IMAGE_TAG .
    docker push $DOCKER_REGISTRY/brain-system:$IMAGE_TAG
    
    # Apply Kubernetes manifests
    echo "Applying Kubernetes manifests..."
    kubectl apply -f k8s/namespace.yaml
    kubectl apply -f k8s/configmap.yaml
    kubectl apply -f k8s/deployment.yaml
    kubectl apply -f k8s/service.yaml
    
    # Wait for deployment
    echo "Waiting for deployment to be ready..."
    kubectl wait --for=condition=available --timeout=300s \
        deployment/brain-server -n brain-system
    
    echo -e "${GREEN}✓ Kubernetes deployment complete${NC}"
    echo ""
    echo "Deployment status:"
    kubectl get pods -n brain-system
}

deploy_systemd() {
    echo -e "${YELLOW}Deploying as systemd service...${NC}"
    
    # Build
    ./scripts/build.sh
    
    # Install binaries
    sudo mkdir -p /opt/brain-system/bin
    sudo mkdir -p /opt/brain-system/configs
    sudo mkdir -p /var/lib/brain-system/data
    sudo mkdir -p /var/log/brain-system
    
    sudo cp build/kernel/brain_server /opt/brain-system/bin/
    sudo cp -r configs/* /opt/brain-system/configs/
    
    # Create systemd service
    cat << EOF | sudo tee /etc/systemd/system/brain-server.service
[Unit]
Description=Human-AI Brain gRPC Server
After=network.target

[Service]
Type=simple
User=brain
Group=brain
WorkingDirectory=/opt/brain-system
ExecStart=/opt/brain-system/bin/brain_server
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/brain-system /var/log/brain-system

[Install]
WantedBy=multi-user.target
EOF
    
    # Create user
    sudo useradd -r -s /bin/false brain || true
    sudo chown -R brain:brain /opt/brain-system
    sudo chown -R brain:brain /var/lib/brain-system
    sudo chown -R brain:brain /var/log/brain-system
    
    # Enable and start service
    sudo systemctl daemon-reload
    sudo systemctl enable brain-server
    sudo systemctl start brain-server
    
    echo -e "${GREEN}✓ systemd deployment complete${NC}"
    echo ""
    echo "Service status:"
    sudo systemctl status brain-server
}

# Main deployment logic
case "$DEPLOYMENT_MODE" in
    local)
        deploy_local
        ;;
    docker)
        deploy_docker
        ;;
    k8s|kubernetes)
        deploy_kubernetes
        ;;
    systemd)
        deploy_systemd
        ;;
    *)
        echo -e "${RED}Error: Unknown deployment mode: $DEPLOYMENT_MODE${NC}"
        echo "Usage: $0 {local|docker|kubernetes|systemd}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Deployment Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
