#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

typedef float precision_t;

std::vector<int> transform(const std::vector<float> cutPoints, const std::vector<float>& data)
{
    std::vector<int> discretizedData;
    discretizedData.reserve(data.size());
    for (const float& item : data) {
        auto upper = std::lower_bound(cutPoints.begin(), cutPoints.end(), item);
        discretizedData.push_back(upper - cutPoints.begin());
    }
    return discretizedData;
}
template <typename T>
void show_vector(const std::vector<T>& data, std::string title)
{
    std::cout << title << ": ";
    std::string sep = "";
    for (const auto& d : data) {
        std::cout << sep << d;
        sep = ", ";
    }
    std::cout << std::endl;
}
std::vector<precision_t> linspace(precision_t start, precision_t end, int num)
{
    if (start == end) {
        return { start, end };
    }
    precision_t delta = (end - start) / static_cast<precision_t>(num - 1);
    std::vector<precision_t> linspc;
    for (size_t i = 0; i < num - 1; ++i) {
        precision_t val = start + delta * static_cast<precision_t>(i);
        linspc.push_back(val);
    }
    return linspc;
}
size_t clip(const size_t n, size_t lower, size_t upper)
{
    return std::max(lower, std::min(n, upper));
}
std::vector<precision_t> percentile(std::vector<precision_t>& data, std::vector<precision_t>& percentiles)
{
    // Implementation taken from https://dpilger26.github.io/NumCpp/doxygen/html/percentile_8hpp_source.html
    std::vector<precision_t> results;
    results.reserve(percentiles.size());
    for (auto percentile : percentiles) {
        const size_t i = static_cast<size_t>(std::floor(static_cast<double>(data.size() - 1) * percentile / 100.));
        const auto indexLower = clip(i, 0, data.size() - 2);
        const double percentI = static_cast<double>(indexLower) / static_cast<double>(data.size() - 1);
        const double fraction =
            (percentile / 100.0 - percentI) /
            (static_cast<double>(indexLower + 1) / static_cast<double>(data.size() - 1) - percentI);
        const auto value = data[indexLower] + (data[indexLower + 1] - data[indexLower]) * fraction;
        if (value != results.back())
            results.push_back(value);
    }
    return results;
}
int main()
{
    // std::vector<float> test;
    // std::vector<float> cuts = { 0, 24.75, 49.5, 74.25, 10000 };
    // for (int i = 0; i < 100; ++i) {
    //     test.push_back(i);
    // }
    // auto Xt = transform(cuts, test);
    // show_vector(Xt, "Discretized data:");
    // std::vector<float> test2 = { 0,1,2,3,4,5,6,7,8,9,10,11 };
    // std::vector<float> cuts2 = { 0,1,2,3,4,5,6,7,8,9 };
    // auto Xt2 = transform(cuts2, test2);
    // show_vector(Xt2, "discretized data2: ");
    auto quantiles = linspace(0.0, 100.0, 3 + 1);
    std::vector<float> data = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
    std::vector<float> cutPoints;
    std::sort(data.begin(), data.end());
    cutPoints = percentile(data, quantiles);
    cutPoints.push_back(std::numeric_limits<precision_t>::max());
    data.push_back(15);
    data.push_back(0);
    cutPoints.pop_back();
    cutPoints.erase(cutPoints.begin());
    cutPoints.clear();
    cutPoints.push_back(9.0);
    auto Xt = transform(cutPoints, data);
    show_vector(data, "Original data");
    show_vector(Xt, "Discretized data");
    show_vector(cutPoints, "Cutpoints");
    return 0;
}
/*
n_bins = 3
data = [1,2,3,4,5,6,7,8,9,10]
quantiles = np.linspace(0, 100, n_bins + 1)
bin_edges = np.percentile(data, quantiles)

*/