#include <iostream>
#include <Eigen/Dense>

int main() {
    std::cout << "Testing Eigen..." << std::endl;
    
    Eigen::MatrixXcd m(3, 3);
    m = Eigen::MatrixXcd::Identity(3, 3);
    
    std::cout << "Matrix:\n" << m << std::endl;
    std::cout << "Trace: " << m.trace() << std::endl;
    
    std::cout << "Basic test passed!" << std::endl;
    return 0;
}
