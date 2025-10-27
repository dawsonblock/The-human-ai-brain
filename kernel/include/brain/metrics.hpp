#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

namespace hab {

/**
 * MetricsCollector - Thread-safe metrics collection for observability
 * 
 * Tracks:
 * - collapses_total: Total quantum collapses
 * - dwell_overruns_total: Times dwell exceeded max_dwell_ms
 * - entropy_current: Current von Neumann entropy
 * - steps_total: Total cognitive steps executed
 * - collapse_rate_hz: Collapses per second
 * 
 * Export formats:
 * - JSON for logging
 * - Prometheus for metrics endpoint
 */
class MetricsCollector {
public:
    MetricsCollector() 
        : start_time_(std::chrono::steady_clock::now()) {}
    
    // Recording methods
    void record_step(int64_t latency_us) {
        steps_total_.fetch_add(1, std::memory_order_relaxed);
        last_step_latency_us_.store(latency_us, std::memory_order_relaxed);
    }
    
    void record_collapse() {
        collapses_total_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void record_dwell_overrun() {
        dwell_overruns_total_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void set_entropy(double entropy) {
        entropy_current_.store(entropy, std::memory_order_relaxed);
    }
    
    // Query methods
    double get_collapse_rate_hz() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        
        if (elapsed.count() == 0) return 0.0;
        
        return static_cast<double>(collapses_total_.load()) / elapsed.count();
    }
    
    double get_entropy_current() const {
        return entropy_current_.load(std::memory_order_relaxed);
    }
    
    int64_t get_collapses_total() const {
        return collapses_total_.load(std::memory_order_relaxed);
    }
    
    int64_t get_dwell_overruns_total() const {
        return dwell_overruns_total_.load(std::memory_order_relaxed);
    }
    
    int64_t get_steps_total() const {
        return steps_total_.load(std::memory_order_relaxed);
    }
    
    int64_t get_last_step_latency_us() const {
        return last_step_latency_us_.load(std::memory_order_relaxed);
    }
    
    // Export methods
    std::string to_json() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << "{\n";
        oss << "  \"collapses_total\": " << get_collapses_total() << ",\n";
        oss << "  \"dwell_overruns_total\": " << get_dwell_overruns_total() << ",\n";
        oss << "  \"entropy_current\": " << get_entropy_current() << ",\n";
        oss << "  \"collapse_rate_hz\": " << get_collapse_rate_hz() << ",\n";
        oss << "  \"steps_total\": " << get_steps_total() << ",\n";
        oss << "  \"last_step_latency_us\": " << get_last_step_latency_us() << "\n";
        oss << "}";
        return oss.str();
    }
    
    std::string to_prometheus() const {
        std::ostringstream oss;
        
        oss << "# HELP collapses_total Number of quantum collapses\n";
        oss << "# TYPE collapses_total counter\n";
        oss << "collapses_total " << get_collapses_total() << "\n\n";
        
        oss << "# HELP entropy_current Current von Neumann entropy\n";
        oss << "# TYPE entropy_current gauge\n";
        oss << "entropy_current " << get_entropy_current() << "\n\n";
        
        oss << "# HELP dwell_overruns_total Dwell time overruns\n";
        oss << "# TYPE dwell_overruns_total counter\n";
        oss << "dwell_overruns_total " << get_dwell_overruns_total() << "\n\n";
        
        oss << "# HELP collapse_rate_hz Collapse rate in Hz\n";
        oss << "# TYPE collapse_rate_hz gauge\n";
        oss << "collapse_rate_hz " << get_collapse_rate_hz() << "\n\n";
        
        oss << "# HELP steps_total Total cognitive steps executed\n";
        oss << "# TYPE steps_total counter\n";
        oss << "steps_total " << get_steps_total() << "\n\n";
        
        oss << "# HELP step_latency_us Last step latency in microseconds\n";
        oss << "# TYPE step_latency_us gauge\n";
        oss << "step_latency_us " << get_last_step_latency_us() << "\n";
        
        return oss.str();
    }

private:
    std::atomic<int64_t> collapses_total_{0};
    std::atomic<int64_t> dwell_overruns_total_{0};
    std::atomic<int64_t> steps_total_{0};
    std::atomic<int64_t> last_step_latency_us_{0};
    std::atomic<double> entropy_current_{0.0};
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace hab
