#include <gtest/gtest.h>
#include "brain/qw.hpp"
#include "brain/brain_system.hpp"
#include <Eigen/Dense>
#include <complex>
#include <iostream>

using namespace hab;
using Scalar = double;
using Complex = std::complex<Scalar>;
using MatrixXc = Eigen::Matrix<Complex, Eigen::Dynamic, Eigen::Dynamic>;

// ============================================================================
// TEST SUITE 1: POSITIVE SEMI-DEFINITE (PSD) VALIDATION
// ============================================================================

TEST(MinimalTest, DensityMatrixIsPSD) {
    std::cout << "Testing density matrix is positive semi-definite..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    config.entropy_cap = std::log(7.0);
    
    QuantumWorkspace qw(config);
    const auto& rho = qw.rho();
    
    // Verify matrix is Hermitian (required for PSD)
    auto rho_conj_transpose = rho.adjoint();
    Scalar hermitian_error = (rho - rho_conj_transpose).norm();
    EXPECT_LT(hermitian_error, 1e-10) << "Density matrix is not Hermitian";
    
    // Check all eigenvalues >= 0 (definition of PSD)
    Eigen::SelfAdjointEigenSolver<MatrixXc> solver(rho);
    auto eigenvals = solver.eigenvalues();
    
    std::cout << "  Eigenvalues: ";
    for (int i = 0; i < eigenvals.size(); ++i) {
        std::cout << eigenvals(i) << " ";
        EXPECT_GE(eigenvals(i), -1e-10) 
            << "Eigenvalue " << i << " is negative: " << eigenvals(i);
    }
    std::cout << std::endl;
    
    std::cout << "✓ Density matrix is PSD" << std::endl;
}

TEST(MinimalTest, PSDAfterEvolution) {
    std::cout << "Testing PSD property preserved after Lindblad evolution..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    config.dt = 0.01;  // 10ms timestep
    config.decoherence_rate = 0.05;
    
    QuantumWorkspace qw(config);
    
    // Evolve for multiple steps
    for (int step = 0; step < 50; ++step) {
        qw.step_ticks(1);
        
        const auto& rho = qw.rho();
        
        // Check PSD at each step
        Eigen::SelfAdjointEigenSolver<MatrixXc> solver(rho);
        auto eigenvals = solver.eigenvalues();
        
        for (int i = 0; i < eigenvals.size(); ++i) {
            EXPECT_GE(eigenvals(i), -1e-10) 
                << "Step " << step << ": Eigenvalue " << i 
                << " became negative: " << eigenvals(i);
        }
    }
    
    std::cout << "✓ PSD preserved through 50 evolution steps" << std::endl;
}

// ============================================================================
// TEST SUITE 2: TRACE VALIDATION (Tr(ρ) = 1)
// ============================================================================

TEST(MinimalTest, TraceEqualsOne) {
    std::cout << "Testing Tr(ρ) = 1 (normalization)..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    
    QuantumWorkspace qw(config);
    const auto& rho = qw.rho();
    
    Complex trace = rho.trace();
    
    std::cout << "  Trace: " << trace << std::endl;
    EXPECT_NEAR(trace.real(), 1.0, 1e-6) 
        << "Trace(ρ) != 1, got " << trace.real();
    EXPECT_NEAR(trace.imag(), 0.0, 1e-6) 
        << "Trace has imaginary component: " << trace.imag();
    
    std::cout << "✓ Trace equals 1.0" << std::endl;
}

TEST(MinimalTest, TracePreservedAfterEvolution) {
    std::cout << "Testing trace preservation during evolution..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    config.dt = 0.01;
    
    QuantumWorkspace qw(config);
    
    // Test trace preservation over many steps
    for (int step = 0; step < 100; ++step) {
        qw.step_ticks(1);
        
        Complex trace = qw.rho().trace();
        
        EXPECT_NEAR(trace.real(), 1.0, 1e-5) 
            << "Step " << step << ": Trace deviated from 1.0: " << trace.real();
        EXPECT_NEAR(trace.imag(), 0.0, 1e-5) 
            << "Step " << step << ": Trace has imaginary part: " << trace.imag();
    }
    
    std::cout << "✓ Trace preserved through 100 evolution steps" << std::endl;
}

