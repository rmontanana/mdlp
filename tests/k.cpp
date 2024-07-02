#include <iostream>
#include <vector>
#include <algorithm> // For std::lower_bound

std::vector<int> searchsorted(const std::vector<float>& cuts, const std::vector<float>& data) {
    std::vector<int> indices;
    indices.reserve(data.size());

    for (const float& value : data) {
        // Find the first position in 'a' where 'value' could be inserted to maintain order
        auto it = std::lower_bound(cuts.begin(), cuts.end(), value);
        // Calculate the index
        int index = it - cuts.begin();
        indices.push_back(index);
    }

    return indices;
}

int main() {
    std::vector<float> cuts = { 10.0 };
    std::vector<float> data = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0 };

    std::vector<int> result = searchsorted(cuts, data);

    for (int idx : result) {
        std::cout << idx << " ";
    }

    return 0;
}

