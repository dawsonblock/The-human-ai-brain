#pragma once

#include "utils.hpp"
#include <Eigen/Dense>
#include <memory>
#include <vector>

namespace hab {

// Global Workspace Configuration
struct GWConfig {
    int input_dim = 256;     // Input feature dimension
    int hidden_dim = 128;    // Hidden layer dimension
    int output_dim = 60;     // Global workspace dimension
    int sparse_k = 12;       // Top-k sparse activation
    Scalar learning_rate = 1e-3;
};

// Global Workspace - Pre-conscious integration hub
class GlobalWorkspace {
public:
    explicit GlobalWorkspace(const GWConfig& config = GWConfig{});
    
    // Rule of five
    ~GlobalWorkspace() = default;
    GlobalWorkspace(const GlobalWorkspace&) = delete;
    GlobalWorkspace& operator=(const GlobalWorkspace&) = delete;
    GlobalWorkspace(GlobalWorkspace&&) noexcept = default;
    GlobalWorkspace& operator=(GlobalWorkspace&&) noexcept = default;
    
    // Forward pass with sparsity gate
    Eigen::VectorXd forward(const Eigen::VectorXd& input);
    
    // Batch processing
    Eigen::MatrixXd forward_batch(const Eigen::MatrixXd& inputs);
    
    // Compute Shannon entropy
    Scalar compute_entropy(const Eigen::VectorXd& state) const;
    
    // Get current state
    const Eigen::VectorXd& current_state() const { return current_state_; }
    
    // Learning/adaptation (simplified - no full backprop in this version)
    void adapt(const Eigen::VectorXd& target, Scalar lr);
    
private:
    Eigen::VectorXd layer_norm(const Eigen::VectorXd& x, Scalar eps = 1e-5) const;
    Eigen::VectorXd gelu(const Eigen::VectorXd& x) const;
    Eigen::VectorXd apply_sparsity_gate(const Eigen::VectorXd& x) const;
    void initialize_weights();
    
    GWConfig config_;
    
    // Network parameters
    Eigen::MatrixXd W1_, W2_, W3_;
    Eigen::VectorXd b1_, b2_, b3_;
    
    // Layer norm parameters
    Eigen::VectorXd gamma1_, beta1_;
    Eigen::VectorXd gamma2_, beta2_;
    
    // Current state
    Eigen::VectorXd current_state_;
    
    std::mt19937 rng_;
};

} // namespace hab
