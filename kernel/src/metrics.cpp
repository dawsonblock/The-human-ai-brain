#include "brain/metrics.hpp"
#include <sstream>
#include <iomanip>

namespace hab {

Metrics& Metrics::instance() {
    static Metrics instance;
    return instance;
}

void Metrics::increment_counter(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (counters_.find(name) == counters_.end()) {
        counters_[name].store(0.0);
    }
    counters_[name].store(counters_[name].load() + value);
}

double Metrics::get_counter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return it->second.load();
    }
    return 0.0;
}

void Metrics::set_gauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gauges_.find(name) == gauges_.end()) {
        gauges_[name].store(0.0);
    }
    gauges_[name].store(value);
}

double Metrics::get_gauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second.load();
    }
    return 0.0;
}

void Metrics::record_histogram(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (histograms_.find(name) == histograms_.end()) {
        histograms_[name].store(0.0);
        histogram_counts_[name].store(0);
    }
    histograms_[name].store(value);
    histogram_counts_[name].store(histogram_counts_[name].load() + 1);
}

std::string Metrics::export_prometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Counters
    for (const auto& [name, value] : counters_) {
        oss << "# TYPE " << name << " counter\n";
        oss << name << " " << std::fixed << std::setprecision(6) << value.load() << "\n";
    }
    
    // Gauges
    for (const auto& [name, value] : gauges_) {
        oss << "# TYPE " << name << " gauge\n";
        oss << name << " " << std::fixed << std::setprecision(6) << value.load() << "\n";
    }
    
    // Histograms (simplified)
    for (const auto& [name, value] : histograms_) {
        oss << "# TYPE " << name << " histogram\n";
        oss << name << "_sum " << std::fixed << std::setprecision(6) << value.load() << "\n";
        oss << name << "_count " << histogram_counts_.at(name).load() << "\n";
    }
    
    return oss.str();
}

void Metrics::record_entropy(double value) {
    set_gauge("brain_entropy", value);
}

void Metrics::record_trace_error(double value) {
    set_gauge("trace_error", value);
}

void Metrics::record_collapse() {
    increment_counter("collapse_rate_hz", 1.0);
}

void Metrics::record_gw_sparsity(double value) {
    set_gauge("gw_sparsity", value);
}

void Metrics::record_wiring_energy(double value) {
    set_gauge("wiring_energy", value);
}

void Metrics::record_step_latency(double value) {
    record_histogram("step_latency", value);
}

} // namespace hab