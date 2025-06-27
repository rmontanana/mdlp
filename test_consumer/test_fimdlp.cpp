#include <iostream>
#include <vector>
#include <fimdlp/CPPFImdlp.h>
#include <fimdlp/BinDisc.h>

int main() {
    std::cout << "Testing FIMDLP package..." << std::endl;
    
    // Test data - simple continuous values with binary classification
    mdlp::samples_t data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    mdlp::labels_t labels = {0, 0, 0, 1, 1, 0, 1, 1, 1, 1};
    
    std::cout << "Created test data with " << data.size() << " samples" << std::endl;
    
    // Test MDLP discretizer
    mdlp::CPPFImdlp discretizer;
    discretizer.fit(data, labels);
    
    auto cut_points = discretizer.getCutPoints();
    std::cout << "MDLP found " << cut_points.size() << " cut points" << std::endl;
    
    for (size_t i = 0; i < cut_points.size(); ++i) {
        std::cout << "Cut point " << i << ": " << cut_points[i] << std::endl;
    }
    
    // Test BinDisc discretizer
    mdlp::BinDisc bin_discretizer(3, mdlp::strategy_t::UNIFORM);  // 3 bins, uniform strategy
    bin_discretizer.fit(data, labels);
    
    auto bin_cut_points = bin_discretizer.getCutPoints();
    std::cout << "BinDisc found " << bin_cut_points.size() << " cut points" << std::endl;
    
    for (size_t i = 0; i < bin_cut_points.size(); ++i) {
        std::cout << "Bin cut point " << i << ": " << bin_cut_points[i] << std::endl;
    }
    
    std::cout << "FIMDLP package test completed successfully!" << std::endl;
    return 0;
}