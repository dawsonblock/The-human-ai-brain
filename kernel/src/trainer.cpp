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
    if (config_.enable_checkpoints) {
        try {
            std::error_code ec;
            std::filesystem::create_directories(config_.checkpoint_dir, ec);
            if (ec) {
                std::cerr << "Checkpoint directory create failed: " << config_.checkpoint_dir
                          << " (" << ec.message() << ")\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Checkpoint directory exception: " << e.what() << "\n";
        }
    }

    if (config_.verbose) {
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::string title = "Brain Training - " + dataset.name();
        std::cout << "║ " << title << std::string(58 - title.length(), ' ') << "║\n";
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
        apply_learning_rate_schedule(epoch);
        if (config_.enable_checkpoints && (epoch + 1) % config_.checkpoint_interval == 0) {
            std::string path = config_.checkpoint_dir + "/checkpoint_epoch_" + std::to_string(epoch + 1) + ".bin";
            save_checkpoint(path, epoch + 1);
        }
    }

    if (config_.verbose) {
        std::cout << "\n✓ Training complete!\n";
        std::cout << "  Final Loss: " << metrics_.loss << "\n";
        std::cout << "  Final Accuracy: " << (metrics_.accuracy * 100.0) << "%\n";
    }
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
    if (dataset.size() == 0) {
        if (config_.verbose) {
            std::cout << "Epoch " << (epoch + 1) << "/" << config_.num_epochs
                      << " - Skipped (empty dataset)\n";
        }
        return;
    }

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

        const size_t start_idx = batch_id * config_.batch_size;
        const size_t end_idx = std::min(start_idx + config_.batch_size, dataset.size());
        if (config_.verbose && config_.log_interval > 0 && (batch_id + 1) % config_.log_interval == 0) {
            auto& os = std::cout;
            std::ios old_state(nullptr);
            old_state.copyfmt(os);
            os << "Epoch " << (epoch + 1) << "/" << config_.num_epochs 
               << " - Batch " << (batch_id + 1) << "/" << num_batches
               << " - Loss: " << std::fixed << std::setprecision(4) << metrics_.loss
               << " - Acc: " << std::fixed << std::setprecision(2) << (metrics_.accuracy * 100.0) << "%"
               << " - LR: " << std::scientific << std::setprecision(2) << get_current_learning_rate();
            // Print newline on last batch to avoid leaving the cursor mid-line
            if (batch_id + 1 == num_batches) {
                os << std::endl;
            } else {
                os << "\r" << std::flush;
            }
            os.copyfmt(old_state);
        }
        epoch_loss += metrics_.loss * batch.samples.size();
        epoch_accuracy += metrics_.accuracy * batch.samples.size();
        samples_in_epoch += batch.samples.size();
        
        // Logging
        if (config_.verbose && (batch_id + 1) % config_.log_interval == 0) {
            auto& os = std::cout;
            std::ios old_state(nullptr);
            old_state.copyfmt(os);
            os << "Epoch " << (epoch + 1) << "/" << config_.num_epochs 
               << " - Batch " << (batch_id + 1) << "/" << num_batches
               << " - Loss: " << std::fixed << std::setprecision(4) << metrics_.loss
               << " - Acc: " << std::fixed << std::setprecision(2) << (metrics_.accuracy * 100.0) << "%"
               << " - LR: " << std::scientific << std::setprecision(2) << get_current_learning_rate()
               << "\r" << std::flush;
            os.copyfmt(old_state); // restore original formatting
        }
        
        if (batch_callback_) {
            batch_callback_(batch_id, batch, metrics_);
        }
    }
    
    if (samples_in_epoch == 0) {
        // No samples processed; avoid division by zero.
        if (config_.verbose) {
            std::cout << "Epoch " << (epoch + 1) << "/" << config_.num_epochs
                      << " - No samples processed\n";
        }
        return;
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
    case TrainerConfig::Mode::REINFORCEMENT: {
        // Entropy-regularized objective: negative reward plus bounded entropy bonus
        const Scalar entropy = std::clamp(result.entropy, static_cast<Scalar>(0.0), static_cast<Scalar>(5.0));
        const Scalar entropy_coef = static_cast<Scalar>(0.01); // small, stable bonus
        return static_cast<Scalar>(-1.0) * sample.reward + entropy_coef * entropy;
    }
    metrics_.collapse_rate = (static_cast<Scalar>(collapse_count) / static_cast<Scalar>(batch.samples.size())) * 8.2;  // Approximate Hz
}

