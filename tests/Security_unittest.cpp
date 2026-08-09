// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// Robustness against inputs that are large, degenerate or hostile.
//
// This file holds what is NOT already covered elsewhere, rather than restating
// it. Tensor validation lives in Discretizer_unittest (including the
// non-contiguous case), index bounds in FImdlp_unittest, and full real datasets
// in RealDatasets_unittest. What was missing was recursion depth, scale, and
// degenerate distributions.
//
// See SECURITY.md for the limitations these tests document rather than fix.

#include <cmath>
#include <limits>
#include <vector>
#include "gtest/gtest.h"
#include <string>
#include "CPPFImdlp.h"
#include "BinDisc.h"
#include "PKIDisc.h"

namespace mdlp {

    namespace {
        // Blocks of one class each, in increasing order: the shape that makes MDLP
        // recurse. Depth grows with the number of justified splits, so block size
        // controls it.
        void make_blocks(size_t n, size_t block, int classes, samples_t& X, labels_t& y)
        {
            X.resize(n);
            y.resize(n);
            for (size_t i = 0; i < n; ++i) {
                X[i] = static_cast<precision_t>(i);
                y[i] = static_cast<label_t>((i / block) % static_cast<size_t>(classes));
            }
        }
    }

    // ---- recursion depth --------------------------------------------------- //

    // computeCutPoints recurses once per split, so stack depth tracks the number
    // of cut points. This is the shape that pushes it, and it must complete.
    TEST(Security, DeepRecursionCompletes)
    {
        samples_t X;
        labels_t y;
        make_blocks(20000, 20, 7, X, y);

        CPPFImdlp disc;
        ASSERT_NO_THROW(disc.fit(X, y));
        EXPECT_GT(disc.get_depth(), 100) << "this input was meant to recurse deeply";
        EXPECT_GT(disc.getCutPoints().size(), 100u);
    }

    // The mitigation for the above. SECURITY.md recommends setting max_depth for
    // untrusted input; this proves it actually bounds the recursion.
    TEST(Security, MaxDepthBoundsTheRecursion)
    {
        samples_t X;
        labels_t y;
        make_blocks(20000, 20, 7, X, y);

        for (const int limit : { 1, 5, 20 }) {
            CPPFImdlp disc(3, limit, 0.0f);
            samples_t X_copy = X;
            labels_t y_copy = y;
            ASSERT_NO_THROW(disc.fit(X_copy, y_copy));
            EXPECT_LE(disc.get_depth(), limit)
                << "max_depth=" << limit << " did not bound the recursion";
        }
    }

    // Rapidly alternating labels carry no information, so the MDLP criterion
    // rejects every candidate. Confirms the stopping rule bounds recursion by
    // itself on data that would otherwise split endlessly.
    TEST(Security, UninformativeLabelsProduceNoSplits)
    {
        samples_t X(20000);
        labels_t y(20000);
        for (size_t i = 0; i < X.size(); ++i) {
            X[i] = static_cast<precision_t>(i);
            y[i] = static_cast<label_t>(i % 2);
        }
        CPPFImdlp disc;
        ASSERT_NO_THROW(disc.fit(X, y));
        EXPECT_EQ(1, disc.get_depth());
        EXPECT_EQ(2u, disc.getCutPoints().size()) << "only the min/max sentinels";
    }

    // ---- scale ------------------------------------------------------------- //

    // Distinct values on purpose: heavy duplication is exercised separately by
    // MassiveDuplicationIsHandled, and combining both here made this single test
    // take 13 seconds under coverage without testing anything the pair does not.
    TEST(Security, LargeInputCompletes)
    {
        const size_t n = 200000;
        samples_t X(n);
        labels_t y(n);
        for (size_t i = 0; i < n; ++i) {
            X[i] = static_cast<precision_t>(i);
            y[i] = static_cast<label_t>((i / 1000) % 3);
        }
        CPPFImdlp disc;
        ASSERT_NO_THROW(disc.fit(X, y));
        const auto labels = disc.transform(X);
        EXPECT_EQ(n, labels.size());
    }

    // ---- degenerate distributions ------------------------------------------ //

    TEST(Security, ConstantFeatureIsHandled)
    {
        samples_t X(1000, 42.0f);
        labels_t y(1000);
        for (size_t i = 0; i < y.size(); ++i) {
            y[i] = static_cast<label_t>(i % 3);
        }

        CPPFImdlp mdlp_disc;
        samples_t X_a = X;
        labels_t y_a = y;
        ASSERT_NO_THROW(mdlp_disc.fit(X_a, y_a));
        EXPECT_EQ(2u, mdlp_disc.getCutPoints().size()) << "a constant feature has no cut";

        BinDisc bins(4, strategy_t::QUANTILE);
        samples_t X_b = X;
        labels_t y_b = y;
        ASSERT_NO_THROW(bins.fit(X_b, y_b));
        ASSERT_NO_THROW(bins.transform(X));
    }

    TEST(Security, SingleClassProducesNoSplits)
    {
        samples_t X(1000);
        labels_t y(1000, 7);
        for (size_t i = 0; i < X.size(); ++i) {
            X[i] = static_cast<precision_t>(i);
        }
        CPPFImdlp disc;
        ASSERT_NO_THROW(disc.fit(X, y));
        EXPECT_EQ(2u, disc.getCutPoints().size());
    }

