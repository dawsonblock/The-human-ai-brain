#include "brain/qw.hpp"
#include <iostream>

using namespace hab;

int main() {
    std::cout << "1. Creating QW on heap..." << std::endl;
    auto* qw = new QuantumWorkspace();
    
    std::cout << "2. Entropy: " << qw->entropy() << std::endl;
    
    std::cout << "3. Deleting QW..." << std::endl;
    delete qw;
    
    std::cout << "4. Test passed!" << std::endl;
    return 0;
}
