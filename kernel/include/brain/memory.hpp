#pragma once

#include "utils.hpp"
#include <Eigen/Dense>
#include <deque>
#include <vector>
#include <map>

namespace hab {

// Memory episode: (gw_state, qw_onehot, action, reward, timestamp)
struct Episode {
    Eigen::VectorXd gw_state;
    Eigen::VectorXd qw_onehot;
    int action;
    Scalar reward;
    TimePoint timestamp;
    
    Episode() : action(0), reward(0.0), timestamp(std::chrono::steady_clock::now()) {}
};

// Short-term memory configuration
struct STMConfig {
    int capacity = 20;          // Number of recent episodes
    Scalar decay_tau = 3.0;     // Exponential decay time constant (seconds)
};

// Long-term memory configuration
struct LTMConfig {
    int max_memories = 1000;     // Maximum LTM capacity
    Scalar importance_threshold = 0.5; // Consolidation threshold
};

// Short-Term Memory - Recent experience buffer with temporal decay
class ShortTermMemory {
public:
    explicit ShortTermMemory(const STMConfig& config = STMConfig{});
    
    void add(const Episode& episode);
    
    // Retrieve weighted average of recent memories
    Eigen::VectorXd recall(const Eigen::VectorXd& query, Scalar beta = 10.0) const;
    
    // Get importance score for consolidation decision
    Scalar get_importance(const Episode& episode) const;
    
    const std::deque<Episode>& episodes() const { return episodes_; }
    
private:
    STMConfig config_;
    std::deque<Episode> episodes_;
};

// Long-Term Memory - Stable schema storage
class LongTermMemory {
public:
    explicit LongTermMemory(const LTMConfig& config = LTMConfig{});
    
    // Consolidate important episodes from STM
    void consolidate(const Episode& episode, Scalar importance);
    
    // Retrieve similar memories
    std::vector<Episode> retrieve(const Eigen::VectorXd& query, int top_k = 5) const;
    
    // Cosine similarity
    Scalar similarity(const Eigen::VectorXd& a, const Eigen::VectorXd& b) const;
    
    size_t size() const { return memories_.size(); }
    
private:
    LTMConfig config_;
    std::vector<Episode> memories_;
    std::vector<Scalar> importance_scores_;
};

// Integrated Memory System
class Memory {
public:
    explicit Memory(const STMConfig& stm_config = STMConfig{},
                   const LTMConfig& ltm_config = LTMConfig{});
    
    void store(const Episode& episode);
    Eigen::VectorXd recall(const Eigen::VectorXd& query) const;
    
    const ShortTermMemory& stm() const { return stm_; }
    const LongTermMemory& ltm() const { return ltm_; }
    
private:
    ShortTermMemory stm_;
    LongTermMemory ltm_;
};

} // namespace hab
