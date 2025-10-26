#include "brain/trainer.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace hab {

BrainTrainer::BrainTrainer(BrainSystem& brain, const TrainerConfig& config)
    : brain_(brain), config_(config) {
    initialize_optimizer();
}

void BrainTrainer::train(Dataset& dataset) {
    if (config_.verbose) {
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║          Brain Training - " << dataset.name() << std::setw(28) << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        std::cout << "Configuration:\n";
        std::cout << "  ├─ Epochs: " << config_.num_epochs << "\n";
        std::cout << "  ├─ Batch Size: " << config_.batch_size << "\n";
        std::cout << "  ├─ Learning Rate: " << config_.learning_rate << "\n";
        std::cout << "  ├─ Optimizer: " << (config_.optimizer == TrainerConfig::Optimizer::ADAM ? "Adam" : 
                                           config_.optimizer == TrainerConfig::Optimizer::SGD ? "SGD" : "RMSProp") << "\n";
        std::cout << "  └─ Dataset Size: " << dataset.size() << " samples\n\n";
    }
    
    for (size_t epoch = 0; epoch < config_.num_epochs; ++epoch) {
        train_epoch(dataset, epoch);
        
        // Learning rate schedule
        apply_learning_rate_schedule(epoch);
        
        // Checkpointing
        if (config_.enable_checkpoints && (epoch + 1) % config_.checkpoint_interval == 0) {
            std::string path = config_.checkpoint_dir + "/checkpoint_epoch_" + std::to_string(epoch + 1) + ".bin";
            save_checkpoint(path, epoch + 1);
        }
        
        // Early stopping
        if (config_.enable_early_stopping && should_stop_early()) {
            if (config_.verbose) {
                std::cout << "\n⚠ Early stopping triggered after " << epoch + 1 << " epochs\n";
            }
            break;
        }
        
        // Epoch callback
        if (epoch_callback_) {
            epoch_callback_(epoch, metrics_);
        }
    }
    
    if (config_.verbose) {
        std::cout << "\n✓ Training complete!\n";
        std::cout << "  Final Loss: " << metrics_.loss << "\n";
        std::cout << "  Final Accuracy: " << (metrics_.accuracy * 100.0) << "%\n";
    }
}

void BrainTrainer::train_epoch(Dataset& dataset, size_t epoch) {
    if (config_.shuffle) {
        dataset.shuffle();
    }
    
    size_t num_batches = (dataset.size() + config_.batch_size - 1) / config_.batch_size;
    Scalar epoch_loss = 0.0;
    Scalar epoch_accuracy = 0.0;
    size_t samples_in_epoch = 0;
    
    for (size_t batch_id = 0; batch_id < num_batches; ++batch_id) {
        TrainingBatch batch;
        batch.batch_id = batch_id;
        batch.epoch = epoch;
        
        // Collect samples for batch
        size_t start_idx = batch_id * config_.batch_size;
        size_t end_idx = std::min(start_idx + config_.batch_size, dataset.size());
        
        for (size_t i = start_idx; i < end_idx; ++i) {
            batch.samples.push_back(dataset.get(i));
        }
        
        // Train on batch
        train_batch(batch);
        
        // Accumulate metrics
        epoch_loss += metrics_.loss * batch.samples.size();
        epoch_accuracy += metrics_.accuracy * batch.samples.size();
        samples_in_epoch += batch.samples.size();
        
        // Logging
        if (config_.verbose && (batch_id + 1) % config_.log_interval == 0) {
            std::cout << "Epoch " << (epoch + 1) << "/" << config_.num_epochs 
                      << " - Batch " << (batch_id + 1) << "/" << num_batches
                      << " - Loss: " << std::fixed << std::setprecision(4) << metrics_.loss
                      << " - Acc: " << std::fixed << std::setprecision(2) << (metrics_.accuracy * 100.0) << "%"
                      << " - LR: " << std::scientific << std::setprecision(2) << get_current_learning_rate()
                      << "\r" << std::flush;
        }
        
        // Batch callback
        if (batch_callback_) {
            batch_callback_(batch_id, batch, metrics_);
        }
    }
    
    // Store epoch metrics
    epoch_loss /= samples_in_epoch;
    epoch_accuracy /= samples_in_epoch;
    metrics_.epoch_losses.push_back(epoch_loss);
    metrics_.epoch_accuracies.push_back(epoch_accuracy);
    
    if (config_.verbose) {
        std::cout << "Epoch " << (epoch + 1) << "/" << config_.num_epochs 
                  << " - Loss: " << std::fixed << std::setprecision(4) << epoch_loss
                  << " - Acc: " << std::fixed << std::setprecision(2) << (epoch_accuracy * 100.0) << "%"
                  << " - Collapse Rate: " << std::fixed << std::setprecision(2) << metrics_.collapse_rate << " Hz"
                  << std::endl;
    }
    
    // Early stopping check
    if (epoch_loss < best_loss_ - config_.min_delta) {
        best_loss_ = epoch_loss;
        epochs_without_improvement_ = 0;
    } else {
        epochs_without_improvement_++;
    }
}

