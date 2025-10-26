#pragma once

#include "utils.hpp"
#include <Eigen/Dense>
#include <vector>

namespace hab {

// Action decoder configuration
struct DecoderConfig {
    int input_dim = 67;      // 60 (GW) + 7 (QW onehot)
    int hidden_dim = 32;
    int num_actions = 10;
    Scalar temperature = 1.0;
};

// Action Decoder - Convert conscious state to actions
class Decoder {
public:
    explicit Decoder(const DecoderConfig& config = DecoderConfig{});
    
    // Get action distribution
    Eigen::VectorXd forward(const Eigen::VectorXd& gw_state,
                           const Eigen::VectorXd& qw_onehot);
    
    // Sample action from distribution
    int sample_action(const Eigen::VectorXd& probs);
    
    // Get most likely action (greedy)
    int greedy_action(const Eigen::VectorXd& probs) const;
    
private:
    void initialize_weights();
    Eigen::VectorXd softmax(const Eigen::VectorXd& logits, Scalar temperature) const;
    
    DecoderConfig config_;
    Eigen::MatrixXd W1_, W2_;
    Eigen::VectorXd b1_, b2_;
    std::mt19937 rng_;
};

} // namespace hab