    TEST(Security, MassiveDuplicationIsHandled)
    {
        // Two distinct values only, each repeated thousands of times: exercises the
        // duplicate-scanning loops in valueCutPoint.
        samples_t X(20000);
        labels_t y(20000);
        for (size_t i = 0; i < X.size(); ++i) {
            X[i] = (i < X.size() / 2) ? 1.0f : 2.0f;
            y[i] = static_cast<label_t>(i < X.size() / 2 ? 0 : 1);
        }
        CPPFImdlp disc;
        ASSERT_NO_THROW(disc.fit(X, y));
        const auto labels = disc.transform(X);
        EXPECT_EQ(X.size(), labels.size());
    }

    TEST(Security, ExtremeMagnitudesAreHandled)
    {
        samples_t X = {
            -3.0e38f, -1.0e30f, -1.0f, 0.0f, 1.0e-30f, 1.0f, 1.0e30f, 3.0e38f
        };
        labels_t y = { 0, 0, 0, 0, 1, 1, 1, 1 };
        CPPFImdlp disc;
        ASSERT_NO_THROW(disc.fit(X, y));
        const auto labels = disc.transform(X);
        EXPECT_EQ(X.size(), labels.size());
    }

    // ---- non-finite input is rejected everywhere --------------------------- //

    // NaN has no strict weak ordering, so sorting it is undefined behaviour, and
    // both CPPFImdlp and BinDisc's QUANTILE strategy sort. Until 3.0.0 nothing
    // rejected it and the library returned nonsense. Infinities were rejected only
    // by BinDisc UNIFORM, through linspace, and accepted everywhere else.
    namespace {
        const float kNaN = std::numeric_limits<float>::quiet_NaN();
        const float kInf = std::numeric_limits<float>::infinity();
    }

    TEST(Security, FitRejectsNaN)
    {
        const labels_t y = { 0, 0, 0, 1, 1, 1 };
        const samples_t X = { 1.0f, 2.0f, kNaN, 4.0f, 5.0f, 6.0f };

        samples_t X_a = X; labels_t y_a = y;
        EXPECT_THROW(CPPFImdlp().fit(X_a, y_a), ValidationError);

        samples_t X_b = X; labels_t y_b = y;
        EXPECT_THROW(BinDisc(3, strategy_t::UNIFORM).fit(X_b, y_b), ValidationError);

        samples_t X_c = X; labels_t y_c = y;
        EXPECT_THROW(BinDisc(3, strategy_t::QUANTILE).fit(X_c, y_c), ValidationError);

        samples_t X_d = X; labels_t y_d = y;
        EXPECT_THROW(PKIDisc().fit(X_d, y_d), ValidationError);
    }

    TEST(Security, FitRejectsInfinity)
    {
        const labels_t y = { 0, 0, 0, 1, 1, 1 };
        for (const float bad : { kInf, -kInf }) {
            const samples_t X = { 1.0f, 2.0f, bad, 4.0f, 5.0f, 6.0f };

            samples_t X_a = X; labels_t y_a = y;
            EXPECT_THROW(CPPFImdlp().fit(X_a, y_a), ValidationError);

            samples_t X_b = X; labels_t y_b = y;
            EXPECT_THROW(BinDisc(3, strategy_t::UNIFORM).fit(X_b, y_b), ValidationError);

            samples_t X_c = X; labels_t y_c = y;
            EXPECT_THROW(BinDisc(3, strategy_t::QUANTILE).fit(X_c, y_c), ValidationError);
        }
    }

    // transform() is a separate entry point and was equally unguarded: a NaN there
    // silently landed in the last bin rather than being reported.
    TEST(Security, TransformRejectsNonFinite)
    {
        samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
        labels_t y = { 0, 0, 0, 1, 1, 1 };
        CPPFImdlp disc;
        disc.fit(X, y);

        const samples_t with_nan = { 1.0f, kNaN, 3.0f };
        const samples_t with_inf = { 1.0f, kInf, 3.0f };
        EXPECT_THROW(disc.transform(with_nan), ValidationError);
        EXPECT_THROW(disc.transform(with_inf), ValidationError);

        labels_t out;
        EXPECT_THROW(disc.transform(with_nan, out), ValidationError);
    }

    // The message must say which sample, since finding one bad value in 100,000
    // is the whole difficulty.
    TEST(Security, NonFiniteMessageNamesTheOffendingSample)
    {
        samples_t X = { 1.0f, 2.0f, 3.0f, kNaN, 5.0f, 6.0f };
        labels_t y = { 0, 0, 0, 1, 1, 1 };
        try {
            CPPFImdlp().fit(X, y);
            FAIL() << "expected ValidationError";
        }
        catch (const ValidationError& e) {
            const std::string what = e.what();
            EXPECT_NE(std::string::npos, what.find("index 3")) << what;
            EXPECT_NE(std::string::npos, what.find("nan")) << what;
        }
    }

    // The tensor entry points funnel into the same fit(), so they inherit it.
    TEST(Security, TensorEntryPointsRejectNonFinite)
    {
        auto X = torch::tensor({ 1.0f, 2.0f, kNaN, 4.0f, 5.0f, 6.0f }, torch::kFloat32);
        auto y = torch::tensor({ 0, 0, 0, 1, 1, 1 }, torch::kInt32);
        BinDisc disc(3, strategy_t::UNIFORM);
        EXPECT_THROW(disc.fit_t(X, y), ValidationError);
        EXPECT_THROW(disc.fit_transform_t(X, y), ValidationError);
    }
}
