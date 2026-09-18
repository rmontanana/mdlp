// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include "BinDisc.h"
#include "Exceptions.h"

namespace mdlp {

    // Both constructors funnel through the config one, so validation lives in a
    // single place (BinDiscConfig::validate) instead of being duplicated here.
    BinDisc::BinDisc(int n_bins, strategy_t strategy) :
        BinDisc(BinDiscConfig{}.withNBins(n_bins).withStrategy(strategy))
    {
    }

    BinDisc::BinDisc(const BinDiscConfig& config) :
        Discretizer(), n_bins{ config.n_bins }, strategy{ config.strategy }
    {
        config.validate();
    }

    labels_t BinDisc::discretize(const samples_t& X, const labels_t& y, const BinDiscConfig& config)
    {
        BinDisc disc(config);
        samples_t X_copy = X;
        labels_t y_copy = y;
        disc.fit(std::move(X_copy), std::move(y_copy));
        labels_t out;
        disc.transform(X, out);
        return out;
    }
    BinDisc::~BinDisc() = default;
    void BinDisc::validate_input(const samples_t& X) const
    {
        if (X.empty()) {
            throw ValidationError("Input data X cannot be empty");
        }
        if (X.size() < static_cast<size_t>(n_bins)) {
            throw ValidationError("Input data size (" + std::to_string(X.size()) + ") must be at least n_bins (" + std::to_string(n_bins) + ")");
        }
        // QUANTILE sorts, and UNIFORM feeds min/max into linspace; neither
        // tolerates a non-finite sample.
        validate_finite(X);
    }
    void BinDisc::fit(samples_t& X)
    {
        validate_input(X);
        cutPoints.clear();
        direction = bound_dir_t::RIGHT;
        if (strategy == strategy_t::QUANTILE) {
            fit_quantile(X);  // copies into fit_quantile's by-value parameter
        } else if (strategy == strategy_t::UNIFORM) {
            fit_uniform(X);
        }
    }
    void BinDisc::fit(samples_t&& X)
    {
        validate_input(X);
        cutPoints.clear();
        direction = bound_dir_t::RIGHT;
        if (strategy == strategy_t::QUANTILE) {
            fit_quantile(std::move(X));  // adopts the caller's buffer and sorts it
        } else if (strategy == strategy_t::UNIFORM) {
            fit_uniform(X);  // reads only; nothing to adopt
        }
    }
    // y is accepted and ignored on purpose: every discretizer takes fit(X, y) so
    // an experimentation platform can drive them all through one code path. The
    // attribute says "unused deliberately" rather than silencing the warning.
    void BinDisc::fit(samples_t& X, [[maybe_unused]] labels_t& y)
    {
        if (X.empty()) {
            throw ValidationError("X cannot be empty");
        }
        fit(X);
    }
    void BinDisc::fit(samples_t&& X, [[maybe_unused]] labels_t&& y)
    {
        if (X.empty()) {
            throw ValidationError("X cannot be empty");
        }
        fit(std::move(X));
    }
    std::vector<precision_t> BinDisc::linspace(precision_t start, precision_t end, int num)
    {
        // Input validation
        if (num < 2) {
            throw InvalidParameter("linspace: num must be at least 2, got " + std::to_string(num));
        }
        if (std::isnan(start) || std::isnan(end)) {
            throw InvalidParameter("Start and end values cannot be NaN");
        }
        if (std::isinf(start) || std::isinf(end)) {
            throw InvalidParameter("Start and end values cannot be infinite");
        }

        if (start == end) {
            return { start, end };
        }
        precision_t delta = (end - start) / static_cast<precision_t>(num - 1);
        std::vector<precision_t> linspc;
        for (size_t i = 0; i < static_cast<size_t>(num); ++i) {
            precision_t val = start + delta * static_cast<precision_t>(i);
            linspc.push_back(val);
        }
        return linspc;
    }
    size_t clip(const size_t n, const size_t lower, const size_t upper)
    {
        return std::max(lower, std::min(n, upper));
    }
    std::vector<precision_t> BinDisc::percentile(samples_t& data, const std::vector<precision_t>& percentiles)
    {
        // Input validation
        if (data.empty()) {
            throw ValidationError("Data cannot be empty for percentile calculation");
        }
        if (percentiles.empty()) {
            throw ValidationError("Percentiles cannot be empty");
        }
        // Implementation taken from https://dpilger26.github.io/NumCpp/doxygen/html/percentile_8hpp_source.html
        // One value per requested percentile, repeats included: fit_quantile
        // needs to see which percentiles landed on the same value.
        std::vector<precision_t> results;
        results.reserve(percentiles.size());
        for (auto percentile : percentiles) {
            const auto i = static_cast<size_t>(std::floor(static_cast<precision_t>(data.size() - 1) * percentile / 100.));
            const auto indexLower = clip(i, 0, data.size() - 2);
            const precision_t percentI = static_cast<precision_t>(indexLower) / static_cast<precision_t>(data.size() - 1);
            const precision_t fraction =
                (percentile / 100.0 - percentI) /
                (static_cast<precision_t>(indexLower + 1) / static_cast<precision_t>(data.size() - 1) - percentI);
            results.push_back(data[indexLower] + (data[indexLower + 1] - data[indexLower]) * fraction);
        }
        return results;
    }
    // transform() semantics for a fitted QUANTILE discretizer: the bin of x is
    // the number of interior cut points <= x (bound_dir_t::RIGHT, upper_bound).
    size_t BinDisc::bin_of(precision_t x) const
    {
        return static_cast<size_t>(std::upper_bound(cutPoints.begin() + 1, cutPoints.end() - 1, x) - (cutPoints.begin() + 1));
    }
    // Adds cut to the interior of cutPoints, keeping it sorted. cut may equal
    // the max sentinel (the last element): it then sits right before it, and
    // transform() treats it as a real cut point, unlike the sentinel.
    void BinDisc::insert_cut(precision_t cut)
    {
        cutPoints.insert(std::upper_bound(cutPoints.begin() + 1, cutPoints.end() - 1, cut), cut);
    }
    // A mass point is a value that occupies at least one full quantile width,
    // so that two or more requested edges landed on it and were collapsed into
    // one. Left alone, that value is indistinguishable from its neighbours: a
    // binary feature yields [min, max] and every sample maps onto bin 0. This
    // guarantees that the mass point gets a bin of its own, on both sides, by
    // adding at most two cut points: the value itself, which separates it from
    // everything below (x >= cut goes right), and the next distinct value in
    // the data, which separates it from everything above. Neither is added when
    // an existing edge already does the job, so no empty bins appear and the
    // edges of data without mass points are untouched.
    void BinDisc::separate_mass_point(const samples_t& sorted, precision_t value)
    {
        const auto lower = std::lower_bound(sorted.begin(), sorted.end(), value);
        const auto upper = std::upper_bound(sorted.begin(), sorted.end(), value);
        if (lower != sorted.begin() && bin_of(*(lower - 1)) == bin_of(value)) {
            insert_cut(value);
        }
        if (upper != sorted.end() && bin_of(value) == bin_of(*upper)) {
            insert_cut(*upper);
        }
    }
    void BinDisc::fit_quantile(samples_t data)
    {
        auto quantiles = linspace(0.0, 100.0, n_bins + 1);
        std::sort(data.begin(), data.end());
        if (data.front() == data.back()) {
            // if X is constant, pass any two given points that shall be ignored in transform
            cutPoints.push_back(data.front());
            cutPoints.push_back(data.front());
            return;
        }
        // The requested edges, before collapsing: a value repeated here is a
        // mass point (see separate_mass_point).
        const auto requested = percentile(data, quantiles);
        // Percentiles are monotonic, so a repeated value can only be the
        // previous one: comparing against back() is enough to collapse
        // coincident quantiles into a single edge.
        cutPoints.clear();
        for (auto edge : requested) {
            if (cutPoints.empty() || edge != cutPoints.back()) {
                cutPoints.push_back(edge);
            }
        }
        for (size_t i = 1; i < requested.size(); ++i) {
            const bool repeated = requested[i] == requested[i - 1];
            const bool first_repeat = i < 2 || requested[i - 1] != requested[i - 2];
            if (repeated && first_repeat) {
                separate_mass_point(data, requested[i]);
            }
        }
    }
    void BinDisc::fit_uniform(const samples_t& X)
    {
        auto [vmin, vmax] = std::minmax_element(X.begin(), X.end());
        cutPoints = linspace(*vmin, *vmax, n_bins + 1);
    }
}