#pragma once

#include "simple_metrics.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <memory>

namespace hab {

/**
 * @brief Simple HTTP server for Prometheus /metrics endpoint
 * 
 * Runs on port 9090 and serves metrics in Prometheus format.
 * Non-blocking: runs in background thread.
 */
class MetricsServer {
public:
    explicit MetricsServer(int port = 9090);
    ~MetricsServer();
    
    // Non-copyable, non-movable
    MetricsServer(const MetricsServer&) = delete;
    MetricsServer& operator=(const MetricsServer&) = delete;
    MetricsServer(MetricsServer&&) = delete;
    MetricsServer& operator=(MetricsServer&&) = delete;
    
    /**
     * @brief Start the HTTP server in a background thread
     */
    void start();
    
    /**
     * @brief Stop the HTTP server gracefully
     */
    void stop();
    
    /**
     * @brief Check if server is running
     */
    bool is_running() const { return running_.load(); }
    
    /**
     * @brief Get the server port
     */
    int port() const { return port_; }
    
private:
    void run_server();
    std::string handle_request(const std::string& request);
    
    int port_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> server_thread_;
};

} // namespace hab
