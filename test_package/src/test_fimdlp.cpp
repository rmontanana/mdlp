#include <iostream>
#include <vector>
#include <fimdlp/CPPFImdlp.h>
#include <fimdlp/Metrics.h>

int main() {
    std::cout << "Testing fimdlp library..." << std::endl;
    
    // Simple test of the library
    try {
        // Test Metrics class
        Metrics metrics;
        std::vector<int> labels = {0, 0, 1, 1, 0, 1};
        double entropy = metrics.entropy(labels);
        std::cout << "Entropy calculated: " << entropy << std::endl;
        
        // Test CPPFImdlp creation
        CPPFImdlp discretizer;
        std::cout << "CPPFImdlp instance created successfully" << std::endl;
        
        std::cout << "fimdlp library test completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error testing fimdlp library: " << e.what() << std::endl;
        return 1;
    }
}