void BrainTrainer::train_batch(const TrainingBatch& batch) {
    Scalar batch_loss = 0.0;
    Scalar batch_accuracy = 0.0;
    Scalar batch_entropy = 0.0;
    size_t collapse_count = 0;
    
    for (const auto& sample : batch.samples) {
        // Forward pass
        auto result = brain_.step(sample.input, sample.reward);
        
        // Compute loss
        Scalar sample_loss = compute_loss(sample, result);
        batch_loss += sample_loss;
        
        // Compute accuracy
        batch_accuracy += compute_accuracy(sample, result);
        
        // Track metrics
        batch_entropy += result.entropy;
        if (result.collapsed_quale >= 0) {
            collapse_count++;
        }
        
        // Compute gradients (simplified - would need actual backprop implementation)
        auto gradients = compute_gradients(sample, result);
        
        // Update parameters
        update_parameters(gradients);
        
        metrics_.samples_processed++;
    }
    
    // Average metrics
    metrics_.loss = batch_loss / batch.samples.size();
    metrics_.accuracy = batch_accuracy / batch.samples.size();
    metrics_.avg_entropy = batch_entropy / batch.samples.size();
    metrics_.collapse_rate = (collapse_count / static_cast<Scalar>(batch.samples.size())) * 8.2;  // Approximate Hz
}

Scalar BrainTrainer::compute_loss(const TrainingSample& sample, const CognitiveResult& result) {
    switch (config_.mode) {
        case TrainerConfig::Mode::SUPERVISED:
            if (!sample.target.size()) {
                return 0.0;  // No target provided
            }
            return loss::mse_loss(result.h_global, sample.target);
            
        case TrainerConfig::Mode::REINFORCEMENT:
            // Policy gradient loss (negative expected return)
            return -sample.reward * result.entropy;  // Entropy bonus for exploration
            
        case TrainerConfig::Mode::SELF_SUPERVISED:
            // Reconstruction loss (predict next state)
            return loss::mse_loss(result.h_wm, sample.input);
            
        default:
            return 0.0;
    }
}

Scalar BrainTrainer::compute_accuracy(const TrainingSample& sample, const CognitiveResult& result) {
    if (sample.label.empty() || result.h_global.size() == 0) {
        return 0.0;
    }
    
    // Find predicted class (argmax of global state - first 10 dims)
    int predicted_class = 0;
    Scalar max_val = result.h_global(0);
    int search_dims = std::min(10, static_cast<int>(result.h_global.size()));
    for (int i = 1; i < search_dims; ++i) {
        if (result.h_global(i) > max_val) {
            max_val = result.h_global(i);
            predicted_class = i;
        }
    }
    
    // Compare with true label
    int true_class = std::stoi(sample.label);
    return (predicted_class == true_class) ? 1.0 : 0.0;
}

