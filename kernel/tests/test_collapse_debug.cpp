#include "brain/qw.hpp"
#include <iostream>
#include <iomanip>

using namespace hab;

int main() {
    QWConfig config;
    config.dimension = 7;
    config.entropy_cap = std::log(7.0);
    config.decoherence_rate = 1e-8;
    config.dt = 0.001;
    
    QuantumWorkspace qw(config);
    
    std::cout << "Initial state (uniform superposition):" << std::endl;
    std::cout << "  Entropy: " << std::fixed << std::setprecision(10) << qw.entropy() << std::endl;
    std::cout << "  Threshold: " << config.entropy_cap << std::endl;
    std::cout << std::endl;
    
    // Project from random GW state
    Eigen::VectorXd gw_state = Eigen::VectorXd::Random(60) * 10.0;
    qw.project_from_gw(gw_state);
    
    std::cout << "After project_from_gw:" << std::endl;
    std::cout << "  Entropy: " << qw.entropy() << std::endl;
    std::cout << std::endl;
    
    std::cout << "Evolution (first 20 steps):" << std::endl;
    for (int i = 0; i < 20; ++i) {
        qw.step_ticks(1);
        std::cout << "  Step " << std::setw(3) << i << ": entropy = " << qw.entropy();
        if (qw.has_collapsed()) {
            std::cout << " [COLLAPSED to quale " << qw.collapsed_quale() << "]";
            break;
        }
        std::cout << std::endl;
    }
    
    return 0;
}