Scalar BrainTrainer::compute_loss(const TrainingSample& sample, const CognitiveResult& result) {
    switch (config_.mode) {
        case TrainerConfig::Mode::SUPERVISED: {
            const auto tgt_size = sample.target.size();
            const auto pred_size = result.h_global.size();
            if (tgt_size == 0 || pred_size == 0 || tgt_size != pred_size) {
                // Dimension mismatch or missing target; avoid invalid Eigen ops
                return 0.0;
            }
            return loss::mse_loss(result.h_global, sample.target);
        }
        case TrainerConfig::Mode::REINFORCEMENT:
            // Policy gradient loss (negative expected return)
            return -sample.reward * result.entropy;  // Entropy bonus for exploration
        case TrainerConfig::Mode::SELF_SUPERVISED: {
            const auto in_size = sample.input.size();
            const auto wm_size = result.h_wm.size();
            if (in_size == 0 || wm_size == 0 || in_size != wm_size) {
                return 0.0;
            }
            return loss::mse_loss(result.h_wm, sample.input);
        }
        default:
            return 0.0;
    }
}

Scalar BrainTrainer::compute_accuracy(const TrainingSample& sample, const CognitiveResult& result) {
    if (sample.label.empty() || result.h_global.size() == 0) {
        return 0.0;
    }
    
    // Find predicted class (argmax of global state - first 10 dims or available size)
    int predicted_class = 0;
    Scalar max_val = result.h_global(0);
    const int search_dims = std::min(10, static_cast<int>(result.h_global.size()));
    for (int i = 1; i < search_dims; ++i) {
        step_count_++;

        Scalar lr = get_current_learning_rate();
        if (!std::isfinite(lr) || lr <= 0) {
            lr = static_cast<Scalar>(1e-6);
        }

        auto is_bad = [](const Eigen::VectorXd& g) {
            if (g.size() == 0) return true;
            for (int i = 0; i < g.size(); ++i) {
                if (!std::isfinite(g[i])) return true;
            }
            return false;
        };

        switch (config_.optimizer) {
            case TrainerConfig::Optimizer::SGD: {
                for (size_t i = 0; i < gradients.size(); ++i) {
                    if (is_bad(gradients[i])) continue;
                    if (i >= momentum_buffers_.size()) {
                        momentum_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                    }
                    momentum_buffers_[i] = config_.momentum * momentum_buffers_[i] - lr * gradients[i];
                    // Apply update to parameters here
                }
                break;
            }
            case TrainerConfig::Optimizer::ADAM: {
                for (size_t i = 0; i < gradients.size(); ++i) {
                    if (is_bad(gradients[i])) continue;
                    if (i >= momentum_buffers_.size()) {
                        momentum_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                        velocity_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                    }
                    momentum_buffers_[i] = config_.beta1 * momentum_buffers_[i] + (1.0 - config_.beta1) * gradients[i];
                    velocity_buffers_[i] = config_.beta2 * velocity_buffers_[i] + (1.0 - config_.beta2) * gradients[i].array().square().matrix();

                    const Scalar bias_correction1 = std::max<Scalar>(1e-12, 1.0 - std::pow(config_.beta1, static_cast<double>(step_count_)));
                    const Scalar bias_correction2 = std::max<Scalar>(1e-12, 1.0 - std::pow(config_.beta2, static_cast<double>(step_count_)));
                    Eigen::ArrayXd m_hat = momentum_buffers_[i].array() / bias_correction1;
                    Eigen::ArrayXd v_hat = velocity_buffers_[i].array() / bias_correction2;

                    Eigen::ArrayXd denom = v_hat.sqrt();
                    denom = denom.max(config_.epsilon); // clamp to epsilon
                    Eigen::ArrayXd update = (-lr) * (m_hat / denom);
                    (void)update; // Apply update to parameters here
                }
                break;
            }
            case TrainerConfig::Optimizer::RMSPROP: {
                for (size_t i = 0; i < gradients.size(); ++i) {
                    if (is_bad(gradients[i])) continue;
                    if (i >= velocity_buffers_.size()) {
                        velocity_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                    }
                    velocity_buffers_[i] = config_.momentum * velocity_buffers_[i] +
                                           (1.0 - config_.momentum) * gradients[i].array().square().matrix();

                    Eigen::ArrayXd denom = velocity_buffers_[i].array().sqrt();
                    denom = denom.max(config_.epsilon); // clamp to epsilon
                    Eigen::ArrayXd update = (-lr) * gradients[i].array() / denom;
                    (void)update; // Apply update to parameters here
                Eigen::ArrayXd m_hat = momentum_buffers_[i].array() / bias_correction1;
                Eigen::ArrayXd v_hat = velocity_buffers_[i].array() / bias_correction2;

                Eigen::ArrayXd denom = v_hat.sqrt() + config_.epsilon;
                Eigen::ArrayXd update = (-lr) * (m_hat / denom);
                (void)update; // Apply update to parameters here
            }
            break;
        }
        case TrainerConfig::Optimizer::RMSPROP: {
            for (size_t i = 0; i < gradients.size(); ++i) {
                if (is_bad(gradients[i])) continue;
                if (i >= velocity_buffers_.size()) {
                    velocity_buffers_.push_back(Eigen::VectorXd::Zero(gradients[i].size()));
                }
                velocity_buffers_[i] = config_.momentum * velocity_buffers_[i] +
                                       (1.0 - config_.momentum) * gradients[i].array().square().matrix();

                Eigen::ArrayXd update = (-lr) * gradients[i].array() / (velocity_buffers_[i].array().sqrt() + config_.epsilon);
                (void)update; // Apply update to parameters here
            }
            break;
        }
    }
}
// In constructor initialization list, ensure initialization occurs:
BrainTrainer::BrainTrainer(BrainSystem& brain, const TrainerConfig& config)
    : brain_(brain), config_(config) {
    // Initialize initial_learning_rate_ safely
    if (std::isfinite(config_.learning_rate) && config_.learning_rate > 0) {
        initial_learning_rate_ = config_.learning_rate;
    } else {
        initial_learning_rate_ = static_cast<Scalar>(1e-3);
        config_.learning_rate = initial_learning_rate_;
    }
    initialize_optimizer();
}

void BrainTrainer::apply_learning_rate_schedule(size_t epoch) {
    const Scalar min_lr = static_cast<Scalar>(1e-12);
    const Scalar max_lr = static_cast<Scalar>(1.0);
    const Scalar safe_decay = std::clamp(config_.lr_decay, static_cast<Scalar>(1e-6), static_cast<Scalar>(1.0));

    // Fallback if initial_learning_rate_ is invalid
    if (!std::isfinite(initial_learning_rate_) || initial_learning_rate_ <= 0) {
        initial_learning_rate_ = std::clamp(config_.learning_rate > 0 ? config_.learning_rate
                                                                      : static_cast<Scalar>(1e-3),
                                            min_lr, max_lr);
    }

    switch (config_.lr_schedule) {
        case TrainerConfig::LRSchedule::STEP:
            if ((epoch + 1) % std::max<size_t>(1, config_.lr_step_size) == 0) {
                config_.learning_rate = std::clamp(config_.learning_rate * safe_decay, min_lr, max_lr);
            }
            break;
        case TrainerConfig::LRSchedule::EXPONENTIAL: {
            Scalar lr = initial_learning_rate_ * std::pow(safe_decay, static_cast<Scalar>(epoch + 1));
            config_.learning_rate = std::clamp(lr, min_lr, max_lr);
            break;
        }
        case TrainerConfig::LRSchedule::COSINE: {
            const Scalar denom = static_cast<Scalar>(std::max<size_t>(1, config_.num_epochs));
            const Scalar t = static_cast<Scalar>(epoch + 1) / denom;
            const Scalar pi = static_cast<Scalar>(M_PI);
            Scalar lr = initial_learning_rate_ * static_cast<Scalar>(0.5) * (1.0 + std::cos(pi * t));
            config_.learning_rate = std::clamp(lr, min_lr, max_lr);
            break;
        }
        default:
            // CONSTANT: keep current learning_rate within bounds
            config_.learning_rate = std::clamp(config_.learning_rate, min_lr, max_lr);
            break;
                }
                break;
            case TrainerConfig::LRSchedule::EXPONENTIAL: {
                Scalar lr = initial_learning_rate_ * std::pow(safe_decay, static_cast<Scalar>(epoch + 1));
                config_.learning_rate = std::clamp(lr, min_lr, max_lr);
                break;
            }
            case TrainerConfig::LRSchedule::COSINE: {
                const Scalar denom = static_cast<Scalar>(std::max<size_t>(1, config_.num_epochs));
                const Scalar t = static_cast<Scalar>(epoch + 1) / denom;
                const Scalar pi = static_cast<Scalar>(M_PI);
                Scalar lr = initial_learning_rate_ * static_cast<Scalar>(0.5) * (1.0 + std::cos(pi * t));
                config_.learning_rate = std::clamp(lr, min_lr, max_lr);
                break;
            }
            default:
                break;
        }

        if (!std::isfinite(config_.learning_rate) || config_.learning_rate <= 0) {
            config_.learning_rate = std::clamp(initial_learning_rate_, min_lr, max_lr);
        } else {
            config_.learning_rate = std::clamp(config_.learning_rate, min_lr, max_lr);
        }
}
    
            // Write epoch
            file.write(reinterpret_cast<const char*>(&epoch), sizeof(epoch));
            if (!file) {
                std::cerr << "Failed to write epoch to checkpoint: " << path << std::endl;
                return;
            }
    
            // Write metrics
            file.write(reinterpret_cast<const char*>(&metrics_.loss), sizeof(metrics_.loss));
            file.write(reinterpret_cast<const char*>(&metrics_.accuracy), sizeof(metrics_.accuracy));
            if (!file) {
                std::cerr << "Failed to write metrics to checkpoint: " << path << std::endl;
                return;
            }
    
            file.close();
            if (!file) {
                std::cerr << "Failed to finalize checkpoint: " << path << std::endl;
                return;
            }
    
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
    
            size_t epoch = 0;
            file.read(reinterpret_cast<char*>(&epoch), sizeof(epoch));
            if (file.gcount() != static_cast<std::streamsize>(sizeof(epoch))) {
                std::cerr << "Corrupt checkpoint (epoch) in: " << path << std::endl;
                return;
            }

                file.close();

                if (config_.verbose) {
                    std::cout << "✓ Checkpoint loaded: " << path << " (epoch " << epoch << ")" << std::endl;
                }
            }

            file.read(reinterpret_cast<char*>(&metrics_.accuracy), sizeof(metrics_.accuracy));
            if (file.gcount() != static_cast<std::streamsize>(sizeof(metrics_.accuracy))) {
                std::cerr << "Corrupt checkpoint (accuracy) in: " << path << std::endl;
                return;
            }
    
            if (!file.good() && !file.eof()) {
                std::cerr << "Error reading checkpoint: " << path << std::endl;
                return;
            }
            file.close();
    if (config_.verbose) {
        std::cout << "✓ Checkpoint saved: " << path << std::endl;
    }
}

