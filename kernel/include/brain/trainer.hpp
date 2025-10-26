#pragma once

#include "brain/brain_system.hpp"
#include "brain/utils.hpp"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace hab {

// Training sample
struct TrainingSample {
    Eigen::VectorXd input;      // Input stimulus
    Eigen::VectorXd target;     // Target output (optional, for supervised)
    Scalar reward;              // Reward signal (for RL)
    std::string label;          // Class label (for classification)
    std::map<std::string, Scalar> metadata;
};

// Training batch
struct TrainingBatch {
    std::vector<TrainingSample> samples;
    int batch_id;
    size_t epoch;
};

// Training metrics
struct TrainingMetrics {
    Scalar loss;                // Current loss
    Scalar accuracy;            // Classification accuracy
    Scalar avg_reward;          // Average reward (RL)
    Scalar avg_entropy;         // Average quantum entropy
    Scalar collapse_rate;       // Collapse frequency
    size_t samples_processed;   // Total samples
    
    // Per-epoch statistics
    std::vector<Scalar> epoch_losses;
    std::vector<Scalar> epoch_accuracies;
    std::vector<Scalar> epoch_rewards;
};

// Training configuration
struct TrainerConfig {
    // General
    size_t num_epochs = 10;
    size_t batch_size = 32;
    Scalar learning_rate = 1e-3;
    bool shuffle = true;
    
    // Optimization
    enum class Optimizer { SGD, ADAM, RMSPROP };
    Optimizer optimizer = Optimizer::ADAM;
    Scalar momentum = 0.9;
    Scalar beta1 = 0.9;        // Adam beta1
    Scalar beta2 = 0.999;      // Adam beta2
    Scalar epsilon = 1e-8;     // Adam epsilon
    Scalar weight_decay = 0.0; // L2 regularization
    
    // Learning rate schedule
    enum class LRSchedule { CONSTANT, STEP, EXPONENTIAL, COSINE };
    LRSchedule lr_schedule = LRSchedule::CONSTANT;
    Scalar lr_decay = 0.1;
    size_t lr_step_size = 10;  // Epochs between decay
    
    // Training mode
    enum class Mode { SUPERVISED, REINFORCEMENT, SELF_SUPERVISED };
    Mode mode = Mode::SUPERVISED;
    
    // Reinforcement learning
    Scalar gamma = 0.99;       // Discount factor
    Scalar reward_scale = 1.0;
    
    // Checkpointing
    bool enable_checkpoints = true;
    size_t checkpoint_interval = 1;  // Epochs
    std::string checkpoint_dir = "checkpoints";
    
    // Logging
    bool verbose = true;
    size_t log_interval = 100;  // Steps
    
    // Early stopping
    bool enable_early_stopping = false;
    size_t patience = 5;       // Epochs without improvement
    Scalar min_delta = 1e-4;   // Minimum improvement
    
    // Validation
    Scalar validation_split = 0.2;
    size_t validation_interval = 1;  // Epochs
};

// Dataset interface
class Dataset {
public:
    virtual ~Dataset() = default;
    
    virtual size_t size() const = 0;
    virtual TrainingSample get(size_t index) = 0;
    virtual void shuffle() = 0;
    virtual std::string name() const = 0;
};

// Brain trainer
class BrainTrainer {
public:
    explicit BrainTrainer(BrainSystem& brain, const TrainerConfig& config = TrainerConfig{});
    
    // Training
    void train(Dataset& dataset);
    void train_epoch(Dataset& dataset, size_t epoch);
    void train_batch(const TrainingBatch& batch);
    
    // Evaluation
    TrainingMetrics evaluate(Dataset& dataset);
    Scalar compute_loss(const TrainingSample& sample, const CognitiveResult& result);
    Scalar compute_accuracy(const TrainingSample& sample, const CognitiveResult& result);
    
    // Optimization
    void update_parameters(const std::vector<Eigen::VectorXd>& gradients);
    void apply_learning_rate_schedule(size_t epoch);
    Scalar get_current_learning_rate() const;
    
    // Checkpointing
    void save_checkpoint(const std::string& path, size_t epoch);
    void load_checkpoint(const std::string& path);
    
    // Metrics
    const TrainingMetrics& metrics() const { return metrics_; }
    void reset_metrics();
    
    // Callbacks
    using EpochCallback = std::function<void(size_t epoch, const TrainingMetrics&)>;
    using BatchCallback = std::function<void(size_t step, const TrainingBatch&, const TrainingMetrics&)>;
    
    void set_epoch_callback(EpochCallback callback) { epoch_callback_ = callback; }
    void set_batch_callback(BatchCallback callback) { batch_callback_ = callback; }
    
private:
    BrainSystem& brain_;
    TrainerConfig config_;
    TrainingMetrics metrics_;
    
    // Optimizer state
    std::vector<Eigen::VectorXd> momentum_buffers_;
    std::vector<Eigen::VectorXd> velocity_buffers_;  // For Adam
    size_t step_count_ = 0;
    
    // Early stopping
    Scalar best_loss_ = std::numeric_limits<Scalar>::max();
    size_t epochs_without_improvement_ = 0;
    
    // Callbacks
    EpochCallback epoch_callback_;
    BatchCallback batch_callback_;
    
    // Helper methods
    void initialize_optimizer();
    std::vector<Eigen::VectorXd> compute_gradients(const TrainingSample& sample, 
                                                     const CognitiveResult& result);
    void log_progress(size_t step, const TrainingMetrics& metrics);
    bool should_stop_early() const;
};

// Supervised learning loss functions
namespace loss {
    Scalar mse_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target);
    Scalar cross_entropy_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target);
    Scalar l1_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target);
    Scalar huber_loss(const Eigen::VectorXd& pred, const Eigen::VectorXd& target, Scalar delta = 1.0);
}

// Reinforcement learning utilities
namespace rl {
    Scalar compute_return(const std::vector<Scalar>& rewards, Scalar gamma);
    std::vector<Scalar> compute_advantages(const std::vector<Scalar>& rewards,
                                           const std::vector<Scalar>& values,
                                           Scalar gamma);
    Scalar policy_gradient_loss(const Eigen::VectorXd& action_logprobs,
                               const std::vector<Scalar>& advantages);
}

} // namespace hab
