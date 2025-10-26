#include "brain/decoder.hpp"
#include <cmath>
#include <algorithm>

namespace hab {

Decoder::Decoder(const DecoderConfig& config)
    : config_(config), rng_(std::random_device{}()) {
    initialize_weights();
}

void Decoder::initialize_weights() {
    std::normal_distribution<Scalar> dist(0.0, 0.1);
    
    W1_ = Eigen::MatrixXd::NullaryExpr(config_.hidden_dim, config_.input_dim,
        [&](){ return dist(rng_); });
    b1_ = Eigen::VectorXd::Zero(config_.hidden_dim);
    
    W2_ = Eigen::MatrixXd::NullaryExpr(config_.num_actions, config_.hidden_dim,
        [&](){ return dist(rng_); });
    b2_ = Eigen::VectorXd::Zero(config_.num_actions);
}

Eigen::VectorXd Decoder::forward(const Eigen::VectorXd& gw_state,
                                const Eigen::VectorXd& qw_onehot) {
    // Concatenate GW and QW states
    Eigen::VectorXd input(gw_state.size() + qw_onehot.size());
    input << gw_state, qw_onehot;
    
    // Hidden layer with ReLU
    Eigen::VectorXd h = (W1_ * input + b1_).array().max(0.0);
    
    // Output logits
    Eigen::VectorXd logits = W2_ * h + b2_;
    
    // Softmax
    return softmax(logits, config_.temperature);
}

Eigen::VectorXd Decoder::softmax(const Eigen::VectorXd& logits, Scalar temperature) const {
    Scalar max_logit = logits.maxCoeff();
    Eigen::VectorXd exp_logits = ((logits.array() - max_logit) / temperature).exp();
    Scalar sum = exp_logits.sum();
    return exp_logits / sum;
}

int Decoder::sample_action(const Eigen::VectorXd& probs) {
    std::discrete_distribution<int> dist(probs.data(), probs.data() + probs.size());
    return dist(rng_);
}

int Decoder::greedy_action(const Eigen::VectorXd& probs) const {
    Eigen::VectorXd::Index max_index;
    probs.maxCoeff(&max_index);
    return static_cast<int>(max_index);
}

} // namespace hab