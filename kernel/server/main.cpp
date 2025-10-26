#include "brain/brain_system.hpp"
#include <grpcpp/grpcpp.h>
#include "brain.grpc.pb.h"
#include <iostream>
#include <csignal>
#include <memory>
#include <mutex>

// Forward declaration from brain_service.cpp
void RunServer(const std::string& server_address, const hab::BrainConfig& config);

// Global server pointer for signal handling
std::unique_ptr<grpc::Server> g_server;
std::mutex g_server_mutex;

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    
    std::lock_guard<std::mutex> lock(g_server_mutex);
    if (g_server) {
        g_server->Shutdown();
    }
}

int main(int argc, char** argv) {
    // Parse command line arguments
    std::string server_address = "0.0.0.0:50051";
    if (argc > 1) {
        server_address = argv[1];
    }
    
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   Human-AI Brain FDQC v3.1.0 - gRPC Server             ║" << std::endl;
    std::cout << "║   Finite-Dimensional Quantum Consciousness             ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // Configure brain system
    hab::BrainConfig config;
    
    // GW Configuration
    config.gw_config.input_dim = 28 * 28;  // MNIST default
    config.gw_config.output_dim = 60;
    config.gw_config.hidden_dim = 128;
    config.gw_config.sparse_k = 12;
    config.gw_config.learning_rate = 1e-3;
    
    // QW Configuration
    config.qw_config.dimension = 7;
    config.qw_config.dt = 0.001;
    config.qw_config.entropy_cap = std::log(7.0);
    config.qw_config.decoherence_rate = 1e-8;
    config.qw_config.max_dwell_ms = 120;
    
    // Memory Configuration
    config.memory_config.stm_config.capacity = 20;
    config.memory_config.ltm_config.max_memories = 1000;
    config.memory_config.ltm_config.importance_threshold = 0.5;
    
    // Affective Configuration
    config.affective_config.alpha_valence = 0.9;
    config.affective_config.alpha_arousal = 0.8;
    config.affective_config.alpha_novelty = 0.7;
    
    // Epistemic Configuration
    config.epistemic_config.crisis_threshold = 5.0;  // 5σ
    config.epistemic_config.buffer_size = 100;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  ├─ Input Dimension: " << config.gw_config.input_dim << std::endl;
    std::cout << "  ├─ GW Output Dimension: " << config.gw_config.output_dim << std::endl;
    std::cout << "  ├─ QW Dimension: " << config.qw_config.dimension << std::endl;
    std::cout << "  ├─ Entropy Cap: " << config.qw_config.entropy_cap << " (" 
              << config.qw_config.entropy_cap / std::log(config.qw_config.dimension) * 100.0 
              << "% of max)" << std::endl;
    std::cout << "  ├─ Max Dwell: " << config.qw_config.max_dwell_ms << " ms" << std::endl;
    std::cout << "  ├─ STM Capacity: " << config.memory_config.stm_config.capacity << " episodes" << std::endl;
    std::cout << "  ├─ LTM Capacity: " << config.memory_config.ltm_config.max_memories << " schemas" << std::endl;
    std::cout << "  └─ Epistemic Crisis: " << config.epistemic_config.crisis_threshold << "σ" << std::endl;
    std::cout << std::endl;
    
    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    std::cout << "Starting gRPC server on " << server_address << "..." << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  ├─ Health()   - Check server health and uptime" << std::endl;
    std::cout << "  ├─ Step()     - Execute cognitive cycle with sensory input" << std::endl;
    std::cout << "  └─ GetState() - Query quantum density matrix and memory state" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    try {
        // Run server (blocking call)
        RunServer(server_address, config);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}
