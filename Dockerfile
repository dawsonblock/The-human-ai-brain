# Multi-stage build for Human-AI Brain System
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    libeigen3-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgrpc++-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libpoppler-cpp-dev \
    libgtest-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Build GTest
RUN cd /usr/src/gtest && \
    cmake . && \
    make && \
    cp lib/*.a /usr/lib/ || cp *.a /usr/lib/

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Configure and build
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_GRPC_SERVER=ON && \
    cmake --build build -j$(nproc)

# Run tests during build (optional, comment out if you want faster builds)
RUN cd build && ctest --output-on-failure

# ============================================================================
# Runtime stage - minimal image with only runtime dependencies
# ============================================================================
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libprotobuf23 \
    libgrpc++1.45 \
    libssl3 \
    libcurl4 \
    libpoppler-cpp0v5 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1000 -s /bin/bash brainuser

# Set working directory
WORKDIR /app

# Copy binaries from builder
COPY --from=builder /build/build/kernel/brain_server /app/brain_server
COPY --from=builder /build/build/kernel/interactive_demo /app/interactive_demo

# Copy configuration files
COPY --from=builder /build/configs /app/configs

# Change ownership
RUN chown -R brainuser:brainuser /app

# Switch to non-root user
USER brainuser

# Expose gRPC port
EXPOSE 50051

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD grpc_health_probe -addr=localhost:50051 || exit 1

# Default command
CMD ["/app/brain_server"]

# ============================================================================
# Development stage - includes build tools and source code
# ============================================================================
FROM builder AS development

WORKDIR /workspace

# Copy full source
COPY . .

# Install additional development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    clang-format \
    clang-tidy \
    vim \
    && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]