void BrainTrainer::update_parameters(const std::vector<Eigen::VectorXd>& gradients) {
    step_count_++;
    Scalar lr = get_current_learning_rate();
    
    switch (config_.optimizer) {
        case TrainerConfig::Optimizer::SGD: {
            // Simple SGD with momentum
            for (size_t i = 0; i < gradients.size(); ++i) {
                if (i >= momentum_buffers_.size()) {
                    momentum_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                }
                momentum_buffers_[i] = config_.momentum * momentum_buffers_[i] - lr * gradients[i];
                // Apply update (would need actual parameter access)
            }
            break;
        }
        
        case TrainerConfig::Optimizer::ADAM: {
            // Adam optimizer
            for (size_t i = 0; i < gradients.size(); ++i) {
                if (i >= momentum_buffers_.size()) {
                    momentum_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                    velocity_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                }
                
                // Update biased first moment estimate
                momentum_buffers_[i] = config_.beta1 * momentum_buffers_[i] + 
                                      (1.0 - config_.beta1) * gradients[i];
                
                // Update biased second moment estimate
                velocity_buffers_[i] = config_.beta2 * velocity_buffers_[i] + 
                                      (1.0 - config_.beta2) * gradients[i].array().square().matrix();
                
                // Bias correction
                Eigen::VectorXd m_hat = momentum_buffers_[i] / (1.0 - std::pow(config_.beta1, step_count_));
                Eigen::VectorXd v_hat = velocity_buffers_[i] / (1.0 - std::pow(config_.beta2, step_count_));
                
                // Update parameters
                Eigen::VectorXd update = -lr * m_hat.array() / (v_hat.array().sqrt() + config_.epsilon);
                // Apply update (would need actual parameter access)
            }
            break;
        }
        
        case TrainerConfig::Optimizer::RMSPROP: {
            // RMSProp optimizer
            for (size_t i = 0; i < gradients.size(); ++i) {
                if (i >= velocity_buffers_.size()) {
                    velocity_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                }
                
                velocity_buffers_[i] = config_.momentum * velocity_buffers_[i] + 
                                      (1.0 - config_.momentum) * gradients[i].array().square().matrix();
                
                Eigen::VectorXd update = -lr * gradients[i].array() / 
                                        (velocity_buffers_[i].array().sqrt() + config_.epsilon);
                // Apply update (would need actual parameter access)
            }
            break;
        }
    }
}

void BrainTrainer::apply_learning_rate_schedule(size_t epoch) {
    // Learning rate schedule logic (would modify config_.learning_rate)
    switch (config_.lr_schedule) {
        case TrainerConfig::LRSchedule::STEP:
            if ((epoch + 1) % config_.lr_step_size == 0) {
                config_.learning_rate *= config_.lr_decay;
            }
            break;
        case TrainerConfig::LRSchedule::EXPONENTIAL:
            config_.learning_rate *= config_.lr_decay;
            break;
        case TrainerConfig::LRSchedule::COSINE:
            config_.learning_rate = config_.learning_rate * 0.5 * 
                (1.0 + std::cos(M_PI * epoch / config_.num_epochs));
            break;
        default:
            break;
    }
}

Scalar BrainTrainer::get_current_learning_rate() const {
    return config_.learning_rate;
}

void BrainTrainer::save_checkpoint(const std::string& path, size_t epoch) {
    std::filesystem::create_directories(config_.checkpoint_dir);
    
    // Save checkpoint (simplified - would need actual serialization)
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to save checkpoint: " << path << std::endl;
        return;
    }
    
    // Write epoch
    file.write(reinterpret_cast<const char*>(&epoch), sizeof(epoch));
    
    // Write metrics
    file.write(reinterpret_cast<const char*>(&metrics_.loss), sizeof(metrics_.loss));
    file.write(reinterpret_cast<const char*>(&metrics_.accuracy), sizeof(metrics_.accuracy));
    
    file.close();
    
    if (config_.verbose) {
        std::cout << "✓ Checkpoint saved: " << path << std::endl;
    }
}

void BrainTrainer::load_checkpoint(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to load checkpoint: " << path << std::endl;
        return;
    }
    
    size_t epoch;
    file.read(reinterpret_cast<char*>(&epoch), sizeof(epoch));
    file.read(reinterpret_cast<char*>(&metrics_.loss), sizeof(metrics_.loss));
    file.read(reinterpret_cast<char*>(&metrics_.accuracy), sizeof(metrics_.accuracy));
    
    file.close();
    
    if (config_.verbose) {
        std::cout << "✓ Checkpoint loaded: " << path << " (epoch " << epoch << ")" << std::endl;
    }
}

