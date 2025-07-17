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
        EXPECT_EQ("2.1.1", version);
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
}
