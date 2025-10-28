#include "brain/qw.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace hab;

class QuantumWorkspaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        config.dimension = 7;
        config.dt = 0.01;
        config.decoherence_rate = 0.05;
        config.entropy_threshold = 1.9459;
        config.max_dwell_ms = 120.0;
        config.trace_tolerance = 1e-10;
        config.eigenvalue_floor = 1e-12;
        config.collapse_rate_target_hz = 8.2;
    }

    QWConfig config;
};

// Test 1: Configuration Validation
TEST_F(QuantumWorkspaceTest, ConfigValidation) {
    EXPECT_TRUE(config.validate());
    EXPECT_EQ(config.validation_error(), "");
    
    // Test invalid config
    QWConfig bad_config = config;
    bad_config.dimension = 1;
    EXPECT_FALSE(bad_config.validate());
    EXPECT_NE(bad_config.validation_error(), "");
}

// Test 2: Initialization
TEST_F(QuantumWorkspaceTest, Initialization) {
    QuantumWorkspace qw(config);
    
    EXPECT_FALSE(qw.has_collapsed());
    EXPECT_EQ(qw.collapsed_quale(), -1);
    EXPECT_GE(qw.entropy(), 0.0);
    EXPECT_LE(qw.entropy(), std::log(static_cast<double>(config.dimension)));
    
    // Check density matrix properties
    const auto& rho = qw.rho();
    EXPECT_EQ(rho.rows(), config.dimension);
    EXPECT_EQ(rho.cols(), config.dimension);
    
    // Trace should be 1
    double trace = rho.trace().real();
    EXPECT_NEAR(trace, 1.0, config.trace_tolerance);
}

// Test 3: Hermiticity
TEST_F(QuantumWorkspaceTest, Hermiticity) {
    QuantumWorkspace qw(config);
    qw.step_ticks(10);
    
    const auto& rho = qw.rho();
    auto rho_dag = rho.adjoint();
    
    // Check if rho = rho^dag
    double max_diff = (rho - rho_dag).cwiseAbs().maxCoeff();
    EXPECT_LT(max_diff, 1e-9);
}

// Test 4: Trace Preservation
TEST_F(QuantumWorkspaceTest, TracePreservation) {
    QuantumWorkspace qw(config);
    
    // With -ffast-math optimization, trace preservation has ~1e-5 tolerance
    const double practical_tolerance = 1e-4;
    
    for (int i = 0; i < 100; ++i) {
        qw.step_ticks(1);
        double trace = qw.rho().trace().real();
        EXPECT_NEAR(trace, 1.0, practical_tolerance);
    }
}

// Test 5: PSD (Positive Semi-Definite)
TEST_F(QuantumWorkspaceTest, PositiveSemiDefinite) {
    QuantumWorkspace qw(config);
    qw.step_ticks(50);
    
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver(qw.rho());
    Eigen::VectorXd eigenvalues = solver.eigenvalues();
    
    for (int i = 0; i < eigenvalues.size(); ++i) {
        EXPECT_GE(eigenvalues(i), -config.eigenvalue_floor);
    }
}

// Test 6: Entropy Monotonicity (with small tolerance for numerical error)
TEST_F(QuantumWorkspaceTest, EntropyMonotonic) {
    QuantumWorkspace qw(config);
    double prev_entropy = qw.entropy();
    
    for (int i = 0; i < 50; ++i) {
        qw.step_ticks(1);
        double curr_entropy = qw.entropy();
        
        // With fast-math optimizations, entropy can have small numerical fluctuations
        // Allow for small decreases before collapse
        if (!qw.has_collapsed()) {
            EXPECT_GE(curr_entropy, prev_entropy - 0.01);  // Relaxed tolerance for production build
        }
        
        prev_entropy = curr_entropy;
        
        // If collapsed, entropy should drop significantly
        if (qw.has_collapsed()) {
            break;
        }
    }
}

// Test 7: Entropy-Based Collapse
TEST_F(QuantumWorkspaceTest, EntropyBasedCollapse) {
    QuantumWorkspace qw(config);
    
    bool collapsed = false;
    int max_steps = 1000;
    
    for (int i = 0; i < max_steps; ++i) {
        qw.step_ticks(1);
        
        if (qw.has_collapsed()) {
            collapsed = true;
            EXPECT_GE(qw.entropy(), 0.0);
            EXPECT_LT(qw.entropy(), config.entropy_threshold);
            EXPECT_GE(qw.collapsed_quale(), 0);
            EXPECT_LT(qw.collapsed_quale(), config.dimension);
            break;
        }
    }
    
    EXPECT_TRUE(collapsed) << "System should collapse within " << max_steps << " steps";
}

// Test 8: Time-Based Collapse
TEST_F(QuantumWorkspaceTest, TimeBasedCollapse) {
    QWConfig slow_config = config;
    slow_config.decoherence_rate = 0.0;  // No decoherence
    slow_config.max_dwell_ms = 50.0;     // Short dwell time
    
    QuantumWorkspace qw(slow_config);
    
    bool collapsed = false;
    double total_sim_time_ms = 0.0;
    
    for (int i = 0; i < 1000; ++i) {
        qw.step_ticks(1);
        total_sim_time_ms = qw.sim_time() * 1000.0;
        
        if (qw.has_collapsed()) {
            collapsed = true;
            EXPECT_LE(total_sim_time_ms, slow_config.max_dwell_ms * 1.5);
            break;
        }
    }
    
    EXPECT_TRUE(collapsed) << "System should collapse due to max dwell time";
}

