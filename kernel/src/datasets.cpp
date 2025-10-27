#include "brain/datasets.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace hab {

// Synthetic Dataset Implementation
SyntheticDataset::SyntheticDataset(Type type, size_t num_samples, int input_dim, int num_classes)
    : type_(type), rng_(std::random_device{}()) {
    
    switch (type) {
        case Type::RANDOM:
            generate_random(num_samples, input_dim, num_classes);
            break;
        case Type::XOR:
            generate_xor(num_samples);
            break;
        case Type::CIRCLE:
            generate_circle(num_samples, input_dim);
            break;
        case Type::SPIRAL:
            generate_spiral(num_samples, input_dim);
            break;
        case Type::GAUSSIAN_MIXTURE:
            generate_gaussian_mixture(num_samples, input_dim, num_classes);
            break;
    }
}

void SyntheticDataset::generate_random(size_t n, int dim, int classes) {
    std::uniform_real_distribution<Scalar> dist(-1.0, 1.0);
    std::uniform_int_distribution<int> class_dist(0, classes - 1);
    
    for (size_t i = 0; i < n; ++i) {
        TrainingSample sample;
        sample.input = Eigen::VectorXd::Zero(dim);
        for (int j = 0; j < dim; ++j) {
            sample.input(j) = dist(rng_);
        }
        
        int label = class_dist(rng_);
        sample.label = std::to_string(label);
        sample.target = Eigen::VectorXd::Zero(classes);
        sample.target(label) = 1.0;
        sample.reward = 0.0;
        
        samples_.push_back(sample);
    }
}

void SyntheticDataset::generate_xor(size_t n) {
    std::uniform_real_distribution<Scalar> noise(-0.1, 0.1);
    
    for (size_t i = 0; i < n; ++i) {
        TrainingSample sample;
        sample.input = Eigen::VectorXd::Zero(2);
        
        int x = (i % 2);
        int y = ((i / 2) % 2);
        
        sample.input(0) = x + noise(rng_);
        sample.input(1) = y + noise(rng_);
        
        int label = (x ^ y);
        sample.label = std::to_string(label);
        sample.target = Eigen::VectorXd::Zero(2);
        sample.target(label) = 1.0;
        sample.reward = 0.0;
        
        samples_.push_back(sample);
    }
}

