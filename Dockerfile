# Multi-stage Dockerfile for Human-AI Brain FDQC v3.1.0

# ============================================================================
# Stage 1: Builder - Compile the brain system
# ============================================================================
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    libeigen3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build the project
RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GRPC_SERVER=ON \
    -DBUILD_TESTS=OFF \
    -DENABLE_OPENMP=ON

RUN cmake --build build --target brain_server -j$(nproc)
RUN cmake --build build --target interactive_demo -j$(nproc)

# ============================================================================
# Stage 2: Runtime - Minimal image with only runtime dependencies
# ============================================================================
FROM debian:bookworm-slim AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libgrpc++1.51 \
    libprotobuf32 \
    libssl3 \
    libgomp1 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -m -u 1000 brain && \
    mkdir -p /app /data /checkpoints && \
    chown -R brain:brain /app /data /checkpoints

# Set working directory
WORKDIR /app

# Copy binaries from builder
COPY --from=builder --chown=brain:brain /build/build/kernel/brain_server /app/
COPY --from=builder --chown=brain:brain /build/build/kernel/interactive_demo /app/

# Copy configuration files

# Switch to app user
USER brain

# Expose gRPC port
EXPOSE 50051

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD timeout 2s bash -c 'exec 3<>/dev/tcp/localhost/50051 && echo -e "\\x00\\x00\\x00\\x00\\x00" >&3 && exec 3<&- && exec 3>&-' || exit 1

# Default command: Run gRPC server
CMD ["/app/brain_server", "0.0.0.0:50051"]

# ============================================================================
# Stage 3: Development - Include development tools
# ============================================================================
FROM builder AS development

# Install additional development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    clang-format \
    clang-tidy \
    doxygen \
    graphviz \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
RUN pip3 install --no-cache-dir \
    grpcio \
    grpcio-tools \
    numpy \
    matplotlib

WORKDIR /workspace

# Copy test client
COPY test_client.py /workspace/

CMD ["/bin/bash"]

# ============================================================================
# Build instructions:
#
# Production image:
#   docker build --target runtime -t brain-fdqc:latest .
#
# Development image:
#   docker build --target development -t brain-fdqc:dev .
#
# Run server:
#   docker run -p 50051:50051 brain-fdqc:latest
#
# Run demo:
#   docker run -it brain-fdqc:latest /app/interactive_demo
#
# Custom address:
#   docker run -p 8080:8080 brain-fdqc:latest /app/brain_server 0.0.0.0:8080
# ============================================================================
