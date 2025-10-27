#pragma once

#include "brain/trainer.hpp"
#include <fstream>
#include <random>

namespace hab {

// MNIST dataset loader
class MNISTDataset : public Dataset {
public:
    MNISTDataset(const std::string& images_path, 
                 const std::string& labels_path,
                 bool normalize = true);
    
    size_t size() const override { return images_.size(); }
    TrainingSample get(size_t index) override;
    void shuffle() override;
    std::string name() const override { return "MNIST"; }
    
    // MNIST-specific
    int get_label(size_t index) const { return labels_[index]; }
    Eigen::VectorXd get_image(size_t index) const { return images_[index]; }
    
private:
    std::vector<Eigen::VectorXd> images_;  // 784D vectors (28x28)
    std::vector<int> labels_;               // 0-9
    std::vector<size_t> indices_;           // For shuffling
    bool normalize_;
    std::mt19937 rng_;
    
    void load_images(const std::string& path);
    void load_labels(const std::string& path);
    int read_int32(std::ifstream& file);
};

// Synthetic dataset for testing
class SyntheticDataset : public Dataset {
public:
    enum class Type {
        RANDOM,           // Random inputs and targets
        XOR,              // XOR problem
        CIRCLE,           // Circle classification
        SPIRAL,           // Two spirals
        GAUSSIAN_MIXTURE  // Mixture of Gaussians
    };
    
    SyntheticDataset(Type type, size_t num_samples, int input_dim, int num_classes = 2);
    
    size_t size() const override { return samples_.size(); }
    TrainingSample get(size_t index) override { return samples_[index]; }
    void shuffle() override;
    std::string name() const override;
    
private:
    std::vector<TrainingSample> samples_;
    Type type_;
    std::mt19937 rng_;
    
    void generate_random(size_t n, int dim, int classes);
    void generate_xor(size_t n);
    void generate_circle(size_t n, int dim);
    void generate_spiral(size_t n, int dim);
    void generate_gaussian_mixture(size_t n, int dim, int classes);
};

// Reinforcement learning environment dataset
class RLEnvironmentDataset : public Dataset {
public:
    struct Transition {
        Eigen::VectorXd state;
        Eigen::VectorXd action;
        Scalar reward;
        Eigen::VectorXd next_state;
        bool done;
    };
    
    explicit RLEnvironmentDataset(size_t capacity = 10000);
    
    void add_transition(const Transition& transition);
    void add_episode(const std::vector<Transition>& episode);
    
    size_t size() const override { return transitions_.size(); }
    TrainingSample get(size_t index) override;
    void shuffle() override;
    std::string name() const override { return "RLEnvironment"; }
    
    // RL-specific
    const Transition& get_transition(size_t index) const { return transitions_[index]; }
    void compute_returns(Scalar gamma);
    
private:
    std::vector<Transition> transitions_;
    std::vector<Scalar> returns_;  // Computed returns
    std::vector<size_t> indices_;
    size_t capacity_;
    std::mt19937 rng_;
};

// CSV dataset loader
class CSVDataset : public Dataset {
public:
    struct Config {
        std::string filepath;
        bool has_header = true;
        char delimiter = ',';
        int target_column = -1;  // -1 = last column
        bool normalize = true;
        std::vector<int> feature_columns;  // Empty = all except target
    };
    
    explicit CSVDataset(const Config& config);
    
    size_t size() const override { return samples_.size(); }
    TrainingSample get(size_t index) override { return samples_[index]; }
    void shuffle() override;
    std::string name() const override { return "CSV"; }
    
private:
    std::vector<TrainingSample> samples_;
    std::vector<size_t> indices_;
    Config config_;
    std::mt19937 rng_;
    
    void load_from_file();
    std::vector<std::string> parse_line(const std::string& line);
    void normalize_features();
};

// Memory replay buffer for online learning
class ReplayBuffer : public Dataset {
public:
    explicit ReplayBuffer(size_t capacity = 100000);
    
    void add(const TrainingSample& sample);
    void add_batch(const std::vector<TrainingSample>& samples);
    
    size_t size() const override {
        if (capacity_ == 0) return 0;
        return std::min(buffer_.size(), capacity_);
    }
    TrainingSample get(size_t index) override;
    void shuffle() override;
    std::string name() const override { return "ReplayBuffer"; }
    
    // Sampling strategies
    std::vector<TrainingSample> sample_batch(size_t batch_size);
    
    void clear() { buffer_.clear(); write_pos_ = 0; }
    bool is_full() const { return capacity_ > 0 && buffer_.size() >= capacity_; }
    
private:
    std::vector<TrainingSample> buffer_;
    size_t capacity_;
    size_t write_pos_ = 0;
    std::vector<Scalar> priorities_;  // For prioritized replay
    std::mt19937 rng_;
};

// Data augmentation utilities
namespace augmentation {
    Eigen::VectorXd add_noise(const Eigen::VectorXd& input, Scalar std_dev);
    Eigen::VectorXd normalize(const Eigen::VectorXd& input, Scalar mean, Scalar std);
}

// Dataset splitting utilities
namespace split {
    std::pair<std::vector<size_t>, std::vector<size_t>> 
    train_test_split(size_t dataset_size, Scalar test_ratio = 0.2, int seed = 42);
    
    std::vector<std::vector<size_t>> 
    k_fold_split(size_t dataset_size, int k = 5, int seed = 42);
}

} // namespace hab
