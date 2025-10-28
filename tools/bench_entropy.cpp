#include "brain/qw.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace hab;
using namespace std::chrono;

struct BenchmarkResult {
    double avg_step_latency_ms;
    double min_step_latency_ms;
    double max_step_latency_ms;
    double throughput_steps_per_sec;
    int total_steps;
    int collapses;
};

BenchmarkResult benchmark_quantum_workspace(int num_steps, bool enable_threads) {
    QWConfig config;
    config.dimension = 7;
    config.dt = 0.01;
    config.decoherence_rate = 0.05;
    config.entropy_threshold = 1.9459;
    config.max_dwell_ms = 120.0;
    
    QuantumWorkspace qw(config);
    if (enable_threads) {
        qw.enable_threading(-1);  // Use all available threads
    }
    
    std::vector<double> step_latencies;
    step_latencies.reserve(num_steps);
    
    int collapse_count = 0;
    auto start_total = steady_clock::now();
    
    for (int i = 0; i < num_steps; ++i) {
        bool was_collapsed_before = qw.has_collapsed();
        
        auto start_step = steady_clock::now();
        qw.step_ticks(1);
        auto end_step = steady_clock::now();
        
        double latency_ms = duration_cast<nanoseconds>(end_step - start_step).count() / 1e6;
        step_latencies.push_back(latency_ms);
        
        if (!was_collapsed_before && qw.has_collapsed()) {
            collapse_count++;
        }
    }
    
    auto end_total = steady_clock::now();
    double total_time_s = duration_cast<microseconds>(end_total - start_total).count() / 1e6;
    
    // Compute statistics
    double sum = 0.0;
    double min_latency = step_latencies[0];
    double max_latency = step_latencies[0];
    
    for (double lat : step_latencies) {
        sum += lat;
        min_latency = std::min(min_latency, lat);
        max_latency = std::max(max_latency, lat);
    }
    
    return {
        .avg_step_latency_ms = sum / num_steps,
        .min_step_latency_ms = min_latency,
        .max_step_latency_ms = max_latency,
        .throughput_steps_per_sec = num_steps / total_time_s,
        .total_steps = num_steps,
        .collapses = collapse_count
    };
}

int main(int argc, char* argv[]) {
    int num_steps = 1000;
    if (argc > 1) {
        num_steps = std::atoi(argv[1]);
    }
    
    std::cout << "========================================\n";
    std::cout << "Quantum Workspace Performance Benchmark\n";
    std::cout << "========================================\n\n";
    
    std::cout << "Configuration:\n";
    std::cout << "  Dimension: 7\n";
    std::cout << "  Steps: " << num_steps << "\n";
    std::cout << "  dt: 0.01 (10ms)\n\n";
    
    // Single-threaded benchmark
    std::cout << "Running single-threaded benchmark...\n";
    auto result_single = benchmark_quantum_workspace(num_steps, false);
    
    std::cout << "\n📊 Single-Threaded Results:\n";
    std::cout << "  Avg step latency: " << std::fixed << std::setprecision(3) 
              << result_single.avg_step_latency_ms << " ms\n";
    std::cout << "  Min step latency: " << result_single.min_step_latency_ms << " ms\n";
    std::cout << "  Max step latency: " << result_single.max_step_latency_ms << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
              << result_single.throughput_steps_per_sec << " steps/sec\n";
    std::cout << "  Collapses: " << result_single.collapses << "\n\n";
    
    // Multi-threaded benchmark
    std::cout << "Running multi-threaded benchmark...\n";
    auto result_multi = benchmark_quantum_workspace(num_steps, true);
    
    std::cout << "\n📊 Multi-Threaded Results:\n";
    std::cout << "  Avg step latency: " << std::fixed << std::setprecision(3) 
              << result_multi.avg_step_latency_ms << " ms\n";
    std::cout << "  Min step latency: " << result_multi.min_step_latency_ms << " ms\n";
    std::cout << "  Max step latency: " << result_multi.max_step_latency_ms << " ms\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) 
              << result_multi.throughput_steps_per_sec << " steps/sec\n";
    std::cout << "  Collapses: " << result_multi.collapses << "\n\n";
    
    // Speedup
    double speedup = result_multi.throughput_steps_per_sec / result_single.throughput_steps_per_sec;
    std::cout << "⚡ Multi-threading speedup: " << std::fixed << std::setprecision(2) 
              << speedup << "x\n\n";
    
    // Performance targets
    std::cout << "========================================\n";
    std::cout << "Performance Targets (v3.4.0):\n";
    std::cout << "========================================\n";
    std::cout << "  CPU: <10ms avg step latency\n";
    std::cout << "  GPU: <2ms avg step latency\n";
    std::cout << "  Throughput: >2000 steps/sec\n\n";
    
    bool meets_cpu_target = result_multi.avg_step_latency_ms < 10.0;
    bool meets_throughput_target = result_multi.throughput_steps_per_sec > 2000.0;
    
    if (meets_cpu_target && meets_throughput_target) {
        std::cout << "✅ All CPU performance targets MET!\n";
        return 0;
    } else {
        std::cout << "⚠️  Performance targets NOT MET:\n";
        if (!meets_cpu_target) {
            std::cout << "  ❌ CPU latency: " << result_multi.avg_step_latency_ms 
                      << " ms (target: <10ms)\n";
        }
        if (!meets_throughput_target) {
            std::cout << "  ❌ Throughput: " << result_multi.throughput_steps_per_sec 
                      << " steps/sec (target: >2000)\n";
        }
        return 1;
    }
}
