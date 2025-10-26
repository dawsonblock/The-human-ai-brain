#pragma once

#include "utils.hpp"
#include <Eigen/Dense>

namespace hab {

// Self-wiring configuration
struct WiringConfig {
    Scalar eta_0 = 1e-3;           // Base Hebbian learning rate
    Scalar lambda_decay = 1e-4;    // Weight decay coefficient
    Scalar gamma = 10.0;           // Entropy gain for gating
    Scalar S_0 = 0.973;            // Entropy threshold (0.5 * log(7))
};

// Self-Wiring Engine - Hebbian plasticity with entropy gating
class SelfWiring {
public:
    explicit SelfWiring(int dim, const WiringConfig& config = WiringConfig{});
    
    // Entropy-gated Hebbian update
    void update(const Eigen::VectorXd& g_t,
                const Eigen::VectorXd& r_t,
                Scalar entropy);
    
    // Apply wiring to integrate memory recall into GW state
    Eigen::VectorXd apply(const Eigen::VectorXd& g_t, const Eigen::VectorXd& r_t) const;
    
    // Get current wiring matrix
    const Eigen::MatrixXd& W() const { return W_; }
    
    // Energy metric
    Scalar wiring_energy() const;
    
private:
    Scalar compute_learning_rate(Scalar entropy) const;
    
    WiringConfig config_;
    Eigen::MatrixXd W_;              // Wiring matrix
    Eigen::VectorXd mean_g_;         // Running mean of g
    Eigen::VectorXd mean_r_;         // Running mean of r
    Scalar ema_alpha_ = 0.99;        // EMA decay
};

} // namespace hab
