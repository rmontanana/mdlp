// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// Performance harness for the 3.0.0 release (RELEASE_PLAN_V3.md, Phase 0).
//
// Deliberately dependency-free: it reports wall-clock timings for the operations
// later phases intend to change, so their claims can be measured rather than
// asserted. Build with `make bench`, which forces a Release (-O3) build.
//
// Statistics: min / median / mean over a FIXED number of repetitions, after
// warmup. Repetition counts are identical on every platform on purpose — the
// minimum of a sample shrinks as the sample grows, so comparing minima taken
// with different rep counts would systematically favour whichever machine ran
// more of them. Use the MEDIAN for cross-platform comparison and the MINIMUM for
// before/after checks on one machine.
//
// Usage:
//   benchmark [--json PATH] [--level quick|full]
//     --json   also write machine-readable results to PATH
//     --level  quick stops at n=10,000; full includes n=100,000 (default: full)

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
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

    struct Result {
        std::string name;
        size_t n = 0;
        Stats stats;
    };

    Stats summarize(std::vector<double>& samples, int reps)
    {
        std::sort(samples.begin(), samples.end());
        Stats s;
        s.reps = reps;
        s.min_ms = samples.front();
        s.median_ms = samples[samples.size() / 2];
        s.mean_ms = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
        return s;
    }

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
        return summarize(samples, reps);
    }

    // For benchmarks whose body consumes its input, the replacement inputs must be
    // built outside the timed region. The body receives an index into a pool of
    // pre-made copies sized warmup + reps.
    Stats measure_indexed(const std::function<void(int)>& body, int reps, int warmup)
    {
        int k = 0;
        for (int i = 0; i < warmup; ++i) {
            body(k++);
        }
        std::vector<double> samples;
        samples.reserve(static_cast<size_t>(reps));
        for (int i = 0; i < reps; ++i) {
            const auto t0 = clock_type::now();
            body(k++);
            const auto t1 = clock_type::now();
            samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        return summarize(samples, reps);
    }

    struct Dataset {
        samples_t X;
        labels_t y;
    };

    // Class-conditional normals: overlapping but separable, so MDLP has real
    // structure to find rather than degenerating to "no cut points".
    // Fixed seed, so every run on every platform measures identical work.
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

    // FIXED across platforms. Do not make these adaptive: see the header comment.
    int reps_for(size_t n)
    {
        if (n <= 100) return 500;
        if (n <= 1000) return 200;
        if (n <= 10000) return 50;
        return 10;
    }

    int warmup_for(size_t n) { return n <= 1000 ? 20 : 3; }

    std::string compiler_id()
    {
        std::ostringstream os;
#if defined(__apple_build_version__)
        os << "AppleClang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__clang__)
        os << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
        os << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
        os << "MSVC " << _MSC_VER;
#else
        os << "unknown";
#endif
        return os.str();
    }

    void print_header()
    {
        std::cout << std::left << std::setw(36) << "benchmark"
            << std::right << std::setw(10) << "n"
            << std::setw(7) << "reps"
            << std::setw(13) << "min (ms)"
            << std::setw(13) << "median (ms)"
            << std::setw(13) << "mean (ms)"
            << "\n"
            << std::string(92, '-') << "\n";
    }

    void print_row(const Result& r)
    {
        std::cout << std::left << std::setw(36) << r.name
            << std::right << std::setw(10) << r.n
            << std::setw(7) << r.stats.reps
            << std::fixed
            << std::setw(13) << std::setprecision(4) << r.stats.min_ms
            << std::setw(13) << std::setprecision(4) << r.stats.median_ms
            << std::setw(13) << std::setprecision(4) << r.stats.mean_ms
            << "\n";
    }

    std::string json_escape(const std::string& s)
    {
        std::string out;
        for (const char c : s) {
            if (c == '"' || c == '\\') {
                out += '\\';
                out += c;
            } else {
                out += c;
            }
        }
        return out;
    }

    void write_json(const std::string& path,
        const std::vector<Result>& results,
        const std::string& level,
        int n_classes,
        const Stats& drift_before,
        const Stats& drift_after)
    {
        std::ofstream f(path);
        if (!f) {
            std::cerr << "benchmark: cannot write " << path << "\n";
            return;
        }
        f << std::fixed << std::setprecision(6);
        f << "{\n";
        f << "  \"schema\": 1,\n";
        f << "  \"library_version\": \"" << json_escape(mdlp::Discretizer::version()) << "\",\n";
        f << "  \"level\": \"" << json_escape(level) << "\",\n";
        f << "  \"n_classes\": " << n_classes << ",\n";
        f << "  \"seed\": 42,\n";
        f << "  \"build\": {\n";
        f << "    \"compiler\": \"" << json_escape(compiler_id()) << "\",\n";
        f << "    \"cpp_standard\": " << __cplusplus << ",\n";
        f << "    \"pointer_bits\": " << (sizeof(void*) * 8) << ",\n";
        f << "    \"hardware_concurrency\": " << std::thread::hardware_concurrency() << "\n";
        f << "  },\n";
        f << "  \"thermal_drift\": {\n";
        f << "    \"probe\": \"CPPFImdlp::fit n=1000\",\n";
        f << "    \"before_median_ms\": " << drift_before.median_ms << ",\n";
        f << "    \"after_median_ms\": " << drift_after.median_ms << "\n";
        f << "  },\n";
        f << "  \"results\": [\n";
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            f << "    {\"benchmark\": \"" << json_escape(r.name) << "\""
                << ", \"n\": " << r.n
                << ", \"reps\": " << r.stats.reps
                << ", \"min_ms\": " << r.stats.min_ms
                << ", \"median_ms\": " << r.stats.median_ms
                << ", \"mean_ms\": " << r.stats.mean_ms
                << "}";
            if (i + 1 < results.size()) f << ",";
            f << "\n";
        }
        f << "  ]\n";
        f << "}\n";
    }

    // Keeps the optimizer from discarding work whose result is otherwise unused.
    volatile size_t sink = 0;

}  // namespace

