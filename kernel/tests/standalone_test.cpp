#include "brain/qw.hpp"
#include <iostream>
#include <cassert>

using namespace hab;

int main() {
    std::cout << "=== Standalone QW Test ===" << std::endl;
    
    try {
        std::cout << "1. Creating QWConfig..." << std::endl;
        QWConfig config;
        config.dimension = 7;
        
        std::cout << "2. Creating QuantumWorkspace..." << std::endl;
        {
            QuantumWorkspace qw(config);
            
            std::cout << "3. Getting initial entropy..." << std::endl;
            Scalar entropy = qw.entropy();
            std::cout << "   Entropy: " << entropy << std::endl;
            assert(entropy > 0.0);
            
            std::cout << "4. Performing 10 evolution steps..." << std::endl;
            qw.step_ticks(10);
            
            std::cout << "5. Getting trace..." << std::endl;
            Complex trace = qw.rho().trace();
            std::cout << "   Trace: " << trace << std::endl;
            assert(std::abs(trace.real() - 1.0) < 1e-5);
            
            std::cout << "6. Test passed! QW going out of scope..." << std::endl;
        }
        
        std::cout << "7. QW destroyed successfully!" << std::endl;
        std::cout << "=== All tests passed ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
