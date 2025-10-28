#pragma once

#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <sstream>
#include <chrono>

namespace hab {

// Simple metrics system without external dependencies
class SimpleMetrics {
public:
    static SimpleMetrics& instance() {
        static SimpleMetrics metrics;
        return metrics;
    }
    
    // Counter - monotonically increasing value
    void increment_counter(const std::string& name, double value = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        counters_[name] += value;
    }
    
    // Gauge - value that can go up and down
    void set_gauge(const std::string& name, double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        gauges_[name] = value;
    }
    
    // Histogram - track distribution of values
    void observe_histogram(const std::string& name, double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& hist = histograms_[name];
        hist.count++;
        hist.sum += value;
        if (hist.count == 1 || value < hist.min) hist.min = value;
        if (hist.count == 1 || value > hist.max) hist.max = value;
    }
    
    // Generate Prometheus-format metrics
    std::string prometheus_format() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        
        // Counters
        for (const auto& [name, value] : counters_) {
            oss << "# TYPE " << name << " counter\n";
            oss << name << " " << value << "\n";
        }
        
        // Gauges
        for (const auto& [name, value] : gauges_) {
            oss << "# TYPE " << name << " gauge\n";
            oss << name << " " << value << "\n";
        }
        
        // Histograms
        for (const auto& [name, hist] : histograms_) {
            oss << "# TYPE " << name << " summary\n";
            oss << name << "_count " << hist.count << "\n";
            oss << name << "_sum " << hist.sum << "\n";
            oss << name << "_min " << hist.min << "\n";
            oss << name << "_max " << hist.max << "\n";
            if (hist.count > 0) {
                oss << name << "_avg " << (hist.sum / hist.count) << "\n";
            }
        }
        
        return oss.str();
    }
    
    // Reset all metrics
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        counters_.clear();
        gauges_.clear();
        histograms_.clear();
    }

private:
    SimpleMetrics() = default;
    
    struct Histogram {
        uint64_t count = 0;
        double sum = 0.0;
        double min = 0.0;
        double max = 0.0;
    };
    
    mutable std::mutex mutex_;
    std::map<std::string, double> counters_;
    std::map<std::string, double> gauges_;
    std::map<std::string, Histogram> histograms_;
};

// Convenience macros
#define METRICS SimpleMetrics::instance()
#define METRIC_COUNTER(name, value) METRICS.increment_counter(name, value)
#define METRIC_GAUGE(name, value) METRICS.set_gauge(name, value)
#define METRIC_HISTOGRAM(name, value) METRICS.observe_histogram(name, value)

// RAII timer for automatic latency measurement
class MetricTimer {
public:
    explicit MetricTimer(const std::string& metric_name)
        : metric_name_(metric_name)
        , start_(std::chrono::steady_clock::now()) {}
    
    ~MetricTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start_).count();
        METRIC_HISTOGRAM(metric_name_, duration_ms);
    }

private:
    std::string metric_name_;
    std::chrono::steady_clock::time_point start_;
};

#define METRIC_TIMER(name) MetricTimer __metric_timer__(name)

} // namespace hab
