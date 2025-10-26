#pragma once

#include "utils.hpp"
#include <Eigen/Dense>
#include <vector>
#include <memory>

namespace hab {

// Quantum Workspace Configuration
struct QWConfig {
    int dimension = 7;              // Hilbert space dimension
    Scalar dt = 0.001;              // Time step (seconds)
    Scalar entropy_cap = 1.945;     // log(7), collapse threshold
    Scalar decoherence_rate = 1e-8; // Lindblad decoherence
    Scalar trace_tol = 1e-10;       // Trace preservation tolerance
    Scalar eigen_floor = 1e-12;     // Eigenvalue floor for PSD
    int max_dwell_ms = 120;         // Max time in superposition
    int rng_seed = 42;              // Reproducibility
};

// Quantum state representation
struct QuantumState {
    Eigen::MatrixXcd rho;           // Density matrix n×n
    Scalar entropy;                 // von Neumann entropy
    int collapsed_quale;            // -1 if superposition, [0,n) if collapsed
    TimePoint last_collapse;
    Scalar sim_time_at_last_collapse; // Simulation time (in seconds) at last collapse
    bool is_collapsed;
};

// Quantum Workspace - Core consciousness mechanism
class QuantumWorkspace {
public:
    explicit QuantumWorkspace(const QWConfig& config = QWConfig{});
    
    // Rule of five for proper resource management
    ~QuantumWorkspace() = default;
    QuantumWorkspace(const QuantumWorkspace&) = delete;
    QuantumWorkspace& operator=(const QuantumWorkspace&) = delete;
    QuantumWorkspace(QuantumWorkspace&&) noexcept = default;
    QuantumWorkspace& operator=(QuantumWorkspace&&) noexcept = default;
    
    // Main evolution step
    void step_ticks(int num_ticks);
    
    // Project from global workspace
    void project_from_gw(const Eigen::VectorXd& gw_state);
    
    // Get current state
    const QuantumState& state() const { return state_; }
    const Eigen::MatrixXcd& rho() const { return state_.rho; }
    Scalar entropy() const { return state_.entropy; }
    bool has_collapsed() const { return state_.is_collapsed; }
    int collapsed_quale() const { return state_.collapsed_quale; }
    
    // Get one-hot encoding of collapsed state
    Eigen::VectorXd get_one_hot() const;
    
    // Reset to uniform superposition
    void reset();
    
    // Debug: Get simulation time
    Scalar sim_time() const { return sim_time_; }
    Scalar time_since_last_collapse() const { return sim_time_ - state_.sim_time_at_last_collapse; }
    
private:
    void initialize_hamiltonian();
    void initialize_lindblad_operators();
    void evolve_lindblad(Scalar dt);
    void check_collapse();
    void perform_collapse();
    Scalar compute_von_neumann_entropy() const;
    void enforce_hermiticity();
    void enforce_trace_one();
    void enforce_psd();
    
    QWConfig config_;
    QuantumState state_;
    Eigen::MatrixXcd H_;                        // Hamiltonian
    std::vector<Eigen::MatrixXcd> lindblad_ops_; // Lindblad operators L_j
    std::mt19937 rng_;
    int tick_count_;
    Scalar sim_time_;                            // Accumulated simulation time (in seconds)
};

} // namespace hab
