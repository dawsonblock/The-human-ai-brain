#include "brain/qw.hpp"
#include "brain/metrics.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace hab {

QuantumWorkspace::QuantumWorkspace(const QWConfig& config)
    : config_(config), rng_(config.rng_seed), tick_count_(0), sim_time_(0.0) {
    
    const int n = config_.dimension;
    
    // Initialize density matrix
    state_.rho = Eigen::MatrixXcd::Zero(n, n);
    state_.entropy = 0.0;
    state_.collapsed_quale = -1;
    state_.is_collapsed = false;
    state_.last_collapse = std::chrono::steady_clock::now();
    
    // Initialize Hamiltonian
    initialize_hamiltonian();
    
    // Initialize Lindblad operators
    initialize_lindblad_operators();
    
    // Start in uniform superposition
    reset();
}

void QuantumWorkspace::initialize_hamiltonian() {
    const int n = config_.dimension;
    H_ = Eigen::MatrixXcd::Zero(n, n);
    
    std::normal_distribution<Scalar> dist(0.0, 0.1);
    
    // Create Hermitian matrix
    for (int i = 0; i < n; ++i) {
        for (int j = i; j < n; ++j) {
            Scalar real_part = dist(rng_);
            Scalar imag_part = (i == j) ? 0.0 : dist(rng_);
            
            Complex val(real_part, imag_part);
            H_(i, j) = val;
            H_(j, i) = std::conj(val);
        }
    }
}

void QuantumWorkspace::initialize_lindblad_operators() {
    const int n = config_.dimension;
    const Scalar lambda = config_.decoherence_rate;
    
    lindblad_ops_.clear();
    
    // Create n Lindblad operators (one per dimension)
    for (int k = 0; k < n; ++k) {
        Eigen::MatrixXcd L = Eigen::MatrixXcd::Zero(n, n);
        
        // Gaussian localized decoherence
        const Scalar r_c = 1e-7;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                Scalar dist_sq = (i - j) * (i - j);
                Scalar gaussian = std::exp(-0.5 * dist_sq / (r_c * r_c));
                L(i, j) = Complex(std::sqrt(lambda) * gaussian, 0.0);
            }
        }
        
        lindblad_ops_.push_back(std::move(L));
    }
}

void QuantumWorkspace::step_ticks(int num_ticks) {
    for (int i = 0; i < num_ticks; ++i) {
        evolve_lindblad(config_.dt);
        enforce_hermiticity();
        enforce_trace_one();
        enforce_psd();
        
        state_.entropy = compute_von_neumann_entropy();
        sim_time_ += config_.dt;
        
        check_collapse();
        
        tick_count_++;
    }
}

void QuantumWorkspace::evolve_lindblad(Scalar dt) {
    const int n = config_.dimension;
    
    // Hamiltonian evolution: -i[H, rho]
    Eigen::MatrixXcd commutator = Complex(0.0, -1.0) * (H_ * state_.rho - state_.rho * H_);
    
    // Lindblad dissipation: sum_j (L_j rho L_j^dag - 0.5 {L_j^dag L_j, rho})
    Eigen::MatrixXcd lindblad_term = Eigen::MatrixXcd::Zero(n, n);
    
    for (const auto& L : lindblad_ops_) {
        Eigen::MatrixXcd L_dag = L.adjoint();
        Eigen::MatrixXcd L_dag_L = L_dag * L;
        
        lindblad_term += L * state_.rho * L_dag;
        lindblad_term -= 0.5 * (L_dag_L * state_.rho + state_.rho * L_dag_L);
    }
    
    // Update density matrix
    state_.rho += dt * (commutator + lindblad_term);
}

void QuantumWorkspace::check_collapse() {
    // Entropy-based collapse
    if (state_.entropy >= config_.entropy_cap) {
        perform_collapse();
        return;
    }
    
    // Time-based collapse (max dwell time in superposition)
    // Use simulation time, not wall clock time
    Scalar elapsed_sim_time_ms = (sim_time_ - state_.sim_time_at_last_collapse) * 1000.0;
    
    if (elapsed_sim_time_ms >= config_.max_dwell_ms) {
        perform_collapse();
    }
}

void QuantumWorkspace::perform_collapse() {
    const int n = config_.dimension;
    
    // Extract diagonal probabilities
    std::vector<Scalar> probs(n);
    Scalar sum = 0.0;
    
    for (int i = 0; i < n; ++i) {
        probs[i] = std::max(0.0, state_.rho(i, i).real());
        sum += probs[i];
    }
    
    // Normalize
    if (sum < 1e-10) sum = 1.0;
    for (auto& p : probs) p /= sum;
    
    // Sample collapsed state
    std::discrete_distribution<int> dist(probs.begin(), probs.end());
    state_.collapsed_quale = dist(rng_);
    
    // Project to pure state |i><i|
    state_.rho = Eigen::MatrixXcd::Zero(n, n);
    state_.rho(state_.collapsed_quale, state_.collapsed_quale) = Complex(1.0, 0.0);
    
    state_.entropy = 0.0;
    state_.is_collapsed = true;
    state_.last_collapse = std::chrono::steady_clock::now();
    state_.sim_time_at_last_collapse = sim_time_;
    
    // Note: Metrics collection handled at service layer
}

