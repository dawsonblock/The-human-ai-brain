#include <gtest/gtest.h>
#include "brain/qw.hpp"
#include <iostream>

using namespace hab;

TEST(MinimalTest, BasicConstruction) {
    std::cout << "Creating QWConfig..." << std::endl;
    QWConfig config;
    config.dimension = 7;
    
    std::cout << "Creating QuantumWorkspace..." << std::endl;
    QuantumWorkspace qw(config);
    
    std::cout << "Getting initial entropy..." << std::endl;
    Scalar entropy = qw.entropy();
    
    std::cout << "Initial entropy: " << entropy << std::endl;
    EXPECT_GT(entropy, 0.0);
    
    std::cout << "Test passed!" << std::endl;
}

TEST(MinimalTest, SingleStep) {
    std::cout << "Creating QW for single step test..." << std::endl;
    QWConfig config;
    QuantumWorkspace qw(config);
    
    std::cout << "Performing single step..." << std::endl;
    qw.step_ticks(1);
    
    std::cout << "Getting trace..." << std::endl;
    Complex trace = qw.rho().trace();
    
    std::cout << "Trace: " << trace << std::endl;
    EXPECT_NEAR(trace.real(), 1.0, 1e-5);
    
    std::cout << "Single step test passed!" << std::endl;
}

int main(int argc, char **argv) {
    std::cout << "Starting minimal tests..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
