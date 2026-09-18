// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <fstream>
#include <string>
#include <iostream>
#include "gtest/gtest.h"
#include <ArffFiles/ArffFiles.hpp>
#include "BinDisc.h"
#include "Experiments.hpp"
#include <cmath>
#include <type_traits>
#include <utility>

#define EXPECT_THROW_WITH_MESSAGE(stmt, etype, whatstring) EXPECT_THROW( \
try { \
stmt; \
} catch (const etype& ex) { \
EXPECT_EQ(whatstring, std::string(ex.what())); \
throw; \
} \
, etype)

namespace mdlp {
    const float margin = 1e-4;
    static std::string set_data_path()
    {
        std::string path = "datasets/";
        std::ifstream file(path + "iris.arff");
        if (file.is_open()) {
            file.close();
            return path;
        }
        return "tests/datasets/";
    }
    const std::string data_path = set_data_path();
    class TestBinDisc3U : public BinDisc, public testing::Test {
    public:
        TestBinDisc3U(int n_bins = 3) : BinDisc(n_bins, strategy_t::UNIFORM) {};
    };
    class TestBinDisc3Q : public BinDisc, public testing::Test {
    public:
        TestBinDisc3Q(int n_bins = 3) : BinDisc(n_bins, strategy_t::QUANTILE) {};
    };
    class TestBinDisc4U : public BinDisc, public testing::Test {
    public:
        TestBinDisc4U(int n_bins = 4) : BinDisc(n_bins, strategy_t::UNIFORM) {};
    };
    class TestBinDisc4Q : public BinDisc, public testing::Test {
    public:
        TestBinDisc4Q(int n_bins = 4) : BinDisc(n_bins, strategy_t::QUANTILE) {};
    };
    TEST_F(TestBinDisc3U, Easy3BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
        auto y = labels_t();
        fit(X, y);
        auto cuts = getCutPoints();
        ASSERT_EQ(4, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(3.66667, cuts.at(1), margin);
        EXPECT_NEAR(6.33333, cuts.at(2), margin);
        EXPECT_NEAR(9.0, cuts.at(3), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3Q, Easy3BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(4, cuts.size());
        EXPECT_NEAR(1, cuts[0], margin);
        EXPECT_NEAR(3.666667, cuts[1], margin);
        EXPECT_NEAR(6.333333, cuts[2], margin);
        EXPECT_NEAR(9, cuts[3], margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, X10BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(4, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(4.0, cuts.at(1), margin);
        EXPECT_NEAR(7.0, cuts.at(2), margin);
        EXPECT_NEAR(10.0, cuts.at(3), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3Q, X10BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(4, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(4.0, cuts.at(1), margin);
        EXPECT_NEAR(7.0, cuts.at(2), margin);
        EXPECT_NEAR(10.0, cuts.at(3), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, X11BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(4, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(4.33333, cuts.at(1), margin);
        EXPECT_NEAR(7.66667, cuts.at(2), margin);
        EXPECT_NEAR(11.0, cuts.at(3), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, X11BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(4, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(4.33333, cuts.at(1), margin);
        EXPECT_NEAR(7.66667, cuts.at(2), margin);
        EXPECT_NEAR(11.0, cuts.at(3), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, ConstantUniform)
    {
        samples_t X = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(2, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(1, cuts.at(1), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 0, 0 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3Q, ConstantQuantile)
    {
        samples_t X = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(2, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(1, cuts.at(1), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 0, 0 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, EmptyUniform)
    {
        samples_t X = {};
        EXPECT_THROW(fit(X), std::invalid_argument);
    }
    TEST_F(TestBinDisc3Q, EmptyQuantile)
    {
        samples_t X = {};
        EXPECT_THROW(fit(X), std::invalid_argument);
    }
    TEST(TestBinDisc3, ExceptionNumberBins)
    {
        EXPECT_THROW(BinDisc(2), std::invalid_argument);
    }
    TEST_F(TestBinDisc3U, EasyRepeated)
    {
        samples_t X = { 3.0, 1.0, 1.0, 3.0, 1.0, 1.0, 3.0, 1.0, 1.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(4, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(1.66667, cuts.at(1), margin);
        EXPECT_NEAR(2.33333, cuts.at(2), margin);
        EXPECT_NEAR(3.0, cuts.at(3), margin);
        auto labels = transform(X);
        labels_t expected = { 2, 0, 0, 2, 0, 0, 2, 0, 0 };
        EXPECT_EQ(expected, labels);
        ASSERT_EQ(3.0, X[0]); // X is not modified
    }
    TEST_F(TestBinDisc3Q, EasyRepeated)
    {
        samples_t X = { 3.0, 1.0, 1.0, 3.0, 1.0, 1.0, 3.0, 1.0, 1.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(3, cuts.size());
        EXPECT_NEAR(1, cuts.at(0), margin);
        EXPECT_NEAR(1.66667, cuts.at(1), margin);
        EXPECT_NEAR(3.0, cuts.at(2), margin);
        auto labels = transform(X);
        labels_t expected = { 1, 0, 0, 1, 0, 0, 1, 0, 0 };
        EXPECT_EQ(expected, labels);
        ASSERT_EQ(3.0, X[0]); // X is not modified
    }
    TEST_F(TestBinDisc4U, Easy4BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(3.75, cuts.at(1), margin);
        EXPECT_NEAR(6.5, cuts.at(2), margin);
        EXPECT_NEAR(9.25, cuts.at(3), margin);
        EXPECT_NEAR(12.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, Easy4BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(3.75, cuts.at(1), margin);
        EXPECT_NEAR(6.5, cuts.at(2), margin);
        EXPECT_NEAR(9.25, cuts.at(3), margin);
        EXPECT_NEAR(12.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, X13BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(4.0, cuts.at(1), margin);
        EXPECT_NEAR(7.0, cuts.at(2), margin);
        EXPECT_NEAR(10.0, cuts.at(3), margin);
        EXPECT_NEAR(13.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, X13BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(4.0, cuts.at(1), margin);
        EXPECT_NEAR(7.0, cuts.at(2), margin);
        EXPECT_NEAR(10.0, cuts.at(3), margin);
        EXPECT_NEAR(13.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, X14BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(4.25, cuts.at(1), margin);
        EXPECT_NEAR(7.5, cuts.at(2), margin);
        EXPECT_NEAR(10.75, cuts.at(3), margin);
        EXPECT_NEAR(14.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, X14BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(4.25, cuts.at(1), margin);
        EXPECT_NEAR(7.5, cuts.at(2), margin);
        EXPECT_NEAR(10.75, cuts.at(3), margin);
        EXPECT_NEAR(14.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, X15BinsUniform)
    {
        samples_t X = { 15.0, 8.0, 12.0, 14.0, 6.0, 1.0, 13.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(4.5, cuts.at(1), margin);
        EXPECT_NEAR(8, cuts.at(2), margin);
        EXPECT_NEAR(11.5, cuts.at(3), margin);
        EXPECT_NEAR(15.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 3, 2, 3, 3, 1, 0, 3, 2, 2, 2, 1, 0, 0, 1, 0 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, X15BinsQuantile)
    {
        samples_t X = { 15.0, 13.0, 12.0, 14.0, 6.0, 1.0, 8.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(1.0, cuts.at(0), margin);
        EXPECT_NEAR(4.5, cuts.at(1), margin);
        EXPECT_NEAR(8, cuts.at(2), margin);
        EXPECT_NEAR(11.5, cuts.at(3), margin);
        EXPECT_NEAR(15.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 3, 3, 3, 3, 1, 0, 2, 2, 2, 2, 1, 0, 0, 1, 0 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, RepeatedValuesUniform)
    {
        samples_t X = { 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0 };
        //               0    1     2   3    4    5    6    7    8    9
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(0.0, cuts.at(0), margin);
        EXPECT_NEAR(1.0, cuts.at(1), margin);
        EXPECT_NEAR(2.0, cuts.at(2), margin);
        EXPECT_NEAR(3.0, cuts.at(3), margin);
        EXPECT_NEAR(4.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 1, 1, 1, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, RepeatedValuesQuantile)
    {
        samples_t X = { 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0 };
        //               0    1     2   3    4    5    6    7    8    9
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(5, cuts.size());
        EXPECT_NEAR(0.0, cuts.at(0), margin);
        EXPECT_NEAR(1.0, cuts.at(1), margin);
        EXPECT_NEAR(2.0, cuts.at(2), margin);
        EXPECT_NEAR(3.0, cuts.at(3), margin);
        EXPECT_NEAR(4.0, cuts.at(4), margin);
        auto labels = transform(X);
        labels_t expected = { 0, 1, 1, 1, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    // QUANTILE collapses coincident percentiles, but a value that swallowed
    // edges (a mass point) still gets a bin of its own. Here 10 of 12 samples
    // share one value and four bins are asked for: every requested edge but
    // the last lands on 1, so without the guarantee the result would be
    // [min, max] and a single bin. The value is the minimum, so only the
    // next distinct value (2) is needed to set it apart; 3 is not a mass
    // point and is not separated from 2. UNIFORM, which places its edges on
    // the range, still returns n_bins + 1.
    TEST(TestBinDiscGeneric, QuantileKeepsMassPointApart)
    {
        samples_t X = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3 };
        labels_t y(X.size(), 0);

        BinDisc quantile(4, strategy_t::QUANTILE);
        quantile.fit(X, y);
        const auto q_cuts = quantile.getCutPoints();
        ASSERT_EQ(3u, q_cuts.size());
        EXPECT_NEAR(1.0f, q_cuts[0], margin);
        EXPECT_NEAR(2.0f, q_cuts[1], margin);
        EXPECT_NEAR(3.0f, q_cuts[2], margin);
        labels_t expected = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
        EXPECT_EQ(expected, quantile.transform(X));

        BinDisc uniform(4, strategy_t::UNIFORM);
        uniform.fit(X, y);
        EXPECT_EQ(5u, uniform.getCutPoints().size());
    }
    // A binary feature is the mass-point case that matters most: with plain
    // percentile collapsing it always ends up as [0, 1] and one bin, whatever
    // the proportion of ones. It must map onto two bins at any proportion.
    TEST(TestBinDiscGeneric, QuantileBinaryFeatureKeepsTwoBins)
    {
        for (double ones : { 0.05, 0.43, 0.50, 0.80, 0.95 }) {
            const size_t n = 1000;
            const size_t n_ones = static_cast<size_t>(n * ones);
            samples_t X;
            for (size_t i = 0; i < n; ++i) {
                X.push_back(i < n_ones ? 1.0f : 0.0f);
            }
            labels_t y(X.size(), 0);
            BinDisc disc(7, strategy_t::QUANTILE);
            disc.fit(X, y);
            const auto labels = disc.transform(X);
            for (size_t i = 0; i < n; ++i) {
                ASSERT_EQ(i < n_ones ? 1 : 0, labels[i]) << "ones=" << ones << " i=" << i;
            }
            // Exactly two bins: no empty one was created either side.
            EXPECT_EQ(3u, disc.getCutPoints().size()) << "ones=" << ones;
        }
    }
    // Sparse continuous feature: 90 % zeros, the rest spread over (0, 1].
    // Every requested edge but the last lands on 0, so the zeros are a mass
    // point; they must not share a bin with the non-zeros, and the cut that
    // sets them apart is the smallest non-zero value.
    TEST(TestBinDiscGeneric, QuantileSparseFeatureSeparatesZeros)
    {
        samples_t X(900, 0.0f);
        for (int i = 1; i <= 100; ++i) {
            X.push_back(static_cast<precision_t>(i) / 100.0f);
        }
        labels_t y(X.size(), 0);
        BinDisc disc(8, strategy_t::QUANTILE);
        disc.fit(X, y);
        const auto labels = disc.transform(X);
        for (size_t i = 0; i < 900; ++i) {
            ASSERT_EQ(0, labels[i]);
        }
        for (size_t i = 900; i < X.size(); ++i) {
            ASSERT_GE(labels[i], 1) << "non-zero " << X[i] << " fell into the zeros' bin";
        }
        const auto cuts = disc.getCutPoints();
        EXPECT_NEAR(0.01f, cuts[1], margin) << "the first cut is the smallest non-zero value";
    }
    // Several mass points: three values at 30 / 40 / 30 %. Each must get its
    // own bin, including the one at the maximum, which needs a cut equal to
    // the max sentinel.
    TEST(TestBinDiscGeneric, QuantileThreeValuedFeatureKeepsThreeBins)
    {
        samples_t X;
        for (int i = 0; i < 30; ++i) X.push_back(0.0f);
        for (int i = 0; i < 40; ++i) X.push_back(1.0f);
        for (int i = 0; i < 30; ++i) X.push_back(2.0f);
        labels_t y(X.size(), 0);
        BinDisc disc(7, strategy_t::QUANTILE);
        disc.fit(X, y);
        const auto labels = disc.transform(X);
        for (size_t i = 0; i < X.size(); ++i) {
            ASSERT_EQ(static_cast<label_t>(X[i]), labels[i]) << "i=" << i;
        }
        EXPECT_EQ(4u, disc.getCutPoints().size());
    }
    // When an interpolated edge already falls between the mass point and its
    // neighbour, nothing is added: adding the neighbour too would create an
    // empty bin. 7 zeros and 3 ones with 4 bins: the 75th percentile is
    // interpolated between the last 0 and the first 1.
    TEST(TestBinDiscGeneric, QuantileDoesNotDuplicateAnExistingSeparation)
    {
        samples_t X = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
        labels_t y(X.size(), 0);
        BinDisc disc(4, strategy_t::QUANTILE);
        disc.fit(X, y);
        const auto cuts = disc.getCutPoints();
        ASSERT_EQ(3u, cuts.size());
        EXPECT_GT(cuts[1], 0.0f);
        EXPECT_LE(cuts[1], 1.0f);
        labels_t expected = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
        EXPECT_EQ(expected, disc.transform(X));
    }
    // Data without mass points is not touched: the edges are the plain
    // percentiles, exactly as before.
    TEST(TestBinDiscGeneric, QuantileWithoutMassPointsIsUnchanged)
    {
        samples_t X;
        for (int i = 0; i < 100; ++i) X.push_back(static_cast<precision_t>(i));
        labels_t y(X.size(), 0);
        BinDisc disc(4, strategy_t::QUANTILE);
        disc.fit(X, y);
        const auto cuts = disc.getCutPoints();
        ASSERT_EQ(5u, cuts.size());
        EXPECT_NEAR(0.0f, cuts[0], margin);
        EXPECT_NEAR(24.75f, cuts[1], margin);
        EXPECT_NEAR(49.5f, cuts[2], margin);
        EXPECT_NEAR(74.25f, cuts[3], margin);
        EXPECT_NEAR(99.0f, cuts[4], margin);
    }

    TEST(TestBinDiscGeneric, Fileset)
    {
        Experiments exps(data_path + "tests.txt");
        int num = 0;
        while (exps.is_next()) {
            ++num;
            Experiment exp = exps.next();
            BinDisc disc(exp.n_bins_, exp.strategy_[0] == 'Q' ? strategy_t::QUANTILE : strategy_t::UNIFORM);
            std::vector<precision_t> test;
            if (exp.type_ == experiment_t::RANGE) {
                for (float i = exp.from_; i < exp.to_; i += exp.step_) {
                    test.push_back(i);
                }
            } else {
                test = exp.dataset_;
            }
            // show_vector(test, "Test");
            auto empty = std::vector<int>();
            auto Xt = disc.fit_transform(test, empty);
            auto cuts = disc.getCutPoints();
            EXPECT_EQ(exp.discretized_data_.size(), Xt.size());
            auto flag = false;
            size_t n_errors = 0;
            if (num < 40) {
                //
                // Check discretization of only the first 40 tests as after we cannot ensure the same codification due to precision problems
                //
                for (int i = 0; i < exp.discretized_data_.size(); ++i) {
                    if (exp.discretized_data_.at(i) != Xt.at(i)) {
                        if (!flag) {
                            if (exp.type_ == experiment_t::RANGE)
                                std::cout << "+Exp #: " << num << " From: " << exp.from_ << " To: " << exp.to_ << " Step: " << exp.step_ << " Bins: " << exp.n_bins_ << " Strategy: " << exp.strategy_ << std::endl;
                            else {
                                std::cout << "+Exp #: " << num << " strategy: " << exp.strategy_ << " " << " n_bins: " << exp.n_bins_ << " ";
                                show_vector(exp.dataset_, "Dataset");
                            }
                            show_vector(cuts, "Cuts");
                            std::cout << "Error at " << i << " test[i]=" << test.at(i) << " Expected: " << exp.discretized_data_.at(i) << " Got: " << Xt.at(i) << std::endl;
                            flag = true;
                            EXPECT_EQ(exp.discretized_data_.at(i), Xt.at(i));
                        }
                        n_errors++;
                    }
                }
                if (flag) {
                    std::cout << "*** Found " << n_errors << " mistakes in this experiment dataset" << std::endl;
                }
            }
            EXPECT_EQ(exp.cutpoints_.size(), cuts.size());
            for (int i = 0; i < exp.cutpoints_.size(); ++i) {
                EXPECT_NEAR(exp.cutpoints_.at(i), cuts.at(i), margin);
            }
        }
        // std::cout << "* Number of experiments tested: " << num << std::endl;
    }

    TEST_F(TestBinDisc3U, FitDataSizeTooSmall)
    {
        // Test when data size is smaller than n_bins
        samples_t X = { 1.0, 2.0 }; // Only 2 elements for 3 bins
        EXPECT_THROW_WITH_MESSAGE(fit(X), std::invalid_argument, "Input data size (2) must be at least n_bins (3)");
    }

    TEST_F(TestBinDisc3Q, FitDataSizeTooSmall)
    {
        // Test when data size is smaller than n_bins
        samples_t X = { 1.0, 2.0 }; // Only 2 elements for 3 bins
        EXPECT_THROW_WITH_MESSAGE(fit(X), std::invalid_argument, "Input data size (2) must be at least n_bins (3)");
    }

    TEST_F(TestBinDisc3U, FitWithYEmptyX)
    {
        // Test fit(X, y) with empty X
        samples_t X = {};
        labels_t y = { 1, 2, 3 };
        EXPECT_THROW_WITH_MESSAGE(fit(X, y), std::invalid_argument, "X cannot be empty");
    }

    TEST_F(TestBinDisc3U, LinspaceInvalidNumPoints)
    {
        // Test linspace with num < 2
        EXPECT_THROW_WITH_MESSAGE(linspace(0.0f, 1.0f, 1), std::invalid_argument, "linspace: num must be at least 2, got 1");
    }

    TEST_F(TestBinDisc3U, LinspaceNaNValues)
    {
        // Test linspace with NaN values
        float nan_val = std::numeric_limits<float>::quiet_NaN();
        EXPECT_THROW_WITH_MESSAGE(linspace(nan_val, 1.0f, 3), std::invalid_argument, "Start and end values cannot be NaN");
        EXPECT_THROW_WITH_MESSAGE(linspace(0.0f, nan_val, 3), std::invalid_argument, "Start and end values cannot be NaN");
    }

    TEST_F(TestBinDisc3U, LinspaceInfiniteValues)
    {
        // Test linspace with infinite values
        float inf_val = std::numeric_limits<float>::infinity();
        EXPECT_THROW_WITH_MESSAGE(linspace(inf_val, 1.0f, 3), std::invalid_argument, "Start and end values cannot be infinite");
        EXPECT_THROW_WITH_MESSAGE(linspace(0.0f, inf_val, 3), std::invalid_argument, "Start and end values cannot be infinite");
    }

    TEST_F(TestBinDisc3U, PercentileEmptyData)
    {
        // Test percentile with empty data
        samples_t empty_data = {};
        std::vector<precision_t> percentiles = { 25.0f, 50.0f, 75.0f };
        EXPECT_THROW_WITH_MESSAGE(percentile(empty_data, percentiles), std::invalid_argument, "Data cannot be empty for percentile calculation");
    }

    TEST_F(TestBinDisc3U, PercentileEmptyPercentiles)
    {
        // Test percentile with empty percentiles
        samples_t data = { 1.0f, 2.0f, 3.0f };
        std::vector<precision_t> empty_percentiles = {};
        EXPECT_THROW_WITH_MESSAGE(percentile(data, empty_percentiles), std::invalid_argument, "Percentiles cannot be empty");
    }

    // percentile() answers every request, repeats included: it is fit_quantile
    // that collapses them, and it needs the repeats to spot mass points.
    TEST_F(TestBinDisc3U, PercentileKeepsRepeatedValues)
    {
        samples_t data = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        std::vector<precision_t> percentiles = { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f };
        const auto values = percentile(data, percentiles);
        ASSERT_EQ(5u, values.size());
        EXPECT_NEAR(0.0f, values[0], margin);
        EXPECT_NEAR(0.0f, values[1], margin);
        EXPECT_NEAR(0.0f, values[2], margin);
        EXPECT_NEAR(0.0f, values[3], margin);
        EXPECT_NEAR(1.0f, values[4], margin);
    }
}

namespace mdlp {
    // T5.1: only QUANTILE benefits (it sorts a copy today), but both strategies
    // must give identical results either way.
    TEST(BinDiscMove, QuantileMoveMatchesCopy)
    {
        const samples_t X_source = { 5.0f, 1.0f, 9.0f, 3.0f, 7.0f, 2.0f, 8.0f, 4.0f, 6.0f, 0.0f };
        const labels_t y_source = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };

        samples_t X_copy = X_source;
        labels_t y_copy = y_source;
        BinDisc by_copy(4, strategy_t::QUANTILE);
        by_copy.fit(X_copy, y_copy);

        samples_t X_moved = X_source;
        labels_t y_moved = y_source;
        BinDisc by_move(4, strategy_t::QUANTILE);
        by_move.fit(std::move(X_moved), std::move(y_moved));

        EXPECT_EQ(by_copy.getCutPoints(), by_move.getCutPoints());

        samples_t probe = X_source;
        EXPECT_EQ(by_copy.transform(probe), by_move.transform(probe));
    }

    TEST(BinDiscMove, UniformMoveMatchesCopy)
    {
        const samples_t X_source = { 5.0f, 1.0f, 9.0f, 3.0f, 7.0f, 2.0f, 8.0f, 4.0f, 6.0f, 0.0f };
        const labels_t y_source = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };

        samples_t X_copy = X_source;
        labels_t y_copy = y_source;
        BinDisc by_copy(4, strategy_t::UNIFORM);
        by_copy.fit(X_copy, y_copy);

        samples_t X_moved = X_source;
        labels_t y_moved = y_source;
        BinDisc by_move(4, strategy_t::UNIFORM);
        by_move.fit(std::move(X_moved), std::move(y_moved));

        EXPECT_EQ(by_copy.getCutPoints(), by_move.getCutPoints());
    }

    // The samples-only overload also has an rvalue form.
    TEST(BinDiscMove, SamplesOnlyMoveMatchesCopy)
    {
        const samples_t X_source = { 5.0f, 1.0f, 9.0f, 3.0f, 7.0f, 2.0f, 8.0f, 4.0f, 6.0f, 0.0f };

        samples_t X_copy = X_source;
        BinDisc by_copy(4, strategy_t::QUANTILE);
        by_copy.fit(X_copy);

        samples_t X_moved = X_source;
        BinDisc by_move(4, strategy_t::QUANTILE);
        by_move.fit(std::move(X_moved));

        EXPECT_EQ(by_copy.getCutPoints(), by_move.getCutPoints());
    }

    TEST(BinDiscMove, RvalueFitRejectsEmptyX)
    {
        BinDisc disc(4, strategy_t::QUANTILE);
        samples_t empty_X;
        labels_t y = { 0, 1, 0, 1 };
        EXPECT_THROW_WITH_MESSAGE(disc.fit(std::move(empty_X), std::move(y)),
            std::invalid_argument, "X cannot be empty");
    }

    // min_bins became `static constexpr`; a const non-static member would have
    // deleted these operators for BinDisc and PKIDisc.
    TEST(BinDiscMove, IsCopyableAndMovable)
    {
        static_assert(std::is_copy_constructible_v<BinDisc>, "BinDisc must be copy constructible");
        static_assert(std::is_copy_assignable_v<BinDisc>, "BinDisc must be copy assignable");
        static_assert(std::is_move_constructible_v<BinDisc>, "BinDisc must be move constructible");
        static_assert(std::is_move_assignable_v<BinDisc>, "BinDisc must be move assignable");
        SUCCEED();
    }
}
