#include <Eigen/Dense>
#include <vector>
#include <iostream>

using Scalar = double;
using Complex = std::complex<Scalar>;

int main() {
    std::cout << "1. Creating vector of Eigen matrices..." << std::endl;
    
    {
        std::vector<Eigen::MatrixXcd> matrices;
        
        std::cout << "2. Adding matrices to vector..." << std::endl;
        for (int i = 0; i < 7; ++i) {
            Eigen::MatrixXcd mat = Eigen::MatrixXcd::Zero(7, 7);
            matrices.push_back(mat);
        }
        
        std::cout << "3. Vector size: " << matrices.size() << std::endl;
        std::cout << "4. Going out of scope..." << std::endl;
    }
    
    std::cout << "5. Survived scope exit!" << std::endl;
    return 0;
}
