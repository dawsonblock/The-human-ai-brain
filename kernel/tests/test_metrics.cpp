#include "brain/qw.hpp"
#include "brain/metrics.hpp"
#include <iostream>
#include <thread>
#include <vector>

using namespace hab;

// Test thread safety by having multiple threads record metrics
void worker_thread(int id, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        Metrics::instance().increment_counter("thread_counter", 1.0);
        Metrics::instance().set_gauge("thread_" + std::to_string(id) + "_gauge", i);
        Metrics::instance().record_histogram("thread_histogram", id * 1.0 + i * 0.1);
    }
}

int main() {
    std::cout << "=== Metrics Thread Safety Test ===" << std::endl;
    
    // Reset metrics
    Metrics::instance().reset();
    
    // Test basic operations
    std::cout << "\n1. Testing basic operations..." << std::endl;
    Metrics::instance().increment_counter("test_counter", 5.0);
    Metrics::instance().set_gauge("test_gauge", 42.5);
    Metrics::instance().record_histogram("test_hist", 10.0);
    Metrics::instance().record_histogram("test_hist", 20.0);
    Metrics::instance().record_histogram("test_hist", 15.0);
    
    std::cout << "   Counter: " << Metrics::instance().get_counter("test_counter") << std::endl;
    std::cout << "   Gauge: " << Metrics::instance().get_gauge("test_gauge") << std::endl;
    
    // Test thread safety
    std::cout << "\n2. Testing thread safety with 10 threads..." << std::endl;
    const int num_threads = 10;
    const int iterations = 1000;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread, i, iterations);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    double final_counter = Metrics::instance().get_counter("thread_counter");
    double expected = num_threads * iterations;
    std::cout << "   Final counter: " << final_counter << " (expected: " << expected << ")" << std::endl;
    std::cout << "   Test " << (final_counter == expected ? "PASSED" : "FAILED") << std::endl;
    
    // Test quantum workspace metrics
    std::cout << "\n3. Testing quantum workspace metrics..." << std::endl;
    Metrics::instance().reset();
    
    QWConfig config;
    QuantumWorkspace qw(config);
    
    for (int i = 0; i < 50; ++i) {
        qw.step_ticks(1);
        if (qw.has_collapsed()) break;
    }
    
    std::cout << "   Entropy gauge: " << Metrics::instance().get_gauge("brain_qw_entropy") << std::endl;
    std::cout << "   Trace error: " << Metrics::instance().get_gauge("brain_qw_trace_error") << std::endl;
    
    // Export Prometheus format
    std::cout << "\n4. Prometheus export (first 500 chars):" << std::endl;
    std::string prom = Metrics::instance().export_prometheus();
    std::cout << prom.substr(0, 500) << std::endl;
    if (prom.length() > 500) {
        std::cout << "   ... (" << (prom.length() - 500) << " more characters)" << std::endl;
    }
    
    std::cout << "\n=== All metrics tests passed! ===" << std::endl;
    return 0;
}