void SyntheticDataset::generate_circle(size_t n, int dim) {
    if (dim < 2) {
        throw std::invalid_argument("generate_circle requires dim >= 2");
    }
    std::uniform_real_distribution<Scalar> angle_dist(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<Scalar> radius_dist(0.0, 1.0);
    std::normal_distribution<Scalar> noise(0.0, 0.05);
    
    for (size_t i = 0; i < n; ++i) {
        TrainingSample sample;
        sample.input = Eigen::VectorXd::Zero(dim);
        
        Scalar angle = angle_dist(rng_);
        Scalar radius = radius_dist(rng_);
        
        sample.input(0) = radius * std::cos(angle) + noise(rng_);
        sample.input(1) = radius * std::sin(angle) + noise(rng_);
        
        // Fill remaining dims with noise
        for (int j = 2; j < dim; ++j) {
            sample.input(j) = noise(rng_);
        }
        
        // Label: inside (0) or outside (1) circle
        int label = (radius > 0.5) ? 1 : 0;
        sample.label = std::to_string(label);
        sample.target = Eigen::VectorXd::Zero(2);
        sample.target(label) = 1.0;
        sample.reward = 0.0;
        
        samples_.push_back(sample);
    }
}

void SyntheticDataset::generate_spiral(size_t n, int dim) {
    if (dim < 2) {
        throw std::invalid_argument("generate_spiral requires dim >= 2");
    }
    std::normal_distribution<Scalar> noise(0.0, 0.05);
    
    for (size_t i = 0; i < n; ++i) {
        TrainingSample sample;
        sample.input = Eigen::VectorXd::Zero(dim);
        
        int label = static_cast<int>(i % 2);
        Scalar t = static_cast<Scalar>(i) / static_cast<Scalar>(n) * static_cast<Scalar>(4.0 * M_PI);
        Scalar r = t / static_cast<Scalar>(4.0 * M_PI);
        
        Scalar sign = (label == 0) ? static_cast<Scalar>(1.0) : static_cast<Scalar>(-1.0);
        sample.input(0) = sign * r * std::cos(t) + noise(rng_);
        sample.input(1) = sign * r * std::sin(t) + noise(rng_);
        
        for (int j = 2; j < dim; ++j) {
            sample.input(j) = noise(rng_);
        }
        
        sample.label = std::to_string(label);
        sample.target = Eigen::VectorXd::Zero(2);
        sample.target(label) = 1.0;
        sample.reward = 0.0;
        
        samples_.push_back(sample);
    }
}

void SyntheticDataset::generate_gaussian_mixture(size_t n, int dim, int classes) {
    if (classes <= 0) {
        classes = 1;
    }
    std::uniform_int_distribution<int> class_dist(0, classes - 1);

    for (size_t i = 0; i < n; ++i) {
        TrainingSample sample;
        sample.input = Eigen::VectorXd::Zero(dim);
    
        const int label = class_dist(rng_);
    
        // Each class has a different mean arranged on a circle
        Eigen::VectorXd mean = Eigen::VectorXd::Zero(dim);
        const Scalar angle = static_cast<Scalar>(2.0) * static_cast<Scalar>(std::acos(-1.0)) 
                           * static_cast<Scalar>(label) / static_cast<Scalar>(classes);
        mean(0) = static_cast<Scalar>(2.0) * std::cos(angle);
        if (dim > 1) mean(1) = static_cast<Scalar>(2.0) * std::sin(angle);
    
        // Sample from Gaussian
        std::normal_distribution<Scalar> dist(0.0, 0.5);
        for (int j = 0; j < dim; ++j) {
            sample.input(j) = mean(j) + dist(rng_);
        }
    
        sample.label = std::to_string(label);
        sample.target = Eigen::VectorXd::Zero(classes);
        sample.target(label) = 1.0;
        sample.reward = 0.0;
    
        samples_.push_back(sample);
    }
}

void SyntheticDataset::shuffle() {
    std::shuffle(samples_.begin(), samples_.end(), rng_);
}

std::string SyntheticDataset::name() const {
    switch (type_) {
        case Type::RANDOM: return "SyntheticRandom";
        case Type::XOR: return "SyntheticXOR";
        case Type::CIRCLE: return "SyntheticCircle";
        case Type::SPIRAL: return "SyntheticSpiral";
        case Type::GAUSSIAN_MIXTURE: return "SyntheticGaussianMixture";
        default: return "Synthetic";
    }
}

// Replay Buffer Implementation
ReplayBuffer::ReplayBuffer(size_t capacity) 
    : capacity_(capacity), rng_(std::random_device{}()) {
}

void ReplayBuffer::add(const TrainingSample& sample) {
    if (buffer_.size() < capacity_) {
        buffer_.push_back(sample);
    } else {
        buffer_[write_pos_] = sample;
    }
    write_pos_ = (write_pos_ + 1) % capacity_;
}

void ReplayBuffer::add_batch(const std::vector<TrainingSample>& samples) {
    for (const auto& sample : samples) {
        add(sample);
    }
}

TrainingSample ReplayBuffer::get(size_t index) {
    const size_t actual_size = buffer_.size();
    if (actual_size == 0) {
        throw std::out_of_range("Cannot get from an empty ReplayBuffer.");
    }
    return buffer_[index % actual_size];
}

void ReplayBuffer::shuffle() {
    std::shuffle(buffer_.begin(), buffer_.end(), rng_);
}

std::vector<TrainingSample> ReplayBuffer::sample_batch(size_t batch_size) {
    std::vector<TrainingSample> batch;
    std::uniform_int_distribution<size_t> dist(0, size() - 1);
    
    for (size_t i = 0; i < batch_size; ++i) {
        batch.push_back(buffer_[dist(rng_)]);
    }
    
    return batch;
}

// Data augmentation
namespace augmentation {
    Eigen::VectorXd add_noise(const Eigen::VectorXd& input, Scalar std_dev) {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::normal_distribution<Scalar> dist(0.0, std_dev);
        
        Eigen::VectorXd noisy = input;
        for (int i = 0; i < input.size(); ++i) {
            noisy(i) += dist(rng);
        }
        return noisy;
    }
    
    Eigen::VectorXd normalize(const Eigen::VectorXd& input, Scalar mean, Scalar std) {
        return (input.array() - mean) / std;
    }
}

// Dataset splitting
namespace split {
    std::pair<std::vector<size_t>, std::vector<size_t>> 
    train_test_split(size_t dataset_size, Scalar test_ratio, int seed) {
        std::vector<size_t> indices(dataset_size);
        std::iota(indices.begin(), indices.end(), 0);
        
        std::mt19937 rng(seed);
        std::shuffle(indices.begin(), indices.end(), rng);
        
        size_t test_size = static_cast<size_t>(dataset_size * test_ratio);
        size_t train_size = dataset_size - test_size;
        
        std::vector<size_t> train_indices(indices.begin(), indices.begin() + train_size);
        std::vector<size_t> test_indices(indices.begin() + train_size, indices.end());
        
        return {train_indices, test_indices};
    }
}

} // namespace hab