void BrainTrainer::initialize_optimizer() {
    momentum_buffers_.clear();
    velocity_buffers_.clear();
    step_count_ = 0;
}

std::vector<Eigen::VectorXd> BrainTrainer::compute_gradients(
    const TrainingSample& sample, const CognitiveResult& result) {
    // Simplified gradient computation
    // In a real implementation, this would use backpropagation through the brain system
    std::vector<Eigen::VectorXd> gradients;
    
    // Placeholder: return zero gradients
    gradients.push_back(Eigen::VectorXd::Zero(result.h_wm.size()));
    
    return gradients;
}

void BrainTrainer::log_progress(size_t step, const TrainingMetrics& metrics) {
    if (config_.verbose && step % config_.log_interval == 0) {
        std::cout << "Step " << step 
                  << " - Loss: " << metrics.loss 
                  << " - Acc: " << (metrics.accuracy * 100.0) << "%" 
                  << std::endl;
    }
}

bool BrainTrainer::should_stop_early() const {
    return epochs_without_improvement_ >= config_.patience;
}

void BrainTrainer::reset_metrics() {
    metrics_ = TrainingMetrics{};
}

TrainingMetrics BrainTrainer::evaluate(Dataset& dataset) {
    TrainingMetrics eval_metrics;
    Scalar total_loss = 0.0;
    Scalar total_accuracy = 0.0;
    
    for (size_t i = 0; i < dataset.size(); ++i) {
        auto sample = dataset.get(i);
        auto result = brain_.step(sample.input, 0.0);
        
        total_loss += compute_loss(sample, result);
        total_accuracy += compute_accuracy(sample, result);
    }
    
    eval_metrics.loss = total_loss / dataset.size();
    eval_metrics.accuracy = total_accuracy / dataset.size();
    eval_metrics.samples_processed = dataset.size();
    
    return eval_metrics;
}

// Loss functions
namespace loss {
    Scalar mse_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target) {
        return (pred - target).squaredNorm() / pred.size();
    }
    
    Scalar cross_entropy_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target) {
        Scalar loss = 0.0;
        for (int i = 0; i < pred.size(); ++i) {
            Scalar p = std::max(std::min(pred(i), 1.0 - 1e-7), 1e-7);
            loss -= target(i) * std::log(p);
        }
        return loss / pred.size();
    }
    
    Scalar l1_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target) {
        return (pred - target).cwiseAbs().sum() / pred.size();
    }
    
    Scalar huber_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target, Scalar delta) {
        Scalar loss = 0.0;
        for (int i = 0; i < pred.size(); ++i) {
            Scalar diff = std::abs(pred(i) - target(i));
            if (diff <= delta) {
                loss += 0.5 * diff * diff;
            } else {
                loss += delta * (diff - 0.5 * delta);
            }
        }
        return loss / pred.size();
    }
}

// RL utilities
namespace rl {
    Scalar compute_return(const std::vector<Scalar>& rewards, Scalar gamma) {
        Scalar G = 0.0;
        for (int t = rewards.size() - 1; t >= 0; --t) {
            G = rewards[t] + gamma * G;
        }
        return G;
    }
    
    std::vector<Scalar> compute_advantages(const std::vector<Scalar>& rewards,
                                           const std::vector<Scalar>& values,
                                           Scalar gamma) {
        std::vector<Scalar> advantages(rewards.size());
        Scalar advantage = 0.0;
        
        for (int t = rewards.size() - 1; t >= 0; --t) {
            Scalar delta = rewards[t] - values[t];
            if (t < static_cast<int>(rewards.size()) - 1) {
                delta += gamma * values[t + 1];
            }
            advantage = delta + gamma * advantage;
            advantages[t] = advantage;
        }
        
        return advantages;
    }
    
    Scalar policy_gradient_loss(const Eigen::VectorXd& action_logprobs,
                               const std::vector<Scalar>& advantages) {
        Scalar loss = 0.0;
        for (size_t i = 0; i < advantages.size(); ++i) {
            loss -= action_logprobs(i) * advantages[i];
        }
        return loss / advantages.size();
    }
}

} // namespace hab