int main(int argc, char** argv)
{
    std::string json_path;
    std::string level = "full";
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--json") == 0 && i + 1 < argc) {
            json_path = argv[++i];
        } else if (std::strcmp(argv[i], "--level") == 0 && i + 1 < argc) {
            level = argv[++i];
        } else {
            std::cerr << "benchmark: unknown argument " << argv[i] << "\n";
            return 1;
        }
    }
    if (level != "quick" && level != "full") {
        std::cerr << "benchmark: --level must be quick or full\n";
        return 1;
    }

    std::vector<size_t> sizes = { 100, 1000, 10000 };
    if (level == "full") {
        sizes.push_back(100000);
    }
    const int n_classes = 3;

    // Thermal drift probe: an identical small workload measured at the start and
    // again at the end. A machine that throttled over the run shows it here.
    const auto drift_data = make_dataset(1000, n_classes);
    const auto drift_body = [&] {
        mdlp::CPPFImdlp disc;
        samples_t X = drift_data.X;
        labels_t y = drift_data.y;
        disc.fit(X, y);
        sink += disc.getCutPoints().size();
        };
    const Stats drift_before = measure(drift_body, 50, 10);

    std::cout << "mdlp benchmark (RELEASE_PLAN_V3.md Phase 0)\n"
        << "library version: " << mdlp::Discretizer::version() << "\n"
        << "compiler: " << compiler_id() << "\n"
        << "level: " << level << ", classes: " << n_classes << ", seed: 42\n"
        << "compare across platforms with the MEDIAN; rep counts are fixed\n\n";

    print_header();

    std::vector<Result> results;
    const auto record = [&](const std::string& name, size_t n, const Stats& s) {
        Result r{ name, n, s };
        results.push_back(r);
        print_row(r);
        };

    for (const auto n : sizes) {
        auto data = make_dataset(n, n_classes);
        const int reps = reps_for(n);
        const int warmup = warmup_for(n);

        // --- fit: the operation that copies its inputs (see T5.1) ---
        record("CPPFImdlp::fit", n, measure([&] {
            mdlp::CPPFImdlp disc;
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        record("BinDisc::fit (uniform)", n, measure([&] {
            mdlp::BinDisc disc(5, mdlp::strategy_t::UNIFORM);
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        record("BinDisc::fit (quantile)", n, measure([&] {
            mdlp::BinDisc disc(5, mdlp::strategy_t::QUANTILE);
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        record("PKIDisc::fit (sqrt)", n, measure([&] {
            mdlp::PKIDisc disc(mdlp::compute_strategy_t::SQRT);
            disc.fit(data.X, data.y);
            sink += disc.getCutPoints().size();
            }, reps, warmup));

        // --- transform ---
        mdlp::CPPFImdlp fitted_mdlp;
        fitted_mdlp.fit(data.X, data.y);
        record("CPPFImdlp::transform", n, measure([&] {
            sink += fitted_mdlp.transform(data.X).size();
            }, reps, warmup));

        mdlp::BinDisc fitted_bin(5, mdlp::strategy_t::UNIFORM);
        fitted_bin.fit(data.X, data.y);
        record("BinDisc::transform", n, measure([&] {
            sink += fitted_bin.transform(data.X).size();
            }, reps, warmup));

        // --- copy cost of the inputs alone, for scale ---
        record("(reference) copy X + y", n, measure([&] {
            samples_t X_copy = data.X;
            labels_t y_copy = data.y;
            sink += X_copy.size() + y_copy.size();
            }, reps, warmup));

        // --- T5.1: rvalue fit(), against the copying rows above ---
        {
            const int pool_size = reps + warmup;
            std::vector<samples_t> X_pool(static_cast<size_t>(pool_size), data.X);
            std::vector<labels_t> y_pool(static_cast<size_t>(pool_size), data.y);
            record("CPPFImdlp::fit (move)", n, measure_indexed([&](int i) {
                mdlp::CPPFImdlp disc;
                disc.fit(std::move(X_pool[static_cast<size_t>(i)]), std::move(y_pool[static_cast<size_t>(i)]));
                sink += disc.getCutPoints().size();
                }, reps, warmup));
        }
        // Control row: same pool, same access pattern, but copying. Reading a
        // different pool slot each rep costs cache locality that the plain
        // "BinDisc::fit (quantile)" row does not pay, so only this row is a fair
        // baseline for the move row that follows.
        {
            const int pool_size = reps + warmup;
            std::vector<samples_t> X_pool(static_cast<size_t>(pool_size), data.X);
            std::vector<labels_t> y_pool(static_cast<size_t>(pool_size), data.y);
            record("BinDisc::fit (quantile, pool copy)", n, measure_indexed([&](int i) {
                mdlp::BinDisc disc(5, mdlp::strategy_t::QUANTILE);
                disc.fit(X_pool[static_cast<size_t>(i)], y_pool[static_cast<size_t>(i)]);
                sink += disc.getCutPoints().size();
                }, reps, warmup));
        }
        {
            const int pool_size = reps + warmup;
            std::vector<samples_t> X_pool(static_cast<size_t>(pool_size), data.X);
            std::vector<labels_t> y_pool(static_cast<size_t>(pool_size), data.y);
            record("BinDisc::fit (quantile, move)", n, measure_indexed([&](int i) {
                mdlp::BinDisc disc(5, mdlp::strategy_t::QUANTILE);
                disc.fit(std::move(X_pool[static_cast<size_t>(i)]), std::move(y_pool[static_cast<size_t>(i)]));
                sink += disc.getCutPoints().size();
                }, reps, warmup));
        }

        // --- T5.2: transform into a reused caller buffer ---
        labels_t out_buffer;
        record("CPPFImdlp::transform (buffer)", n, measure([&] {
            fitted_mdlp.transform(data.X, out_buffer);
            sink += out_buffer.size();
            }, reps, warmup));

        std::cout << "\n";
    }

    const Stats drift_after = measure(drift_body, 50, 10);
    const double drift_pct = drift_before.median_ms > 0.0
        ? (drift_after.median_ms - drift_before.median_ms) / drift_before.median_ms * 100.0
        : 0.0;
    std::cout << "thermal drift probe (CPPFImdlp::fit n=1000 median): "
        << std::fixed << std::setprecision(4) << drift_before.median_ms << " ms before, "
        << drift_after.median_ms << " ms after ("
        << std::showpos << std::setprecision(1) << drift_pct << "%" << std::noshowpos << ")\n";
    if (drift_pct > 10.0) {
        std::cout << "  WARNING: the machine slowed down over the run; results may be throttled.\n";
    }

    if (!json_path.empty()) {
        write_json(json_path, results, level, n_classes, drift_before, drift_after);
        std::cout << "wrote " << json_path << "\n";
    }

    if (sink == 0) {
        std::cout << "(sink was zero)\n";
    }
    return 0;
}
