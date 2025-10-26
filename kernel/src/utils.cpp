#include "brain/utils.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace hab {

LogLevel Logger::current_level_ = LogLevel::INFO;

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < current_level_) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm;
    localtime_r(&time_t, &tm);
    
    const char* level_str = "INFO";
    switch (level) {
        case LogLevel::DEBUG:   level_str = "DEBUG"; break;
        case LogLevel::INFO:    level_str = "INFO"; break;
        case LogLevel::WARNING: level_str = "WARN"; break;
        case LogLevel::ERROR:   level_str = "ERROR"; break;
    }
    
    std::cout << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
              << "." << std::setfill('0') << std::setw(3) << ms.count()
              << " [" << level_str << "] " << msg << std::endl;
}

void Logger::set_level(LogLevel level) {
    current_level_ = level;
}

} // namespace hab