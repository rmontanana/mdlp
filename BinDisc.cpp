#include <algorithm>
#include <limits>
#include <cmath>
#include "BinDisc.h"
#include <iostream>
#include <string>

namespace mdlp {

    BinDisc::BinDisc(int n_bins, strategy_t strategy) : n_bins{ n_bins }, strategy{ strategy }, Discretizer()
    {
        if (n_bins < 3) {
            throw std::invalid_argument("n_bins must be greater than 2");
        }
    }
    BinDisc::~BinDisc() = default;
    void BinDisc::fit(samples_t& X)
    {
        // y is included for compatibility with the Discretizer interface
        cutPoints.clear();
        if (X.empty()) {
            cutPoints.push_back(std::numeric_limits<precision_t>::max());
            return;
        }
        if (strategy == strategy_t::QUANTILE) {
            fit_quantile(X);
        } else if (strategy == strategy_t::UNIFORM) {
            fit_uniform(X);
        }
    }
    void BinDisc::fit(samples_t& X, labels_t& y)
    {
        fit(X);
    }
    std::vector<precision_t> linspace(precision_t start, precision_t end, int num)
    {
        // Doesn't include end point as it is not needed
        if (start == end) {
            return { 0 };
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
    std::vector<precision_t> percentile(samples_t& data, std::vector<precision_t>& percentiles)
    {
        // Implementation taken from https://dpilger26.github.io/NumCpp/doxygen/html/percentile_8hpp_source.html
        std::vector<precision_t> results;
        results.reserve(percentiles.size());
        for (auto percentile : percentiles) {
            const size_t i = static_cast<size_t>(std::floor(static_cast<double>(data.size() - 1) * percentile / 100.));
            const auto indexLower = clip(i, 0, data.size() - 1);
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
    void BinDisc::fit_quantile(samples_t& X)
    {
        auto quantiles = linspace(0.0, 100.0, n_bins + 1);
        auto data = X;
        std::sort(data.begin(), data.end());
        if (data.front() == data.back() || data.size() == 1) {
            // if X is constant
            cutPoints.push_back(std::numeric_limits<precision_t>::max());
            return;
        }
        cutPoints = percentile(data, quantiles);
        normalizeCutPoints();
    }
    void BinDisc::fit_uniform(samples_t& X)
    {

        auto minmax = std::minmax_element(X.begin(), X.end());
        cutPoints = linspace(*minmax.first, *minmax.second, n_bins + 1);
        normalizeCutPoints();
    }
    void BinDisc::normalizeCutPoints()
    {
        // Add max value to the end
        cutPoints.push_back(std::numeric_limits<precision_t>::max());
        // Remove first as it is not needed
        cutPoints.erase(cutPoints.begin());
    }
}