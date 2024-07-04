// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <algorithm>
#include <cmath>
#include "BinDisc.h"
#include <iostream>
#include <string>

namespace mdlp {

    BinDisc::BinDisc(int n_bins, strategy_t strategy) :
        Discretizer(), n_bins{ n_bins }, strategy{ strategy }
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
            cutPoints.push_back(0.0);
            cutPoints.push_back(0.0);
            return;
        }
        if (strategy == strategy_t::QUANTILE) {
            direction = bound_dir_t::RIGHT;
            fit_quantile(X);
        } else if (strategy == strategy_t::UNIFORM) {
            direction = bound_dir_t::RIGHT;
            fit_uniform(X);
        }
    }
    void BinDisc::fit(samples_t& X, labels_t& y)
    {
        fit(X);
    }
    std::vector<precision_t> linspace(precision_t start, precision_t end, int num)
    {
        if (start == end) {
            return { start, end };
        }
        precision_t delta = (end - start) / static_cast<precision_t>(num - 1);
        std::vector<precision_t> linspc;
        for (size_t i = 0; i < num; ++i) {
            precision_t val = start + delta * static_cast<precision_t>(i);
            linspc.push_back(val);
        }
        return linspc;
    }
    size_t clip(const size_t n, const size_t lower, const size_t upper)
    {
        return std::max(lower, std::min(n, upper));
    }
    std::vector<precision_t> percentile(samples_t& data, const std::vector<precision_t>& percentiles)
    {
        // Implementation taken from https://dpilger26.github.io/NumCpp/doxygen/html/percentile_8hpp_source.html
        std::vector<precision_t> results;
        bool first = true;
        results.reserve(percentiles.size());
        for (auto percentile : percentiles) {
            const auto i = static_cast<size_t>(std::floor(static_cast<double>(data.size() - 1) * percentile / 100.));
            const auto indexLower = clip(i, 0, data.size() - 2);
            const double percentI = static_cast<double>(indexLower) / static_cast<double>(data.size() - 1);
            const double fraction =
                (percentile / 100.0 - percentI) /
                (static_cast<double>(indexLower + 1) / static_cast<double>(data.size() - 1) - percentI);
            if (const auto value = data[indexLower] + (data[indexLower + 1] - data[indexLower]) * fraction; value != results.back() || first) // first needed as results.back() return is undefined for empty vectors
                results.push_back(value);
            first = false;
        }
        return results;
    }
    void BinDisc::fit_quantile(const samples_t& X)
    {
        auto quantiles = linspace(0.0, 100.0, n_bins + 1);
        auto data = X;
        std::sort(data.begin(), data.end());
        if (data.front() == data.back() || data.size() == 1) {
            // if X is constant, pass any two given points that shall be ignored in transform
            cutPoints.push_back(data.front());
            cutPoints.push_back(data.front());
            return;
        }
        cutPoints = percentile(data, quantiles);
    }
    void BinDisc::fit_uniform(const samples_t& X)
    {
        auto [vmin, vmax] = std::minmax_element(X.begin(), X.end());
        cutPoints = linspace(*vmin, *vmax, n_bins + 1);
    }
}