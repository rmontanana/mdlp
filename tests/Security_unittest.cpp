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
#include "CPPFImdlp.h"
#include "BinDisc.h"

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

    // ---- infinities: current behaviour, which is inconsistent -------------- //

    // BinDisc UNIFORM rejects infinities because linspace validates its endpoints.
    // Nothing else does. This test pins what is actually guaranteed today; the
    // inconsistency is recorded in SECURITY.md rather than papered over.
    TEST(Security, UniformStrategyRejectsInfiniteValues)
    {
        samples_t X = { 1.0f, 2.0f, std::numeric_limits<float>::infinity(), 4.0f, 5.0f };
        labels_t y = { 0, 0, 0, 1, 1 };
        BinDisc disc(3, strategy_t::UNIFORM);
        EXPECT_THROW(disc.fit(X, y), InvalidParameter);
    }
}
