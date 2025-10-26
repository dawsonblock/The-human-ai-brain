#include "brain/gw.hpp"
#include "brain/metrics.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace hab {

GlobalWorkspace::GlobalWorkspace(const GWConfig& config)
    : config_(config), rng_(std::random_device{}()) {
    
    initialize_weights();
    current_state_ = Eigen::VectorXd::Zero(config_.output_dim);
}

void GlobalWorkspace::initialize_weights() {
    std::normal_distribution<Scalar> dist(0.0, 0.1);
    
    // Layer 1: input_dim -> hidden_dim
    W1_ = Eigen::MatrixXd::NullaryExpr(config_.hidden_dim, config_.input_dim,
        [&](){ return dist(rng_); });
    b1_ = Eigen::VectorXd::Zero(config_.hidden_dim);
    gamma1_ = Eigen::VectorXd::Ones(config_.hidden_dim);
    beta1_ = Eigen::VectorXd::Zero(config_.hidden_dim);
    
    // Layer 2: hidden_dim -> hidden_dim
    W2_ = Eigen::MatrixXd::NullaryExpr(config_.hidden_dim, config_.hidden_dim,
        [&](){ return dist(rng_); });
    b2_ = Eigen::VectorXd::Zero(config_.hidden_dim);
    gamma2_ = Eigen::VectorXd::Ones(config_.hidden_dim);
    beta2_ = Eigen::VectorXd::Zero(config_.hidden_dim);
    
    // Layer 3: hidden_dim -> output_dim
    W3_ = Eigen::MatrixXd::NullaryExpr(config_.output_dim, config_.hidden_dim,
        [&](){ return dist(rng_); });
    b3_ = Eigen::VectorXd::Zero(config_.output_dim);
}

Eigen::VectorXd GlobalWorkspace::forward(const Eigen::VectorXd& input) {
    if (input.size() != config_.input_dim) {
        throw std::runtime_error("Input dimension mismatch");
    }
    
    // Layer 1
    Eigen::VectorXd z1 = W1_ * input + b1_;
    z1 = gelu(z1);
    z1 = layer_norm(z1);
    
    // Layer 2
    Eigen::VectorXd z2 = W2_ * z1 + b2_;
    z2 = gelu(z2);
    z2 = layer_norm(z2);
    
    // Layer 3 (output)
    Eigen::VectorXd g = W3_ * z2 + b3_;
    
    // Apply sparsity gate
    Eigen::VectorXd g_sparse = apply_sparsity_gate(g);
    
    current_state_ = g_sparse;
    
    // Record sparsity metric (disabled for now)
    // int non_zero = (g_sparse.array().abs() > 1e-10).count();
    // Scalar sparsity = static_cast<Scalar>(non_zero) / g_sparse.size();
    // Metrics::instance().record_gw_sparsity(sparsity);
    
    return g_sparse;
}

Eigen::MatrixXd GlobalWorkspace::forward_batch(const Eigen::MatrixXd& inputs) {
    const int batch_size = inputs.rows();
    Eigen::MatrixXd outputs(batch_size, config_.output_dim);
    
    for (int i = 0; i < batch_size; ++i) {
        outputs.row(i) = forward(inputs.row(i)).transpose();
    }
    
    return outputs;
}

Eigen::VectorXd GlobalWorkspace::layer_norm(const Eigen::VectorXd& x, Scalar eps) const {
    Scalar mean = x.mean();
    Scalar variance = (x.array() - mean).square().mean();
    Scalar std = std::sqrt(variance + eps);
    
    return (x.array() - mean) / std;
}

Eigen::VectorXd GlobalWorkspace::gelu(const Eigen::VectorXd& x) const {
    // GELU(x) = x * Phi(x) where Phi is standard normal CDF
    // Approximation: GELU(x) ≈ 0.5 * x * (1 + tanh(sqrt(2/π) * (x + 0.044715 * x^3)))
    const Scalar sqrt_2_pi = std::sqrt(2.0 / M_PI);
    
    Eigen::VectorXd result(x.size());
    for (int i = 0; i < x.size(); ++i) {
        Scalar x_i = x(i);
        Scalar cube = x_i * x_i * x_i;
        result(i) = 0.5 * x_i * (1.0 + std::tanh(sqrt_2_pi * (x_i + 0.044715 * cube)));
    }
    
    return result;
}

Eigen::VectorXd GlobalWorkspace::apply_sparsity_gate(const Eigen::VectorXd& x) const {
    const int k = config_.sparse_k;
    const int n = x.size();
    
    if (k >= n) return x;
    
    // Get top-k indices
    std::vector<std::pair<Scalar, int>> vals;
    vals.reserve(n);
    
    for (int i = 0; i < n; ++i) {
        vals.push_back({std::abs(x(i)), i});
    }
    
    // Sort descending by absolute value
    std::partial_sort(vals.begin(), vals.begin() + k, vals.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Keep only top-k
    Eigen::VectorXd result = Eigen::VectorXd::Zero(n);
    for (int i = 0; i < k; ++i) {
        int idx = vals[i].second;
        result(idx) = x(idx);
    }
    
    return result;
}

Scalar GlobalWorkspace::compute_entropy(const Eigen::VectorXd& state) const {
    // Shannon entropy
    Scalar max_val = state.maxCoeff();
    
    // Softmax
    Eigen::VectorXd probs(state.size());
    Scalar sum = 0.0;
    
    for (int i = 0; i < state.size(); ++i) {
        probs(i) = std::exp(state(i) - max_val);
        sum += probs(i);
    }
    
    probs /= sum;
    
    // Entropy
    Scalar entropy = 0.0;
    for (int i = 0; i < probs.size(); ++i) {
        if (probs(i) > 1e-10) {
            entropy -= probs(i) * std::log2(probs(i));
        }
    }
    
    return entropy;
}

void GlobalWorkspace::adapt(const Eigen::VectorXd& target, Scalar lr) {
    // Simplified adaptation (gradient-free)
    Eigen::VectorXd error = target - current_state_;
    
    // Update output layer weights
    W3_ += lr * error * current_state_.transpose();
    b3_ += lr * error;
}

} // namespace hab
