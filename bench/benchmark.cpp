// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// Baseline performance harness for the 3.0.0 release (RELEASE_PLAN_V3.md, Phase 0).
//
// Deliberately dependency-free: it reports wall-clock timings for the operations
// later phases intend to change, so their claims can be measured rather than
// asserted. Build with `make bench`, which forces a Release (-O3) build.
//
// Timings are reported as min / median / mean over repetitions, after warmup.
// The minimum is the most stable figure on a loaded machine and is the one to
// compare across runs; the spread between min and mean indicates how noisy the
// run was.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "BinDisc.h"
#include "CPPFImdlp.h"
#include "PKIDisc.h"

namespace {

    using clock_type = std::chrono::steady_clock;
    using mdlp::labels_t;
    using mdlp::samples_t;

    struct Stats {
        double min_ms = 0.0;
        double median_ms = 0.0;
        double mean_ms = 0.0;
        int reps = 0;
    };

    Stats measure(const std::function<void()>& body, int reps, int warmup)
    {
        for (int i = 0; i < warmup; ++i) {
            body();
        }
        std::vector<double> samples;
        samples.reserve(static_cast<size_t>(reps));
        for (int i = 0; i < reps; ++i) {
            const auto t0 = clock_type::now();
            body();
            const auto t1 = clock_type::now();
            samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        std::sort(samples.begin(), samples.end());
        Stats s;
        s.reps = reps;
        s.min_ms = samples.front();
        s.median_ms = samples[samples.size() / 2];
        s.mean_ms = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
        return s;
    }

    struct Dataset {
        samples_t X;
        labels_t y;
    };

    // Class-conditional normals: overlapping but separable, so MDLP has real
    // structure to find rather than degenerating to "no cut points".
    // Fixed seed, so every run measures the same work.
    Dataset make_dataset(size_t n, int n_classes, unsigned seed = 42u)
    {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> label_dist(0, n_classes - 1);
        Dataset d;
        d.X.reserve(n);
        d.y.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            const int label = label_dist(rng);
            std::normal_distribution<float> value_dist(static_cast<float>(label) * 2.0f, 1.0f);
            d.X.push_back(value_dist(rng));
            d.y.push_back(label);
        }
        return d;
    }

    // Repetition counts scaled so every cell takes roughly the same wall time.
    int reps_for(size_t n)
    {
        if (n <= 100) return 500;
        if (n <= 1000) return 200;
        if (n <= 10000) return 50;
        return 10;
    }

    int warmup_for(size_t n) { return n <= 1000 ? 20 : 3; }

    void print_header()
    {
        std::cout << std::left << std::setw(34) << "benchmark"
            << std::right << std::setw(10) << "n"
            << std::setw(7) << "reps"
            << std::setw(13) << "min (ms)"
            << std::setw(13) << "median (ms)"
            << std::setw(13) << "mean (ms)"
            << "\n"
            << std::string(90, '-') << "\n";
    }

    void print_row(const std::string& name, size_t n, const Stats& s)
    {
        std::cout << std::left << std::setw(34) << name
            << std::right << std::setw(10) << n
            << std::setw(7) << s.reps
            << std::fixed
            << std::setw(13) << std::setprecision(4) << s.min_ms
            << std::setw(13) << std::setprecision(4) << s.median_ms
            << std::setw(13) << std::setprecision(4) << s.mean_ms
            << "\n";
    }

    // Keeps the optimizer from discarding work whose result is otherwise unused.
    volatile size_t sink = 0;

}  // namespace

int main()
{
    const std::vector<size_t> sizes = { 100, 1000, 10000, 100000 };
    const int n_classes = 3;

    std::cout << "mdlp benchmark - baseline harness (RELEASE_PLAN_V3.md Phase 0)\n"
        << "library version: " << mdlp::Discretizer::version() << "\n"
        << "classes: " << n_classes << ", seed: 42\n\n";

    print_header();

    for (const auto n : sizes) {
        auto data = make_dataset(n, n_classes);
        const int reps = reps_for(n);
        const int warmup = warmup_for(n);

        // --- fit: the operation that copies its inputs today (see T5.1) ---
        print_row("CPPFImdlp::fit", n, measure([&] {
            mdlp::CPPFImdlp disc;
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        print_row("BinDisc::fit (uniform)", n, measure([&] {
            mdlp::BinDisc disc(5, mdlp::strategy_t::UNIFORM);
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        print_row("BinDisc::fit (quantile)", n, measure([&] {
            mdlp::BinDisc disc(5, mdlp::strategy_t::QUANTILE);
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        print_row("PKIDisc::fit (sqrt)", n, measure([&] {
            mdlp::PKIDisc disc(mdlp::compute_strategy_t::SQRT);
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        // --- transform: allocates into internal storage today (see T5.2) ---
        mdlp::CPPFImdlp fitted_mdlp;
        fitted_mdlp.fit(data.X, data.y);
        print_row("CPPFImdlp::transform", n, measure([&] {
            sink += fitted_mdlp.transform(data.X).size();
            }, reps, warmup));

        mdlp::BinDisc fitted_bin(5, mdlp::strategy_t::UNIFORM);
        fitted_bin.fit(data.X, data.y);
        print_row("BinDisc::transform", n, measure([&] {
            sink += fitted_bin.transform(data.X).size();
            }, reps, warmup));

        // --- copy cost of the inputs alone, for scale ---
        // fit() copies X and y, and Metrics copies y and indices again. This row
        // is the floor those copies cannot go below, useful when judging T5.1.
        print_row("(reference) copy X + y", n, measure([&] {
            samples_t X_copy = data.X;
            labels_t y_copy = data.y;
            sink += X_copy.size() + y_copy.size();
            }, reps, warmup));

        std::cout << "\n";
    }

    if (sink == 0) {
        std::cout << "(sink was zero)\n";
    }
    return 0;
}
