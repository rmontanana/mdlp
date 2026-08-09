// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// Standalone toolchain diagnostic. NOT part of the library benchmark.
//
// Why it exists
// -------------
// The cross-platform runs show three operations consistently slower on Linux
// (GCC + libstdc++) than on macOS (AppleClang + libc++), on identical data and
// identical library sources:
//
//   BinDisc::fit (quantile)   2.9-3.2x slower at n=100,000
//   CPPFImdlp::transform      2.3-2.6x slower at n=100,000
//   CPPFImdlp::fit            1.10-1.12x slower since Phase 7 removed the
//                             quadratic term that had been masking it
//
// "Compiler" and "standard library" are two variables, and the project cannot
// separate them: libtorch arrives from conan prebuilt against libstdc++, and
// libstdc++ and libc++ have incompatible ABIs for std::string and std::vector,
// so the library itself cannot be built against libc++ without rebuilding
// libtorch from source.
//
// This file therefore has ZERO dependencies - no libtorch, no conan, no CMake -
// so it can be compiled three ways on one Linux machine:
//
//   g++      -O3                  GCC   + libstdc++
//   clang++  -O3                  clang + libstdc++   isolates the compiler
//   clang++  -O3 -stdlib=libc++   clang + libc++      isolates the library
//
// Limitation, stated plainly: these are *replicas* of the library's hot loops,
// not the library. A result here is a strong hypothesis about the cause, not a
// measurement of mdlp. Confirming it inside the library would need an
// ABI-compatible build, which is exactly what is unavailable.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    using clock_type = std::chrono::steady_clock;
    using precision_t = float;      // mdlp::precision_t
    using label_t = int;            // mdlp::label_t
    using samples_t = std::vector<precision_t>;
    using labels_t = std::vector<label_t>;
    using indices_t = std::vector<size_t>;

    // ---- toolchain identification ------------------------------------------

    std::string compiler_id()
    {
#if defined(__apple_build_version__)
        return "AppleClang " + std::to_string(__clang_major__);
#elif defined(__clang__)
        return "Clang " + std::to_string(__clang_major__);
#elif defined(__GNUC__)
        return "GCC " + std::to_string(__GNUC__);
#else
        return "unknown-compiler";
#endif
    }

    std::string stdlib_id()
    {
#if defined(_LIBCPP_VERSION)
        return "libc++ " + std::to_string(_LIBCPP_VERSION);
#elif defined(__GLIBCXX__)
        return "libstdc++ " + std::to_string(__GLIBCXX__);
#else
        return "unknown-stdlib";
#endif
    }

    // ---- portable data, identical to bench/benchmark.cpp --------------------

    double u01(std::mt19937& rng)
    {
        return static_cast<double>(rng() >> 8) * (1.0 / 16777216.0);
    }

    int bounded(std::mt19937& rng, int k)
    {
        const int v = static_cast<int>(u01(rng) * static_cast<double>(k));
        return v < k ? v : k - 1;
    }

    double normal01(std::mt19937& rng)
    {
        double s = 0.0;
        for (int i = 0; i < 12; ++i) {
            s += u01(rng);
        }
        return s - 6.0;
    }

    struct Dataset {
        samples_t X;
        labels_t y;
    };

    Dataset make_dataset(size_t n, int n_classes, unsigned seed = 42u)
    {
        std::mt19937 rng(seed);
        Dataset d;
        d.X.reserve(n);
        d.y.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            const int label = bounded(rng, n_classes);
            const double value = static_cast<double>(label) * 2.0 + normal01(rng);
            d.X.push_back(static_cast<precision_t>(value));
            d.y.push_back(static_cast<label_t>(label));
        }
        return d;
    }

    // ---- timing -------------------------------------------------------------

    struct Stats { double min_ms = 0.0; double median_ms = 0.0; };

    Stats measure(const std::function<void()>& body, int reps, int warmup)
    {
        for (int i = 0; i < warmup; ++i) body();
        std::vector<double> s;
        s.reserve(static_cast<size_t>(reps));
        for (int i = 0; i < reps; ++i) {
            const auto t0 = clock_type::now();
            body();
            const auto t1 = clock_type::now();
            s.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        std::sort(s.begin(), s.end());
        return { s.front(), s[s.size() / 2] };
    }

    volatile size_t sink = 0;

    // ---- replicas of the library's hot loops --------------------------------

    // (a) BinDisc::fit_quantile: sorts a by-value copy of the samples.
    void shape_sort_floats(const samples_t& X)
    {
        samples_t data = X;
        std::sort(data.begin(), data.end());
        sink += static_cast<size_t>(data.front() < data.back());
    }

    // (b) CPPFImdlp::sortIndices: stable_sort over an index vector with an
    // indirect comparator. Since Phase 7 this is the dominant cost of fit(),
    // so it is the shape that matters most now. The bounds checks inside the
    // comparator are part of the real code and are reproduced deliberately.
    void shape_stable_sort_indices(const samples_t& X_, const labels_t& y_)
    {
        indices_t idx(X_.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::stable_sort(idx.begin(), idx.end(), [&X_, &y_](size_t i1, size_t i2) {
            if (i1 >= X_.size() || i2 >= X_.size() || i1 >= y_.size() || i2 >= y_.size()) {
                throw std::out_of_range("Index out of bounds in sort comparison");
            }
            if (X_[i1] == X_[i2])
                return y_[i1] < y_[i2];
            else
                return X_[i1] < X_[i2];
            });
        sink += idx.front() + idx.back();
    }

    // (c) Discretizer::transform, as written today: the bound function is
    // selected into a *function pointer*, so every element pays an indirect call
    // the compiler cannot inline.
    void shape_transform_fnptr(const samples_t& data, const samples_t& cutPoints,
        labels_t& out, bool left)
    {
        out.clear();
        out.reserve(data.size());
        auto first = cutPoints.begin() + 1;
        auto last = cutPoints.end() - 1;
        auto bound = left
            ? std::lower_bound<samples_t::const_iterator, precision_t>
            : std::upper_bound<samples_t::const_iterator, precision_t>;
        for (const precision_t& item : data) {
            auto pos = bound(first, last, item);
            out.push_back(static_cast<label_t>(pos - first));
        }
        sink += out.size();
    }

    // (c') The same, with the branch hoisted out of the loop so the call can be
    // inlined. If this is much faster, the library has a portable win available
    // that helps every platform rather than favouring one toolchain.
    void shape_transform_direct(const samples_t& data, const samples_t& cutPoints,
        labels_t& out, bool left)
    {
        out.clear();
        out.reserve(data.size());
        auto first = cutPoints.begin() + 1;
        auto last = cutPoints.end() - 1;
        if (left) {
            for (const precision_t& item : data) {
                auto pos = std::lower_bound(first, last, item);
                out.push_back(static_cast<label_t>(pos - first));
            }
        } else {
            for (const precision_t& item : data) {
                auto pos = std::upper_bound(first, last, item);
                out.push_back(static_cast<label_t>(pos - first));
            }
        }
        sink += out.size();
    }

    int reps_for(size_t n) { return n <= 1000 ? 500 : (n <= 10000 ? 200 : 30); }

    void emit(const std::string& label, const std::string& toolchain,
        const std::string& shape, size_t n, const Stats& s)
    {
        std::cout << label << "," << toolchain << "," << shape << "," << n << ","
            << s.min_ms << "," << s.median_ms << "\n";
    }

}  // namespace

int main(int argc, char** argv)
{
    bool header = false;
    std::string label = "build";
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--header") == 0) header = true;
        else if (std::strcmp(argv[i], "--label") == 0 && i + 1 < argc) label = argv[++i];
    }
    // The label is how the build was invoked; the toolchain is what that
    // invocation actually resolved to. On macOS g++ is a symlink to clang, so
    // the two differ and only reporting the latter would silently merge builds
    // that are not separate at all.
    const std::string toolchain = compiler_id() + " / " + stdlib_id();

    if (header) {
        std::cout << "label,toolchain,shape,n,min_ms,median_ms\n";
    }
    std::cout.setf(std::ios::fixed);
    std::cout.precision(4);

    // Warm the governor before anything is timed, as bench/benchmark.cpp does.
    {
        const auto warm = make_dataset(10000, 3);
        const auto deadline = clock_type::now() + std::chrono::duration<double>(1.0);
        while (clock_type::now() < deadline) shape_sort_floats(warm.X);
    }

    for (const size_t n : { size_t(1000), size_t(10000), size_t(100000) }) {
        const auto d = make_dataset(n, 3);
        const int reps = reps_for(n);
        const int warmup = 5;

        emit(label, toolchain, "sort<float>", n,
            measure([&] { shape_sort_floats(d.X); }, reps, warmup));

        emit(label, toolchain, "stable_sort<index>", n,
            measure([&] { shape_stable_sort_indices(d.X, d.y); }, reps, warmup));

        // A plausible cut point set: 16 bins over the observed range.
        samples_t cuts;
        {
            const auto [lo, hi] = std::minmax_element(d.X.begin(), d.X.end());
            for (int i = 0; i <= 16; ++i) {
                cuts.push_back(*lo + (*hi - *lo) * static_cast<precision_t>(i) / 16.0f);
            }
        }
        labels_t out;
        emit(label, toolchain, "transform(fnptr)", n,
            measure([&] { shape_transform_fnptr(d.X, cuts, out, false); }, reps, warmup));
        emit(label, toolchain, "transform(direct)", n,
            measure([&] { shape_transform_direct(d.X, cuts, out, false); }, reps, warmup));
    }

    if (sink == 0) std::cerr << "(sink was zero)\n";
    return 0;
}