Scalar QuantumWorkspace::compute_von_neumann_entropy() const {
    // S(rho) = -Tr(rho log rho) = -sum_i lambda_i log(lambda_i)
    // Use proper eigenvalue decomposition for accurate entropy
    
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver(state_.rho);
    Eigen::VectorXd eigenvalues = solver.eigenvalues();
    
    Scalar entropy = 0.0;
    for (int i = 0; i < eigenvalues.size(); ++i) {
        Scalar lambda = std::max(config_.eigen_floor, eigenvalues(i));
        if (lambda > config_.eigen_floor) {
            entropy -= lambda * std::log(lambda);
        }
    }
    
    // Note: Metrics collection handled at service layer
    return entropy;
}

void QuantumWorkspace::enforce_hermiticity() {
    // Make rho Hermitian: rho = (rho + rho^dag) / 2
    state_.rho = (state_.rho + state_.rho.adjoint()) * 0.5;
}

void QuantumWorkspace::enforce_trace_one() {
    Scalar trace = state_.rho.trace().real();
    
    if (std::abs(trace - 1.0) > config_.trace_tol) {
        state_.rho /= trace;
    }
    
    // Note: Metrics collection handled at service layer
}

void QuantumWorkspace::enforce_psd() {
    // Project to nearest PSD matrix using eigendecomposition
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver(state_.rho);
    
    Eigen::VectorXd eigenvalues = solver.eigenvalues();
    Eigen::MatrixXcd eigenvectors = solver.eigenvectors();
    
    // Floor negative eigenvalues
    for (int i = 0; i < eigenvalues.size(); ++i) {
        if (eigenvalues(i) < config_.eigen_floor) {
            eigenvalues(i) = config_.eigen_floor;
        }
    }
    
    // Reconstruct with proper casting
    Eigen::MatrixXcd eigenvalues_complex = eigenvalues.cast<std::complex<Scalar>>();
    state_.rho = eigenvectors * eigenvalues_complex.asDiagonal() * eigenvectors.adjoint();
}

void QuantumWorkspace::project_from_gw(const Eigen::VectorXd& gw_state) {
    const int n = config_.dimension;
    const int gw_dim = gw_state.size();
    
    // Simple projection: map GW state to quantum amplitudes
    std::vector<Scalar> amplitudes(n, 0.0);
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < gw_dim; ++j) {
            amplitudes[i] += gw_state(j) * (i + 1.0) / (n * gw_dim);
        }
    }
    
    // Normalize to get probabilities
    Scalar sum = 0.0;
    for (auto& a : amplitudes) {
        a = std::abs(a);
        sum += a;
    }
    if (sum < 1e-10) sum = 1.0;
    for (auto& a : amplitudes) a /= sum;
    
    // Create mixed state (diagonal density matrix) instead of pure state
    // This better represents uncertainty from GW projection
    state_.rho = Eigen::MatrixXcd::Zero(n, n);
    for (int i = 0; i < n; ++i) {
        state_.rho(i, i) = Complex(amplitudes[i], 0.0);
    }
    
    state_.is_collapsed = false;
    state_.collapsed_quale = -1;
    // Don't reset collapse timer on projection - only on actual collapse or reset
    
    // Recompute entropy after projection
    state_.entropy = compute_von_neumann_entropy();
}

Eigen::VectorXd QuantumWorkspace::get_one_hot() const {
    Eigen::VectorXd one_hot = Eigen::VectorXd::Zero(config_.dimension);
    
    if (state_.is_collapsed && state_.collapsed_quale >= 0) {
        one_hot(state_.collapsed_quale) = 1.0;
    }
    
    return one_hot;
}

void QuantumWorkspace::reset() {
    const int n = config_.dimension;
    const Scalar val = 1.0 / n;
    
    // Uniform superposition
    state_.rho = Eigen::MatrixXcd::Constant(n, n, Complex(val, 0.0));
    // Set entropy slightly below cap to avoid immediate re-collapse
    state_.entropy = std::log(static_cast<Scalar>(n)) * 0.99;
    state_.is_collapsed = false;
    state_.collapsed_quale = -1;
    state_.last_collapse = std::chrono::steady_clock::now();
    state_.sim_time_at_last_collapse = sim_time_;
}

} // namespace hab