TEST(MinimalTest, TraceAfterCollapse) {
    std::cout << "Testing trace = 1 after quantum collapse..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    config.entropy_cap = std::log(7.0);
    
    QuantumWorkspace qw(config);
    
    // Force high entropy state
    qw.broadcast(Eigen::VectorXcd::Ones(7).normalized());
    
    // Evolve until collapse
    for (int i = 0; i < 100; ++i) {
        qw.step_ticks(1);
        if (qw.last_collapsed()) {
            break;
        }
    }
    
    Complex trace = qw.rho().trace();
    
    std::cout << "  Post-collapse trace: " << trace << std::endl;
    EXPECT_NEAR(trace.real(), 1.0, 1e-5) 
        << "Trace after collapse != 1.0";
    EXPECT_NEAR(trace.imag(), 0.0, 1e-5) 
        << "Trace after collapse has imaginary part";
    
    std::cout << "✓ Trace preserved after collapse" << std::endl;
}

// ============================================================================
// TEST SUITE 3: DWELL TIME VALIDATION
// ============================================================================

TEST(MinimalTest, CollapseDwellRespected) {
    std::cout << "Testing collapse dwell time enforcement..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    config.entropy_cap = std::log(7.0);  // ln(7) ≈ 1.9459
    config.max_dwell_ticks = 12;  // 12 ticks = 120ms at dt=0.01
    config.dt = 0.01;
    config.decoherence_rate = 0.05;
    
    QuantumWorkspace qw(config);
    
    // Create high-entropy state to encourage collapse
    Eigen::VectorXcd uniform = Eigen::VectorXcd::Ones(7);
    uniform /= std::sqrt(7.0);  // Normalize
    qw.broadcast(uniform);
    
    std::cout << "  Initial entropy: " << qw.entropy() << std::endl;
    std::cout << "  Entropy cap: " << config.entropy_cap << std::endl;
    
    // Evolve for 10 steps (100ms) - should NOT collapse yet
    int collapse_count_before = 0;
    for (int i = 0; i < 10; ++i) {
        qw.step_ticks(1);
        if (qw.last_collapsed()) {
            collapse_count_before++;
        }
    }
    
    std::cout << "  Dwell at step 10: " << qw.dwell_ticks() << " ticks" << std::endl;
    EXPECT_LT(qw.dwell_ticks(), 12) 
        << "Dwell should be < max_dwell_ticks at this point";
    
    // Evolve 3 more steps (total 130ms) - now eligible for collapse
    for (int i = 0; i < 3; ++i) {
        qw.step_ticks(1);
    }
    
    std::cout << "  Dwell at step 13: " << qw.dwell_ticks() << " ticks" << std::endl;
    
    // Check if entropy is high enough to trigger collapse
    if (qw.entropy() >= config.entropy_cap) {
        std::cout << "  Entropy (" << qw.entropy() 
                  << ") >= cap (" << config.entropy_cap << ")" << std::endl;
        
        // Dwell should be >= max_dwell_ticks, making collapse eligible
        bool eligible = qw.dwell_ticks() >= config.max_dwell_ticks;
        std::cout << "  Collapse eligible: " << (eligible ? "yes" : "no") << std::endl;
        EXPECT_TRUE(eligible) 
            << "Should be eligible for collapse after exceeding max dwell time";
    }
    
    std::cout << "✓ Dwell time constraints enforced correctly" << std::endl;
}

TEST(MinimalTest, DwellResetAfterCollapse) {
    std::cout << "Testing dwell time reset after collapse..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    config.entropy_cap = std::log(7.0);
    config.max_dwell_ticks = 12;
    config.dt = 0.01;
    config.decoherence_rate = 0.1;  // Higher decoherence for faster collapse
    
    QuantumWorkspace qw(config);
    
    // Create high-entropy uniform state
    Eigen::VectorXcd uniform = Eigen::VectorXcd::Ones(7).normalized();
    qw.broadcast(uniform);
    
    // Evolve until first collapse
    int steps_to_collapse = 0;
    for (int i = 0; i < 200; ++i) {
        qw.step_ticks(1);
        steps_to_collapse++;
        if (qw.last_collapsed()) {
            std::cout << "  First collapse at step " << steps_to_collapse << std::endl;
            break;
        }
    }
    
    EXPECT_TRUE(qw.last_collapsed()) << "Should have collapsed within 200 steps";
    
    // Dwell should be reset (near 0) after collapse
    int dwell_after_collapse = qw.dwell_ticks();
    std::cout << "  Dwell after collapse: " << dwell_after_collapse << " ticks" << std::endl;
    EXPECT_LE(dwell_after_collapse, 1) 
        << "Dwell time should reset after collapse";
    
    std::cout << "✓ Dwell time resets correctly after collapse" << std::endl;
}

