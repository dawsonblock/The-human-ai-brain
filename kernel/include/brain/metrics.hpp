#pragma once

#include <atomic>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace hab {

// Thread-safe metrics collector for Prometheus
// Uses proper double-checked locking and reader-writer locks for performance
class Metrics {
public:
    // Meyer's singleton with thread-safe initialization (C++11+)
    static Metrics& instance() {
        static Metrics instance;
        return instance;
    }
    
    // Delete copy/move to enforce singleton
    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;
    Metrics(Metrics&&) = delete;
    Metrics& operator=(Metrics&&) = delete;
    
    // Counter metrics (monotonically increasing)
    void increment_counter(const std::string& name, double value = 1.0);
    double get_counter(const std::string& name) const;
    
    // Gauge metrics (can go up or down)
    void set_gauge(const std::string& name, double value);
    double get_gauge(const std::string& name) const;
    
    // Histogram (simplified - tracks sum, count, min, max)
    void record_histogram(const std::string& name, double value);
    
    // Get all metrics as Prometheus text format
    std::string export_prometheus() const;
    
    // Reset all metrics (useful for testing)
    void reset();
    
    // Predefined metrics for brain components
    void record_entropy(double value);
    void record_trace_error(double value);
    void record_collapse();
    void record_gw_sparsity(double value);
    void record_wiring_energy(double value);
    void record_step_latency(double value);
    
private:
    Metrics() = default;
    ~Metrics() = default;
    
    // Metric storage structures
    struct CounterData {
        std::atomic<double> value{0.0};
    };
    
    struct GaugeData {
        std::atomic<double> value{0.0};
    };
    
    struct HistogramData {
        std::atomic<double> sum{0.0};
        std::atomic<size_t> count{0};
        std::atomic<double> min{std::numeric_limits<double>::max()};
        std::atomic<double> max{std::numeric_limits<double>::lowest()};
    };
    
    // Use shared_mutex for reader-writer lock (many readers, few writers)
    mutable std::shared_mutex counters_mutex_;
    mutable std::shared_mutex gauges_mutex_;
    mutable std::shared_mutex histograms_mutex_;
    
    std::unordered_map<std::string, std::unique_ptr<CounterData>> counters_;
    std::unordered_map<std::string, std::unique_ptr<GaugeData>> gauges_;
    std::unordered_map<std::string, std::unique_ptr<HistogramData>> histograms_;
    
    // Helper to get or create metric
    CounterData* get_or_create_counter(const std::string& name);
    GaugeData* get_or_create_gauge(const std::string& name);
    HistogramData* get_or_create_histogram(const std::string& name);
};

} // namespace hab
