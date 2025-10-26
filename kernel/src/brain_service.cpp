#include "brain/brain_system.hpp"
#include "brain/metrics.hpp"
#include <grpcpp/grpcpp.h>
#include "brain.grpc.pb.h"
#include <memory>
#include <mutex>
#include <iostream>
#include <chrono>

namespace hab {

class BrainServiceImpl final : public hab::Brain::Service {
public:
    explicit BrainServiceImpl(const BrainConfig& config) 
        : brain_(std::make_unique<BrainSystem>(config)) {}
    
    grpc::Status Health(grpc::ServerContext* context,
                       const hab::HealthReq* request,
                       hab::HealthResp* response) override {
        std::lock_guard<std::mutex> lock(brain_mutex_);
        
        response->set_status("healthy");
        response->set_version("3.1.0");
        
        auto uptime = std::chrono::steady_clock::now() - start_time_;
        auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(uptime).count();
        response->set_uptime_ms(uptime_ms);
        
        return grpc::Status::OK;
    }
    
    grpc::Status Step(grpc::ServerContext* context,
                     const hab::StepReq* request,
                     hab::StepResp* response) override {
        std::lock_guard<std::mutex> lock(brain_mutex_);
        
        // Convert input to Eigen vector
        Eigen::VectorXd input(request->input_size());
        for (int i = 0; i < request->input_size(); ++i) {
            input(i) = request->input(i);
        }
        
        // Get reward if provided
        Scalar reward = 0.0;
        auto ctx_it = request->context().find("reward");
        if (ctx_it != request->context().end()) {
            reward = std::stod(ctx_it->second);
        }
        
        // Run cognitive cycle
        auto result = brain_->step(input, reward);
        
        // Populate response - working memory
        for (int i = 0; i < result.h_wm.size(); ++i) {
            response->add_h_wm(result.h_wm(i));
        }
        
        // Global workspace state
        for (int i = 0; i < result.h_global.size(); ++i) {
            response->add_h_global(result.h_global(i));
        }
        
        // Conscious state
        response->set_collapsed_index(result.collapsed_quale);
        response->set_entropy(result.entropy);
        response->set_trace_error(0.0);  // TODO: Add to result
        
        // Phenomenal report
        auto* phenom = response->mutable_phenomenal();
        phenom->set_intensity(result.phenomenal.intensity);
        phenom->set_clarity(result.phenomenal.clarity);
        phenom->set_valence(result.phenomenal.valence);
        phenom->set_arousal(result.phenomenal.arousal);
        phenom->set_flow(0.5);  // TODO: Compute
        phenom->set_control(0.5);  // TODO: Compute
        phenom->set_presence(result.phenomenal.presence);
        phenom->set_self_salience(result.phenomenal.self_salience);
        
        // Metrics
        auto* metrics = response->mutable_metrics();
        metrics->set_gw_sparsity(0.2);  // TODO: Compute from GW
        metrics->set_wiring_energy(0.0);  // TODO: Get from wiring
        metrics->set_collapse_rate_hz(result.collapse_rate_hz);
        metrics->set_step_latency_us(0);  // TODO: Measure
        
        return grpc::Status::OK;
    }
    
    grpc::Status GetState(grpc::ServerContext* context,
                         const hab::StateReq* request,
                         hab::StateResp* response) override {
        std::lock_guard<std::mutex> lock(brain_mutex_);
        
        // Get quantum state
        const auto& rho = brain_->qw().rho();
        response->set_dimension(rho.rows());
        response->set_entropy(brain_->qw().entropy());
        
        // Flatten density matrix (real and imaginary parts)
        for (int i = 0; i < rho.rows(); ++i) {
            for (int j = 0; j < rho.cols(); ++j) {
                response->add_rho_real(rho(i, j).real());
                response->add_rho_imag(rho(i, j).imag());
            }
        }
        
        // Eigenvalues (for diagnostic)
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver(rho);
        for (int i = 0; i < solver.eigenvalues().size(); ++i) {
            response->add_eigenvalues(solver.eigenvalues()(i));
        }
        
        // Memory state
        auto* mem = response->mutable_memory();
        mem->set_stm_size(brain_->memory().stm().episodes().size());
        mem->set_ltm_size(brain_->memory().ltm().size());
        mem->set_cache_hit_rate(0.0);  // TODO: Track this
        
        return grpc::Status::OK;
    }
    
private:
    std::unique_ptr<BrainSystem> brain_;
    std::mutex brain_mutex_;
    TimePoint start_time_ = std::chrono::steady_clock::now();
};

} // namespace hab

// Server main function (accessible from main.cpp)
extern std::unique_ptr<grpc::Server> g_server;
extern std::mutex g_server_mutex;

void RunServer(const std::string& server_address, const hab::BrainConfig& config) {
    hab::BrainServiceImpl service(config);
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    
    // Store in global for signal handler
    {
        std::lock_guard<std::mutex> lock(g_server_mutex);
        g_server = std::move(server);
    }
    
    std::cout << "✓ Server listening on " << server_address << std::endl;
    std::cout << "✓ Service ready to accept connections" << std::endl;
    
    g_server->Wait();
    
    // Clear global reference
    {
        std::lock_guard<std::mutex> lock(g_server_mutex);
        g_server.reset();
    }
}