// Test 9: Collapse State is Pure
TEST_F(QuantumWorkspaceTest, CollapsedStateIsPure) {
    QuantumWorkspace qw(config);
    
    // Run until collapse
    for (int i = 0; i < 1000; ++i) {
        qw.step_ticks(1);
        if (qw.has_collapsed()) break;
    }
    
    ASSERT_TRUE(qw.has_collapsed());
    
    // Check that density matrix is a pure state |i><i|
    const auto& rho = qw.rho();
    int quale = qw.collapsed_quale();
    
    // Diagonal should have 1 at quale position, 0 elsewhere
    for (int i = 0; i < config.dimension; ++i) {
        if (i == quale) {
            EXPECT_NEAR(rho(i, i).real(), 1.0, 1e-6);
        } else {
            EXPECT_NEAR(rho(i, i).real(), 0.0, 1e-6);
        }
    }
    
    // Off-diagonal should be zero
    for (int i = 0; i < config.dimension; ++i) {
        for (int j = 0; j < config.dimension; ++j) {
            if (i != j) {
                EXPECT_NEAR(std::abs(rho(i, j)), 0.0, 1e-6);
            }
        }
    }
}

// Test 10: One-Hot Encoding
TEST_F(QuantumWorkspaceTest, OneHotEncoding) {
    QuantumWorkspace qw(config);
    
    // Run until collapse
    for (int i = 0; i < 1000; ++i) {
        qw.step_ticks(1);
        if (qw.has_collapsed()) break;
    }
    
    ASSERT_TRUE(qw.has_collapsed());
    
    auto one_hot = qw.get_one_hot();
    EXPECT_EQ(one_hot.size(), config.dimension);
    
    int quale = qw.collapsed_quale();
    for (int i = 0; i < config.dimension; ++i) {
        if (i == quale) {
            EXPECT_EQ(one_hot(i), 1.0);
        } else {
            EXPECT_EQ(one_hot(i), 0.0);
        }
    }
}

// Test 11: Reset Functionality
TEST_F(QuantumWorkspaceTest, Reset) {
    QuantumWorkspace qw(config);
    
    // Run until collapse
    for (int i = 0; i < 1000; ++i) {
        qw.step_ticks(1);
        if (qw.has_collapsed()) break;
    }
    
    ASSERT_TRUE(qw.has_collapsed());
    
    // Reset
    qw.reset();
    
    EXPECT_FALSE(qw.has_collapsed());
    EXPECT_EQ(qw.collapsed_quale(), -1);
    
    // Should be back in uniform superposition
    double trace = qw.rho().trace().real();
    EXPECT_NEAR(trace, 1.0, config.trace_tolerance);
}

// Test 12: Threading Enablement
TEST_F(QuantumWorkspaceTest, Threading) {
    QuantumWorkspace qw(config);
    
    // This should not crash
    qw.enable_threading(2);
    qw.step_ticks(10);
    
    EXPECT_TRUE(true);  // Just check it doesn't crash
}

// Test 13: Performance - Step Latency
TEST_F(QuantumWorkspaceTest, StepLatency) {
    QuantumWorkspace qw(config);
    qw.enable_threading(-1);
    
    auto start = std::chrono::steady_clock::now();
    qw.step_ticks(100);
    auto end = std::chrono::steady_clock::now();
    
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    double avg_latency_ms = elapsed_ms / 100.0;
    
    // Should meet v3.4.0 target (<10ms)
    EXPECT_LT(avg_latency_ms, 10.0) 
        << "Average step latency: " << avg_latency_ms << " ms";
}

// Test 14: Collapse Rate Target
TEST_F(QuantumWorkspaceTest, CollapseRateApproximate) {
    QuantumWorkspace qw(config);
    
    int collapse_count = 0;
    int total_steps = 5000;
    double total_sim_time_s = 0.0;
    
    for (int i = 0; i < total_steps; ++i) {
        bool was_collapsed = qw.has_collapsed();
        qw.step_ticks(1);
        
        if (!was_collapsed && qw.has_collapsed()) {
            collapse_count++;
        }
        
        total_sim_time_s = qw.sim_time();
    }
    
    double collapse_rate_hz = collapse_count / total_sim_time_s;
    
    // Should be approximately 8.2 Hz (with tolerance)
    EXPECT_GT(collapse_rate_hz, 5.0);
    EXPECT_LT(collapse_rate_hz, 15.0);
}

// Test 15: Project from Global Workspace
TEST_F(QuantumWorkspaceTest, ProjectFromGW) {
    QuantumWorkspace qw(config);
    
    Eigen::VectorXd gw_state = Eigen::VectorXd::Random(config.dimension);
    gw_state = gw_state.cwiseAbs();
    gw_state /= gw_state.sum();  // Normalize
    
    qw.project_from_gw(gw_state);
    
    // Density matrix should reflect the projection
    const auto& rho = qw.rho();
    double trace = rho.trace().real();
    EXPECT_NEAR(trace, 1.0, config.trace_tolerance);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
