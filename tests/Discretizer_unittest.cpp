// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <fstream>
#include <string>
#include <iostream>
#include <ArffFiles.hpp>
#include "gtest/gtest.h"
#include "Discretizer.h"
#include "BinDisc.h"
#include "CPPFImdlp.h"

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
        std::string path = "tests/datasets/";
        std::ifstream file(path + "iris.arff");
        if (file.is_open()) {
            file.close();
            return path;
        }
        return "datasets/";
    }
    const std::string data_path = set_data_path();
    const labels_t iris_quantile = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 3, 3, 3, 1, 3, 1, 2, 0, 3, 1, 0, 2, 2, 2, 1, 3, 1, 2, 2, 1, 2, 2, 2, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2, 1, 2, 3, 2, 1, 1, 1, 2, 2, 0, 1, 1, 1, 2, 1, 1, 2, 2, 3, 2, 3, 3, 0, 3, 3, 3, 3, 3, 3, 1, 2, 3, 3, 3, 3, 2, 3, 1, 3, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 2, 2 };
    TEST(Discretizer, Version)
    {
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);
        auto version = disc->version();
        delete disc;
        EXPECT_EQ("3.0.0", version);
    }
    TEST(Discretizer, BinIrisUniform)
    {
        ArffFiles file;
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);
        file.load(data_path + "iris.arff", true);
        vector<samples_t>& X = file.getX();
        auto y = labels_t();
        disc->fit(X[0], y);
        auto Xt = disc->transform(X[0]);
        labels_t expected = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 2, 2, 1, 2, 1, 2, 0, 2, 0, 0, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 0, 1, 2, 1, 3, 2, 2, 3, 0, 3, 2, 3, 2, 2, 2, 1, 1, 2, 2, 3, 3, 1, 2, 1, 3, 2, 2, 3, 2, 1, 2, 3, 3, 3, 2, 2, 1, 3, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };
        delete disc;
        EXPECT_EQ(expected, Xt);
    }
    TEST(Discretizer, BinIrisQuantile)
    {
        ArffFiles file;
        Discretizer* disc = new BinDisc(4, strategy_t::QUANTILE);
        file.load(data_path + "iris.arff", true);
        vector<samples_t>& X = file.getX();
        auto y = labels_t();
        disc->fit(X[0], y);
        auto Xt = disc->transform(X[0]);
        delete disc;
        EXPECT_EQ(iris_quantile, Xt);
    }

    TEST(Discretizer, BinIrisQuantileTorch)
    {
        ArffFiles file;
        Discretizer* disc = new BinDisc(4, strategy_t::QUANTILE);
        file.load(data_path + "iris.arff", true);
        auto X = file.getX();
        auto y = file.getY();
        auto X_torch = torch::tensor(X[0], torch::kFloat32);
        auto yt = torch::tensor(y, torch::kInt32);
        disc->fit_t(X_torch, yt);
        torch::Tensor Xt = disc->transform_t(X_torch);
        delete disc;
        EXPECT_EQ(iris_quantile.size(), Xt.size(0));
        for (int i = 0; i < iris_quantile.size(); ++i) {
            EXPECT_EQ(iris_quantile.at(i), Xt[i].item<int>());
        }
    }
    TEST(Discretizer, BinIrisQuantileTorchFit_transform)
    {
        ArffFiles file;
        Discretizer* disc = new BinDisc(4, strategy_t::QUANTILE);
        file.load(data_path + "iris.arff", true);
        auto X = file.getX();
        auto y = file.getY();
        auto X_torch = torch::tensor(X[0], torch::kFloat32);
        auto yt = torch::tensor(y, torch::kInt32);
        torch::Tensor Xt = disc->fit_transform_t(X_torch, yt);
        delete disc;
        EXPECT_EQ(iris_quantile.size(), Xt.size(0));
        for (int i = 0; i < iris_quantile.size(); ++i) {
            EXPECT_EQ(iris_quantile.at(i), Xt[i].item<int>());
        }
    }

    TEST(Discretizer, FImdlpIris)
    {
        auto labelsq = {
            1,
                0,
                0,
                0,
                0,
                1,
                0,
                0,
                0,
                0,
                1,
                0,
                0,
                0,
                2,
                1,
                1,
                1,
                1,
                1,
                1,
                1,
                0,
                1,
                0,
                0,
                0,
                1,
                1,
                0,
                0,
                1,
                1,
                1,
                0,
                0,
                1,
                0,
                0,
                1,
                0,
                0,
                0,
                0,
                1,
                0,
                1,
                0,
                1,
                0,
                3,
                3,
                3,
                1,
                3,
                1,
                2,
                0,
                3,
                1,
                0,
                2,
                2,
                2,
                1,
                3,
                1,
                2,
                2,
                1,
                2,
                2,
                2,
                2,
                3,
                3,
                3,
                3,
                2,
                1,
                1,
                1,
                2,
                2,
                1,
                2,
                3,
                2,
                1,
                1,
                1,
                2,
                2,
                0,
                1,
                1,
                1,
                2,
                1,
                1,
                2,
                2,
                3,
                2,
                3,
                3,
                0,
                3,
                3,
                3,
                3,
                3,
                3,
                1,
                2,
                3,
                3,
                3,
                3,
                2,
                3,
                1,
                3,
                2,
                3,
                3,
                2,
                2,
                3,
                3,
                3,
                3,
                3,
                2,
                2,
                3,
                2,
                3,
                2,
                3,
                3,
                3,
                2,
                3,
                3,
                3,
                2,
                3,
                2,
                2,
        };
        labels_t expected = {
            5, 3, 4, 4, 5, 5, 5, 5, 2, 4, 5, 5, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5,
            5, 4, 5, 3, 5, 5, 5, 4, 4, 5, 5, 5, 4, 4, 5, 4, 3, 5, 5, 0, 4, 5,
            5, 3, 5, 4, 5, 4, 4, 4, 4, 0, 1, 1, 4, 0, 2, 0, 0, 3, 0, 2, 2, 4,
            3, 0, 0, 0, 4, 1, 0, 1, 2, 3, 1, 3, 2, 0, 0, 0, 0, 0, 3, 5, 4, 0,
            3, 0, 0, 3, 0, 0, 0, 3, 2, 2, 0, 1, 4, 0, 3, 2, 3, 3, 0, 2, 0, 5,
            4, 0, 3, 0, 1, 4, 3, 5, 0, 0, 4, 1, 1, 0, 4, 4, 1, 3, 1, 3, 1, 5,
            1, 1, 0, 3, 5, 4, 3, 4, 4, 4, 0, 4, 4, 3, 0, 3, 5, 3
        };
        ArffFiles file;
        Discretizer* disc = new CPPFImdlp();
        file.load(data_path + "iris.arff", true);
        vector<samples_t>& X = file.getX();
        labels_t& y = file.getY();
        disc->fit(X[1], y);
        auto computed = disc->transform(X[1]);
        delete disc;
        EXPECT_EQ(computed.size(), expected.size());
        for (unsigned long i = 0; i < computed.size(); i++) {
            EXPECT_EQ(computed[i], expected[i]);
        }
    }

    TEST(Discretizer, TransformEmptyData)
    {
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);
        samples_t empty_data = {};
        EXPECT_THROW_WITH_MESSAGE(disc->transform(empty_data), std::invalid_argument, "Data for transformation cannot be empty");
        delete disc;
    }

    TEST(Discretizer, TransformNotFitted)
    {
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);
        samples_t data = { 1.0f, 2.0f, 3.0f };
        EXPECT_THROW_WITH_MESSAGE(disc->transform(data), std::runtime_error, "Discretizer not fitted yet or no valid cut points found");
        delete disc;
    }

    TEST(Discretizer, TensorValidationFit)
    {
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);

        auto X = torch::tensor({ 1.0f, 2.0f, 3.0f }, torch::kFloat32);
        auto y = torch::tensor({ 1, 2, 3 }, torch::kInt32);

        // Test non-1D tensors
        auto X_2d = torch::tensor({ {1.0f, 2.0f}, {3.0f, 4.0f} }, torch::kFloat32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_t(X_2d, y), std::invalid_argument, "Only 1D tensors supported");

        auto y_2d = torch::tensor({ {1, 2}, {3, 4} }, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_t(X, y_2d), std::invalid_argument, "Only 1D tensors supported");

        // Test wrong tensor types
        auto X_int = torch::tensor({ 1, 2, 3 }, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_t(X_int, y), std::invalid_argument, "X tensor must be Float32 type");

        auto y_float = torch::tensor({ 1.0f, 2.0f, 3.0f }, torch::kFloat32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_t(X, y_float), std::invalid_argument, "y tensor must be Int32 type");

        // Test mismatched sizes
        auto y_short = torch::tensor({ 1, 2 }, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_t(X, y_short), std::invalid_argument, "X and y tensors must have same number of elements");

        // Test empty tensors
        auto X_empty = torch::tensor({}, torch::kFloat32);
        auto y_empty = torch::tensor({}, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_t(X_empty, y_empty), std::invalid_argument, "Tensors cannot be empty");

        delete disc;
    }

    TEST(Discretizer, TensorValidationTransform)
    {
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);

        // First fit with valid data
        auto X_fit = torch::tensor({ 1.0f, 2.0f, 3.0f, 4.0f }, torch::kFloat32);
        auto y_fit = torch::tensor({ 1, 2, 3, 4 }, torch::kInt32);
        disc->fit_t(X_fit, y_fit);

        // Test non-1D tensor
        auto X_2d = torch::tensor({ {1.0f, 2.0f}, {3.0f, 4.0f} }, torch::kFloat32);
        EXPECT_THROW_WITH_MESSAGE(disc->transform_t(X_2d), std::invalid_argument, "Only 1D tensors supported");

        // Test wrong tensor type
        auto X_int = torch::tensor({ 1, 2, 3 }, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->transform_t(X_int), std::invalid_argument, "X tensor must be Float32 type");

        // Test empty tensor
        auto X_empty = torch::tensor({}, torch::kFloat32);
        EXPECT_THROW_WITH_MESSAGE(disc->transform_t(X_empty), std::invalid_argument, "Tensor cannot be empty");

        delete disc;
    }

    TEST(Discretizer, TensorValidationFitTransform)
    {
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);

        auto X = torch::tensor({ 1.0f, 2.0f, 3.0f }, torch::kFloat32);
        auto y = torch::tensor({ 1, 2, 3 }, torch::kInt32);

        // Test non-1D tensors
        auto X_2d = torch::tensor({ {1.0f, 2.0f}, {3.0f, 4.0f} }, torch::kFloat32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_transform_t(X_2d, y), std::invalid_argument, "Only 1D tensors supported");

        auto y_2d = torch::tensor({ {1, 2}, {3, 4} }, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_transform_t(X, y_2d), std::invalid_argument, "Only 1D tensors supported");

        // Test wrong tensor types
        auto X_int = torch::tensor({ 1, 2, 3 }, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_transform_t(X_int, y), std::invalid_argument, "X tensor must be Float32 type");

        auto y_float = torch::tensor({ 1.0f, 2.0f, 3.0f }, torch::kFloat32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_transform_t(X, y_float), std::invalid_argument, "y tensor must be Int32 type");

        // Test mismatched sizes
        auto y_short = torch::tensor({ 1, 2 }, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_transform_t(X, y_short), std::invalid_argument, "X and y tensors must have same number of elements");

        // Test empty tensors
        auto X_empty = torch::tensor({}, torch::kFloat32);
        auto y_empty = torch::tensor({}, torch::kInt32);
        EXPECT_THROW_WITH_MESSAGE(disc->fit_transform_t(X_empty, y_empty), std::invalid_argument, "Tensors cannot be empty");

        delete disc;
    }

    // Regression: Discretizer::direction had no initializer, so a
    // default-constructed CPPFImdlp never assigned it and transform() branched on
    // an indeterminate value. The test passed only because the garbage value
    // happened not to compare equal to bound_dir_t::LEFT.
    TEST(Discretizer, DefaultConstructedCPPFImdlpTransformsDeterministically)
    {
        samples_t X = { 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 3.0f, 3.0f, 3.0f };
        labels_t y = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };

        CPPFImdlp disc;  // default constructor: never assigns `direction`
        disc.fit(X, y);
        auto computed = disc.transform(X);

        EXPECT_EQ(computed, expected);
    }

    // Regression: fit_t/transform_t/fit_transform_t validated rank, dtype and
    // element count but not contiguity, then walked data_ptr() + numel(). A 1-D
    // non-contiguous view (a column of a 2-D dataset) was therefore read as raw
    // interleaved memory, silently yielding wrong cut points and no exception.
    TEST(Discretizer, NonContiguousTensorIsReadCorrectly)
    {
        // Column 0 holds 0..9, column 1 holds 100..109.
        auto base = torch::empty({ 10, 2 }, torch::kFloat32);
        for (int i = 0; i < 10; ++i) {
            base[i][0] = static_cast<float>(i);
            base[i][1] = static_cast<float>(100 + i);
        }
        auto col0 = base.select(1, 0);
        ASSERT_FALSE(col0.is_contiguous());
        ASSERT_EQ(col0.numel(), 10);

        auto y = torch::zeros({ 10 }, torch::kInt32);

        BinDisc disc(3, strategy_t::UNIFORM);
        disc.fit_t(col0, y);
        auto cuts = disc.getCutPoints();

        // Must be derived from the logical values 0..9, never from the raw
        // buffer 0,100,1,101,... Before the fix this produced 0, 34.67, 69.33, 104.
        ASSERT_FALSE(cuts.empty());
        EXPECT_NEAR(cuts.front(), 0.0f, margin);
        EXPECT_NEAR(cuts.back(), 9.0f, margin);

        // transform_t must handle the same non-contiguous view.
        auto transformed = disc.transform_t(col0);
        ASSERT_EQ(transformed.numel(), 10);
        auto contiguous_result = disc.transform_t(col0.contiguous());
        EXPECT_TRUE(torch::equal(transformed, contiguous_result));
    }

    // A discretizer that implements only the pure-virtual lvalue fit(). Every
    // discretizer shipped with the library overrides the rvalue overload, so this
    // is what exercises the base class's forwarding default.
    //
    // Note the `using`: declaring any fit() here would otherwise hide every fit()
    // inherited from Discretizer, rvalue overload included. Third-party subclasses
    // hit the same trap.
    class MinimalDiscretizer : public Discretizer {
    public:
        using Discretizer::fit;
        void fit(samples_t& X_, labels_t& y_) override
        {
            cutPoints = { X_.front(), X_.back() };
            fit_calls++;
        }
        int fit_calls = 0;
    };

    TEST(Discretizer, BaseRvalueFitForwardsToTheLvalueOverload)
    {
        MinimalDiscretizer disc;
        samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f };
        labels_t y = { 0, 0, 1, 1 };

        disc.fit(std::move(X), std::move(y));

        EXPECT_EQ(1, disc.fit_calls) << "the subclass's lvalue fit() should have run";
        const auto cuts = disc.getCutPoints();
        ASSERT_EQ(2u, cuts.size());
        EXPECT_NEAR(1.0f, cuts.front(), margin);
        EXPECT_NEAR(4.0f, cuts.back(), margin);
    }

    // T5.2: transform() into a caller-owned buffer.
    TEST(Discretizer, TransformIntoCallerBuffer)
    {
        samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
        labels_t y = { 0, 0, 0, 0, 1, 1, 1, 1 };

        BinDisc disc(4, strategy_t::UNIFORM);
        disc.fit(X, y);

        const labels_t expected = disc.transform(X);

        labels_t out;
        disc.transform(X, out);
        EXPECT_EQ(expected, out);

        // The buffer is reusable: a second call must not append to the first.
        disc.transform(X, out);
        EXPECT_EQ(expected, out);
        EXPECT_EQ(X.size(), out.size());

        // Writing into a caller buffer does not disturb the internal one.
        EXPECT_EQ(expected, disc.transform(X));
    }

    TEST(Discretizer, TransformIntoCallerBufferIsConst)
    {
        samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
        labels_t y = { 0, 0, 0, 0, 1, 1, 1, 1 };

        BinDisc disc(4, strategy_t::UNIFORM);
        disc.fit(X, y);
        const labels_t expected = disc.transform(X);

        const BinDisc& immutable = disc;
        labels_t out;
        immutable.transform(X, out);  // only compiles if the overload is const
        EXPECT_EQ(expected, out);
    }

    TEST(Discretizer, TransformIntoCallerBufferValidatesInput)
    {
        samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
        labels_t y = { 0, 0, 0, 0, 1, 1, 1, 1 };
        labels_t out;

        BinDisc unfitted(4, strategy_t::UNIFORM);
        EXPECT_THROW_WITH_MESSAGE(unfitted.transform(X, out), std::runtime_error,
            "Discretizer not fitted yet or no valid cut points found");

        BinDisc disc(4, strategy_t::UNIFORM);
        disc.fit(X, y);
        samples_t empty_data = {};
        EXPECT_THROW_WITH_MESSAGE(disc.transform(empty_data, out), std::invalid_argument,
            "Data for transformation cannot be empty");
    }

    TEST(Discretizer, NonContiguousTensorFitTransform)
    {
        auto base = torch::empty({ 6, 2 }, torch::kFloat32);
        for (int i = 0; i < 6; ++i) {
            base[i][0] = static_cast<float>(i);
            base[i][1] = static_cast<float>(500 + i);
        }
        auto col0 = base.select(1, 0);
        ASSERT_FALSE(col0.is_contiguous());

        auto y = torch::tensor({ 0, 0, 0, 1, 1, 1 }, torch::kInt32);

        BinDisc disc(3, strategy_t::UNIFORM);
        auto from_view = disc.fit_transform_t(col0, y);
        auto from_contiguous = disc.fit_transform_t(col0.contiguous(), y);

        EXPECT_TRUE(torch::equal(from_view, from_contiguous));
    }
}
