#include "brain/qw.hpp"
#include "brain/gw.hpp"
#include "brain/memory.hpp"
#include "brain/wiring.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace hab;
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::micro>;

template<typename Func>
double benchmark(const std::string& name, Func&& func, int iterations = 1000) {
    // Warmup
    func();
    
    auto start = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = Clock::now();
    
    double avg_us = std::chrono::duration_cast<Duration>(end - start).count() / iterations;
    std::cout << std::setw(40) << std::left << name 
              << std::setw(12) << std::right << std::fixed << std::setprecision(2) 
              << avg_us << " µs  (" 
              << std::setw(8) << (1e6 / avg_us) << " ops/sec)" << std::endl;
    return avg_us;
}

int main() {
    std::cout << "=== Human-AI Brain FDQC v3 Performance Benchmark ===" << std::endl;
    std::cout << std::endl;
    
    // Quantum Workspace benchmarks
    std::cout << "### Quantum Workspace (7D)" << std::endl;
    {
        QWConfig config;
        QuantumWorkspace qw(config);
        
        benchmark("QW: Single step (full)", [&]() {
            qw.step_ticks(1);
        }, 1000);
        
        benchmark("QW: Entropy computation", [&]() {
            qw.entropy();
        }, 5000);
        
        Eigen::VectorXd gw_state = Eigen::VectorXd::Random(60);
        benchmark("QW: Project from GW", [&]() {
            qw.project_from_gw(gw_state);
        }, 1000);
    }
    
    std::cout << std::endl;
    
    // Global Workspace benchmarks
    std::cout << "### Global Workspace (60D)" << std::endl;
    {
        GWConfig config;
        GlobalWorkspace gw(config);
        
        Eigen::VectorXd input = Eigen::VectorXd::Random(60);
        benchmark("GW: Forward pass", [&]() {
            gw.forward(input);
        }, 1000);
        
        Eigen::VectorXd state = Eigen::VectorXd::Random(60);
        benchmark("GW: Compute entropy", [&]() {
            gw.compute_entropy(state);
        }, 5000);
    }
    
    std::cout << std::endl;
    
    // Memory benchmarks
    std::cout << "### Memory System" << std::endl;
    {
        STMConfig stm_config;
        stm_config.capacity = 20;
        ShortTermMemory stm(stm_config);
        
        LTMConfig ltm_config;
        ltm_config.max_schemas = 1000;
        LongTermMemory ltm(ltm_config);
        
        Eigen::VectorXd gw_state = Eigen::VectorXd::Random(60);
        Eigen::VectorXd qw_onehot = Eigen::VectorXd::Zero(7);
        qw_onehot(3) = 1.0;
        
        Episode ep{gw_state, qw_onehot, 0, 1.0, std::chrono::steady_clock::now()};
        
        benchmark("STM: Add episode", [&]() {
            stm.add(ep);
        }, 1000);
        
        benchmark("STM: Recall", [&]() {
            stm.recall(gw_state, 10.0);
        }, 1000);
        
        benchmark("LTM: Consolidate", [&]() {
            ltm.consolidate(ep, 1.0);
        }, 1000);
        
        benchmark("LTM: Retrieve", [&]() {
            ltm.retrieve(gw_state, 5);
        }, 1000);
    }
    
    std::cout << std::endl;
    
    // Self-Wiring benchmarks
    std::cout << "### Self-Wiring System" << std::endl;
    {
        WiringConfig config;
        SelfWiring wiring(60, config);
        
        Eigen::VectorXd g_t = Eigen::VectorXd::Random(60);
        Eigen::VectorXd r_t = Eigen::VectorXd::Random(60);
        
        benchmark("Wiring: Apply", [&]() {
            wiring.apply(g_t, r_t);
        }, 1000);
        
        benchmark("Wiring: Hebbian update", [&]() {
            wiring.update(g_t, r_t, 1.5);
        }, 1000);
    }
    
    std::cout << std::endl;
    
    // Integrated pipeline benchmark
    std::cout << "### Integrated Pipeline" << std::endl;
    {
        QWConfig qw_config;
        QuantumWorkspace qw(qw_config);
        
        GWConfig gw_config;
        GlobalWorkspace gw(gw_config);
        
        STMConfig stm_config;
        stm_config.capacity = 20;
        ShortTermMemory stm(stm_config);
        
        Eigen::VectorXd sensory_input = Eigen::VectorXd::Random(60);
        
        double total_us = benchmark("Full tick (QW + GW + Memory)", [&]() {
            // GW forward pass
            Eigen::VectorXd gw_output = gw.forward(sensory_input);
            
            // QW projection and evolution
            qw.project_from_gw(gw_output);
            qw.step_ticks(1);
            
            // Memory storage
            Eigen::VectorXd qw_onehot = qw.get_one_hot();
            Episode ep{gw_output, qw_onehot, 0, 1.0, std::chrono::steady_clock::now()};
            stm.add(ep);
        }, 500);
        
        double throughput_hz = 1e6 / total_us;
        std::cout << "\nEstimated system throughput: " << std::fixed << std::setprecision(0) 
                  << throughput_hz << " Hz" << std::endl;
        std::cout << "Time per consciousness cycle: " << std::fixed << std::setprecision(2) 
                  << total_us / 1000.0 << " ms" << std::endl;
    }
    
    std::cout << "\n=== Benchmark Complete ===" << std::endl;
    return 0;
}
