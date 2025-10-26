#include "brain/wiring.hpp"
#include "brain/metrics.hpp"
#include <cmath>

namespace hab {

SelfWiring::SelfWiring(int dim, const WiringConfig& config)
    : config_(config) {
    W_ = Eigen::MatrixXd::Zero(dim, dim);
    mean_g_ = Eigen::VectorXd::Zero(dim);
    mean_r_ = Eigen::VectorXd::Zero(dim);
}

Scalar SelfWiring::compute_learning_rate(Scalar entropy) const {
    // η_H(t) = η_0 * σ[γ(S - S_0)]
    Scalar x = config_.gamma * (entropy - config_.S_0);
    Scalar sigmoid = 1.0 / (1.0 + std::exp(-x));
    return config_.eta_0 * sigmoid;
}

void SelfWiring::update(const Eigen::VectorXd& g_t,
                       const Eigen::VectorXd& r_t,
                       Scalar entropy) {
    // Update running means
    mean_g_ = ema_alpha_ * mean_g_ + (1.0 - ema_alpha_) * g_t;
    mean_r_ = ema_alpha_ * mean_r_ + (1.0 - ema_alpha_) * r_t;
    
    // Compute learning rate
    Scalar eta_h = compute_learning_rate(entropy);
    
    // Hebbian update: ΔW = η_H(g_t * r_t^T - mean_g * mean_r^T)
    Eigen::MatrixXd hebbian = g_t * r_t.transpose() - mean_g_ * mean_r_.transpose();
    
    // Weight decay: -λ * tanh(|W|) * sign(W)
    Eigen::MatrixXd decay = Eigen::MatrixXd::Zero(W_.rows(), W_.cols());
    for (int i = 0; i < W_.rows(); ++i) {
        for (int j = 0; j < W_.cols(); ++j) {
            Scalar w = W_(i, j);
            Scalar sign = (w > 0.0) ? 1.0 : ((w < 0.0) ? -1.0 : 0.0);
            decay(i, j) = config_.lambda_decay * std::tanh(std::abs(w)) * sign;
        }
    }
    
    // Update weights
    W_ += eta_h * hebbian - decay;
    
    // Record metrics (disabled for now)
    // Metrics::instance().record_wiring_energy(wiring_energy());
}

Eigen::VectorXd SelfWiring::apply(const Eigen::VectorXd& g_t, const Eigen::VectorXd& r_t) const {
    // g_{t+1} = LayerNorm(W_g * [g_t; r_t] + b_g)
    // Simplified: just add weighted memory recall
    return g_t + W_ * r_t;
}

Scalar SelfWiring::wiring_energy() const {
    // Energy proportional to Frobenius norm
    return W_.squaredNorm();
}

} // namespace hab