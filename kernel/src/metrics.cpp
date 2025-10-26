#include "brain/metrics.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace hab {

// Counter operations
void Metrics::increment_counter(const std::string& name, double value) {
    auto* counter = get_or_create_counter(name);
    if (counter) {
        // Atomic fetch_add for thread-safe increment
        double old_val = counter->value.load(std::memory_order_relaxed);
        while (!counter->value.compare_exchange_weak(old_val, old_val + value,
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed)) {
            // Retry if CAS fails
        }
    }
}

double Metrics::get_counter(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(counters_mutex_);
    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return it->second->value.load(std::memory_order_acquire);
    }
    return 0.0;
}

// Gauge operations
void Metrics::set_gauge(const std::string& name, double value) {
    auto* gauge = get_or_create_gauge(name);
    if (gauge) {
        gauge->value.store(value, std::memory_order_release);
    }
}

double Metrics::get_gauge(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(gauges_mutex_);
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second->value.load(std::memory_order_acquire);
    }
    return 0.0;
}

// Histogram operations
void Metrics::record_histogram(const std::string& name, double value) {
    auto* hist = get_or_create_histogram(name);
    if (hist) {
        // Update sum atomically
        double old_sum = hist->sum.load(std::memory_order_relaxed);
        while (!hist->sum.compare_exchange_weak(old_sum, old_sum + value,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed)) {}
        
        // Increment count
        hist->count.fetch_add(1, std::memory_order_release);
        
        // Update min/max using atomic compare-exchange
        double old_min = hist->min.load(std::memory_order_relaxed);
        while (value < old_min && 
               !hist->min.compare_exchange_weak(old_min, value,
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed)) {}
        
        double old_max = hist->max.load(std::memory_order_relaxed);
        while (value > old_max &&
               !hist->max.compare_exchange_weak(old_max, value,
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed)) {}
    }
}

// Prometheus export
std::string Metrics::export_prometheus() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    // Export counters
    {
        std::shared_lock<std::shared_mutex> lock(counters_mutex_);
        for (const auto& [name, data] : counters_) {
            oss << "# TYPE " << name << " counter\n";
            oss << name << " " << data->value.load(std::memory_order_acquire) << "\n";
        }
    }
    
    // Export gauges
    {
        std::shared_lock<std::shared_mutex> lock(gauges_mutex_);
        for (const auto& [name, data] : gauges_) {
            oss << "# TYPE " << name << " gauge\n";
            oss << name << " " << data->value.load(std::memory_order_acquire) << "\n";
        }
    }
    
    // Export histograms
    {
        std::shared_lock<std::shared_mutex> lock(histograms_mutex_);
        for (const auto& [name, data] : histograms_) {
            size_t count = data->count.load(std::memory_order_acquire);
            if (count > 0) {
                double sum = data->sum.load(std::memory_order_acquire);
                double min = data->min.load(std::memory_order_acquire);
                double max = data->max.load(std::memory_order_acquire);
                double avg = sum / count;
                
                oss << "# TYPE " << name << " summary\n";
                oss << name << "_sum " << sum << "\n";
                oss << name << "_count " << count << "\n";
                oss << name << "_avg " << avg << "\n";
                oss << name << "_min " << min << "\n";
                oss << name << "_max " << max << "\n";
            }
        }
    }
    
    return oss.str();
}

void Metrics::reset() {
    {
        std::unique_lock<std::shared_mutex> lock(counters_mutex_);
        counters_.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(gauges_mutex_);
        gauges_.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(histograms_mutex_);
        histograms_.clear();
    }
}

// Predefined metrics
void Metrics::record_entropy(double value) {
    set_gauge("brain_qw_entropy", value);
    record_histogram("brain_qw_entropy_hist", value);
}

void Metrics::record_trace_error(double value) {
    set_gauge("brain_qw_trace_error", value);
    record_histogram("brain_qw_trace_error_hist", value);
}

void Metrics::record_collapse() {
    increment_counter("brain_qw_collapses_total", 1.0);
}

void Metrics::record_gw_sparsity(double value) {
    set_gauge("brain_gw_sparsity", value);
}

void Metrics::record_wiring_energy(double value) {
    set_gauge("brain_wiring_energy", value);
}

void Metrics::record_step_latency(double value) {
    record_histogram("brain_step_latency_seconds", value);
}

// Helper methods to get or create metrics
Metrics::CounterData* Metrics::get_or_create_counter(const std::string& name) {
    // Try read-only access first (fast path)
    {
        std::shared_lock<std::shared_mutex> lock(counters_mutex_);
        auto it = counters_.find(name);
        if (it != counters_.end()) {
            return it->second.get();
        }
    }
    
    // Acquire write lock to create (slow path)
    std::unique_lock<std::shared_mutex> lock(counters_mutex_);
    // Double-check after acquiring write lock (another thread may have created it)
    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return it->second.get();
    }
    
    auto counter = std::make_unique<CounterData>();
    auto* ptr = counter.get();
    counters_[name] = std::move(counter);
    return ptr;
}

Metrics::GaugeData* Metrics::get_or_create_gauge(const std::string& name) {
    {
        std::shared_lock<std::shared_mutex> lock(gauges_mutex_);
        auto it = gauges_.find(name);
        if (it != gauges_.end()) {
            return it->second.get();
        }
    }
    
    std::unique_lock<std::shared_mutex> lock(gauges_mutex_);
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second.get();
    }
    
    auto gauge = std::make_unique<GaugeData>();
    auto* ptr = gauge.get();
    gauges_[name] = std::move(gauge);
    return ptr;
}

Metrics::HistogramData* Metrics::get_or_create_histogram(const std::string& name) {
    {
        std::shared_lock<std::shared_mutex> lock(histograms_mutex_);
        auto it = histograms_.find(name);
        if (it != histograms_.end()) {
            return it->second.get();
        }
    }
    
    std::unique_lock<std::shared_mutex> lock(histograms_mutex_);
    auto it = histograms_.find(name);
    if (it != histograms_.end()) {
        return it->second.get();
    }
    
    auto hist = std::make_unique<HistogramData>();
    auto* ptr = hist.get();
    histograms_[name] = std::move(hist);
    return ptr;
}

} // namespace hab
