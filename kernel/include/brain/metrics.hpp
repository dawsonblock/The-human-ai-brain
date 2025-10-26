#pragma once

#include <atomic>
#include <string>
#include <map>
#include <memory>
#include <mutex>

namespace hab {

// Metrics collector for Prometheus
class Metrics {
public:
    static Metrics& instance();
    
    // Counter metrics
    void increment_counter(const std::string& name, double value = 1.0);
    double get_counter(const std::string& name) const;
    
    // Gauge metrics
    void set_gauge(const std::string& name, double value);
    double get_gauge(const std::string& name) const;
    
    // Histogram (simplified - just tracks last value and count)
    void record_histogram(const std::string& name, double value);
    
    // Get all metrics as Prometheus text format
    std::string export_prometheus() const;
    
    // Predefined metrics
    void record_entropy(double value);
    void record_trace_error(double value);
    void record_collapse();
    void record_gw_sparsity(double value);
    void record_wiring_energy(double value);
    void record_step_latency(double value);
    
private:
    Metrics() = default;
    
    mutable std::mutex mutex_;
    std::map<std::string, std::atomic<double>> counters_;
    std::map<std::string, std::atomic<double>> gauges_;
    std::map<std::string, std::atomic<double>> histograms_;
    std::map<std::string, std::atomic<size_t>> histogram_counts_;
};

} // namespace hab