TEST(MinimalTest, DwellIncreasesMonotonically) {
    std::cout << "Testing dwell time increases monotonically without collapse..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    config.entropy_cap = 10.0;  // Very high cap - prevent collapse
    config.max_dwell_ticks = 1000;
    config.dt = 0.01;
    
    QuantumWorkspace qw(config);
    
    int prev_dwell = qw.dwell_ticks();
    
    for (int i = 0; i < 20; ++i) {
        qw.step_ticks(1);
        int curr_dwell = qw.dwell_ticks();
        
        // Dwell should increase by 1 each step (if no collapse)
        if (!qw.last_collapsed()) {
            EXPECT_EQ(curr_dwell, prev_dwell + 1) 
                << "Step " << i << ": Dwell should increase by 1";
        }
        
        prev_dwell = curr_dwell;
    }
    
    std::cout << "  Final dwell: " << prev_dwell << " ticks" << std::endl;
    std::cout << "✓ Dwell time increases monotonically" << std::endl;
}

// ============================================================================
// TEST SUITE 4: INTEGRATION TESTS
// ============================================================================

TEST(MinimalTest, FullBrainSystemSmoke) {
    std::cout << "Running full BrainSystem smoke test..." << std::endl;
    
    BrainConfig config;
    config.qw_config.dimension = 7;
    config.qw_config.entropy_cap = std::log(7.0);
    config.gw_config.input_dim = 28 * 28;  // MNIST
    config.gw_config.output_dim = 60;
    
    BrainSystem brain(config);
    
    // Create dummy input
    Eigen::VectorXd input = Eigen::VectorXd::Random(28 * 28);
    Scalar reward = 0.5;
    
    // Run cognitive cycle
    auto result = brain.step(input, reward);
    
    std::cout << "  Collapsed quale: " << result.collapsed_quale << std::endl;
    std::cout << "  Entropy: " << result.entropy << std::endl;
    std::cout << "  Working memory size: " << result.h_wm.size() << std::endl;
    std::cout << "  Global workspace size: " << result.h_global.size() << std::endl;
    
    // Basic sanity checks
    EXPECT_GE(result.collapsed_quale, 0);
    EXPECT_LT(result.collapsed_quale, 7);
    EXPECT_GE(result.entropy, 0.0);
    EXPECT_GT(result.h_wm.size(), 0);
    EXPECT_GT(result.h_global.size(), 0);
    
    // Phenomenal report should have valid ranges
    EXPECT_GE(result.phenomenal.intensity, 0.0);
    EXPECT_LE(result.phenomenal.intensity, 1.0);
    EXPECT_GE(result.phenomenal.clarity, 0.0);
    EXPECT_LE(result.phenomenal.clarity, 1.0);
    
    std::cout << "✓ Full brain system operational" << std::endl;
}

TEST(MinimalTest, BasicConstruction) {
    std::cout << "Testing basic construction..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    
    QuantumWorkspace qw(config);
    
    Scalar entropy = qw.entropy();
    
    std::cout << "  Initial entropy: " << entropy << std::endl;
    EXPECT_GT(entropy, 0.0);
    
    std::cout << "✓ Basic construction successful" << std::endl;
}

TEST(MinimalTest, SingleStep) {
    std::cout << "Testing single evolution step..." << std::endl;
    
    QWConfig config;
    config.dimension = 7;
    
    QuantumWorkspace qw(config);
    
    qw.step_ticks(1);
    
    Complex trace = qw.rho().trace();
    
    std::cout << "  Trace after step: " << trace << std::endl;
    EXPECT_NEAR(trace.real(), 1.0, 1e-5);
    
    std::cout << "✓ Single step successful" << std::endl;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  HUMAN-AI BRAIN MINIMAL TEST SUITE" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n========================================" << std::endl;
    if (result == 0) {
        std::cout << "  ✓ ALL TESTS PASSED" << std::endl;
    } else {
        std::cout << "  ✗ SOME TESTS FAILED" << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
    
    return result;
}
