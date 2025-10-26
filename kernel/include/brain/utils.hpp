#pragma once

#include <string>
#include <chrono>
#include <random>
#include <vector>
#include <complex>

namespace hab {

// Type aliases for clarity
using Scalar = double;
using Complex = std::complex<Scalar>;
using TimePoint = std::chrono::steady_clock::time_point;

// Random number generator
inline std::mt19937& get_rng() {
    static thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}

// Simple timer utility
class Timer {
public:
    void start() { start_ = std::chrono::steady_clock::now(); }
    
    double elapsed_ms() const {
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

private:
    TimePoint start_;
};

// Logging levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Simple logger
class Logger {
public:
    static void log(LogLevel level, const std::string& msg);
    static void set_level(LogLevel level);
    
private:
    static LogLevel current_level_;
};

} // namespace hab
