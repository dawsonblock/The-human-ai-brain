// kernel/src/metrics.cpp
// Implementation file for MetricsCollector
// Most logic is in the header using inline implementations

#include "brain/metrics.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace hab {

// Implementation note: Core metric recording methods are inline in header
// This file provides format conversion methods that are too large for inline

std::string MetricsCollector::to_json() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    auto now = std::chrono::steady_clock::now();
    auto uptime_s = std::chrono::duration_cast<std::chrono::seconds>(
        now - start_time_).count();
    
    oss << "{\n";
    oss << "  \"uptime_seconds\": " << uptime_s << ",\n";
    oss << "  \"collapses_total\": " << collapses_total_.load(std::memory_order_relaxed) << ",\n";
    oss << "  \"collapse_rate_hz\": " << get_collapse_rate_hz() << ",\n";
    oss << "  \"entropy_current\": " << entropy_current_.load(std::memory_order_relaxed) << ",\n";
    oss << "  \"dwell_overruns_total\": " << dwell_overruns_total_.load(std::memory_order_relaxed) << ",\n";
    oss << "  \"steps_total\": " << steps_total_.load(std::memory_order_relaxed) << ",\n";
    oss << "  \"last_step_latency_us\": " << last_step_latency_us_.load(std::memory_order_relaxed) << "\n";
    oss << "}";
    
    return oss.str();
}

std::string MetricsCollector::to_prometheus() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    auto now = std::chrono::steady_clock::now();
    auto uptime_s = std::chrono::duration_cast<std::chrono::seconds>(
        now - start_time_).count();
    
    // Counter metrics
    oss << "# HELP hab_collapses_total Total number of quantum collapses\n";
    oss << "# TYPE hab_collapses_total counter\n";
    oss << "hab_collapses_total " << collapses_total_.load(std::memory_order_relaxed) << "\n";
    
    oss << "# HELP hab_dwell_overruns_total Total number of dwell time overruns\n";
    oss << "# TYPE hab_dwell_overruns_total counter\n";
    oss << "hab_dwell_overruns_total " << dwell_overruns_total_.load(std::memory_order_relaxed) << "\n";
    
    oss << "# HELP hab_steps_total Total number of processing steps\n";
    oss << "# TYPE hab_steps_total counter\n";
    oss << "hab_steps_total " << steps_total_.load(std::memory_order_relaxed) << "\n";
    
    // Gauge metrics
    oss << "# HELP hab_entropy_current Current system entropy\n";
    oss << "# TYPE hab_entropy_current gauge\n";
    oss << "hab_entropy_current " << entropy_current_.load(std::memory_order_relaxed) << "\n";
    
    oss << "# HELP hab_collapse_rate_hz Collapse rate in Hz\n";
    oss << "# TYPE hab_collapse_rate_hz gauge\n";
    oss << "hab_collapse_rate_hz " << get_collapse_rate_hz() << "\n";
    
    oss << "# HELP hab_last_step_latency_us Last step latency in microseconds\n";
    oss << "# TYPE hab_last_step_latency_us gauge\n";
    oss << "hab_last_step_latency_us " << last_step_latency_us_.load(std::memory_order_relaxed) << "\n";
    
    oss << "# HELP hab_uptime_seconds System uptime in seconds\n";
    oss << "# TYPE hab_uptime_seconds counter\n";
    oss << "hab_uptime_seconds " << uptime_s << "\n";
    
    return oss.str();
}

double MetricsCollector::get_collapse_rate_hz() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed_s = std::chrono::duration_cast<std::chrono::duration<double>>(
        now - start_time_).count();
    
    if (elapsed_s < 1e-6) {
        return 0.0;
    }
    
    int64_t total = collapses_total_.load(std::memory_order_relaxed);
    return static_cast<double>(total) / elapsed_s;
}

} // namespace hab
