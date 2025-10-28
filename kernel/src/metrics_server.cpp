#include "brain/metrics_server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>

namespace hab {

MetricsServer::MetricsServer(int port)
    : port_(port)
    , running_(false)
    , server_thread_(nullptr) {
}

MetricsServer::~MetricsServer() {
    stop();
}

void MetricsServer::start() {
    if (running_.load()) {
        return; // Already running
    }
    
    running_.store(true);
    server_thread_ = std::make_unique<std::thread>(&MetricsServer::run_server, this);
}

void MetricsServer::stop() {
    if (!running_.load()) {
        return; // Not running
    }
    
    running_.store(false);
    
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
}

void MetricsServer::run_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "MetricsServer: Failed to create socket\n";
        return;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "MetricsServer: setsockopt failed\n";
        close(server_fd);
        return;
    }
    
    // Set non-blocking mode for accept() timeout
    struct timeval timeout;
    timeout.tv_sec = 1;  // 1 second timeout
    timeout.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "MetricsServer: Bind failed on port " << port_ << "\n";
        close(server_fd);
        return;
    }
    
    if (listen(server_fd, 3) < 0) {
        std::cerr << "MetricsServer: Listen failed\n";
        close(server_fd);
        return;
    }
    
    std::cout << "MetricsServer: Listening on port " << port_ << "\n";
    
    while (running_.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Timeout, check if we should stop
                continue;
            }
            std::cerr << "MetricsServer: Accept failed\n";
            continue;
        }
        
        // Read request
        char buffer[4096] = {0};
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            std::string request(buffer, bytes_read);
            std::string response = handle_request(request);
            write(client_fd, response.c_str(), response.size());
        }
        
        close(client_fd);
    }
    
    close(server_fd);
    std::cout << "MetricsServer: Stopped\n";
}

std::string MetricsServer::handle_request(const std::string& request) {
    // Parse HTTP request line
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    std::ostringstream response;
    
    if (method == "GET" && path == "/metrics") {
        // Get metrics in Prometheus format
        std::string metrics = SimpleMetrics::instance().prometheus_format();
        
        // HTTP response
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain; version=0.0.4\r\n";
        response << "Content-Length: " << metrics.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << metrics;
    } else if (method == "GET" && path == "/health") {
        // Health check endpoint
        std::string body = "OK\n";
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
    } else {
        // 404 Not Found
        std::string body = "Not Found\n";
        response << "HTTP/1.1 404 Not Found\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;
    }
    
    return response.str();
}

} // namespace hab
