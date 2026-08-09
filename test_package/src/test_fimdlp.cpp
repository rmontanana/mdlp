// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// Smoke test for the packaged library.
//
// Its job is to prove that a *consumer* can compile and link against the conan
// package — which is exactly what was broken before 3.0.0: libtorch was declared
// PRIVATE while <torch/torch.h> sits in a public header, so the installed headers
// could not be compiled by anyone. This file must therefore use the real public
// API, headers included the way a consumer includes them.

#include <cstdlib>
#include <iostream>
#include <vector>

#include <fimdlp/CPPFImdlp.h>
#include <fimdlp/BinDisc.h>
#include <fimdlp/DiscretizerConfig.h>
#include <fimdlp/Exceptions.h>
#include <fimdlp/Metrics.h>

namespace {
    int failures = 0;

    void check(bool condition, const char* what)
    {
        std::cout << (condition ? "  ok   " : "  FAIL ") << what << '\n';
        if (!condition) {
            ++failures;
        }
    }
}

int main()
{
    std::cout << "fimdlp " << mdlp::Discretizer::version() << " package smoke test\n";

    const mdlp::samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    const mdlp::labels_t y = { 0, 0, 0, 0, 1, 1, 1, 1 };

    // Supervised discretization through the object API.
    {
        mdlp::samples_t X_fit = X;
        mdlp::labels_t y_fit = y;
        mdlp::CPPFImdlp disc;
        disc.fit(X_fit, y_fit);
        const auto cuts = disc.getCutPoints();
        check(cuts.size() >= 2, "CPPFImdlp::fit produced cut points");
        const auto bins = disc.transform(X);
        check(bins.size() == X.size(), "CPPFImdlp::transform returned one bin per sample");
    }

    // Named configuration and the one-call helper.
    {
        const auto bins = mdlp::CPPFImdlp::discretize(
            X, y, mdlp::MDLPConfig{}.withMinLength(3).withMaxDepth(10));
        check(bins.size() == X.size(), "CPPFImdlp::discretize returned a value");
    }

    // Unsupervised discretization; y is accepted and ignored.
    {
        mdlp::samples_t X_fit = X;
        mdlp::labels_t y_fit = y;
        mdlp::BinDisc bins(3, mdlp::strategy_t::QUANTILE);
        bins.fit(X_fit, y_fit);
        check(bins.getCutPoints().size() >= 2, "BinDisc::fit produced cut points");
    }

    // Entropy over an index range, which is what Metrics actually takes.
    {
        mdlp::labels_t labels = { 0, 0, 0, 0, 1, 1, 1, 1 };
        mdlp::indices_t indices = { 0, 1, 2, 3, 4, 5, 6, 7 };
        mdlp::Metrics metrics(labels, indices);
        const auto entropy = metrics.entropy(0, labels.size());
        check(entropy > 0.99f && entropy < 1.01f, "Metrics::entropy of a balanced split is 1 bit");
    }

    // The exception hierarchy is part of the public surface.
    {
        bool caught_as_library = false;
        bool caught_as_std = false;
        try {
            mdlp::CPPFImdlp bad(2, 10, 0);
        }
        catch (const mdlp::DiscretizerError&) { caught_as_library = true; }
        try {
            mdlp::CPPFImdlp bad(2, 10, 0);
        }
        catch (const std::invalid_argument&) { caught_as_std = true; }
        check(caught_as_library, "library errors are catchable as DiscretizerError");
        check(caught_as_std, "library errors are still catchable as std::invalid_argument");
    }

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "all checks passed\n";
    return EXIT_SUCCESS;
}