// Remove the malformed duplicate checkpoint code above.
void BrainTrainer::load_checkpoint(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to load checkpoint: " << path << std::endl;
        return;
    }

    size_t epoch = 0;
    file.read(reinterpret_cast<char*>(&epoch), sizeof(epoch));
    if (file.gcount() != static_cast<std::streamsize>(sizeof(epoch))) {
        std::cerr << "Corrupt checkpoint (epoch) in: " << path << std::endl;
        return;
    }

    file.read(reinterpret_cast<char*>(&metrics_.loss), sizeof(metrics_.loss));
    if (file.gcount() != static_cast<std::streamsize>(sizeof(metrics_.loss))) {
        std::cerr << "Corrupt checkpoint (loss) in: " << path << std::endl;
        return;
    }

    file.read(reinterpret_cast<char*>(&metrics_.accuracy), sizeof(metrics_.accuracy));
    if (file.gcount() != static_cast<std::streamsize>(sizeof(metrics_.accuracy))) {
        std::cerr << "Corrupt checkpoint (accuracy) in: " << path << std::endl;
        return;
    }

    if (!file.good() && !file.eof()) {
        std::cerr << "Error reading checkpoint: " << path << std::endl;
        return;
    }
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
    TrainingMetrics eval_metrics{};
    const size_t n = dataset.size();
    if (n == 0) {
        // Avoid division by zero; return default-initialized metrics
        eval_metrics.loss = 0.0;
        eval_metrics.accuracy = 0.0;
        eval_metrics.samples_processed = 0;
        return eval_metrics;
    }

    Scalar total_loss = 0.0;
    Scalar total_accuracy = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        auto sample = dataset.get(i);
        auto result = brain_.step(sample.input, 0.0);
        
        total_loss += compute_loss(sample, result);
        total_accuracy += compute_accuracy(sample, result);
    }
    
    eval_metrics.loss = total_loss / static_cast<Scalar>(n);
    eval_metrics.accuracy = total_accuracy / static_cast<Scalar>(n);
    eval_metrics.samples_processed = n;
    
    
    return eval_metrics;
}

// Loss functions
namespace loss {
    using hab::Scalar;
    
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
    using hab::Scalar;
    
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
        
        for (size_t i = 0; i < rewards.size(); ++i) {
            size_t t = rewards.size() - 1 - i;
            Scalar delta = rewards[t] - values[t];
            if (t < rewards.size() - 1) {
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
