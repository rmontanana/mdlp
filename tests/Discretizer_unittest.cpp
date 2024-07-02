#include <fstream>
#include <string>
#include <iostream>
#include "gtest/gtest.h"
#include "ArffFiles.h"
#include "../Discretizer.h"
#include "../BinDisc.h"
#include "../CPPFImdlp.h"

namespace mdlp {
    const float margin = 1e-4;
    static std::string set_data_path()
    {
        std::string path = "../datasets/";
        std::ifstream file(path + "iris.arff");
        if (file.is_open()) {
            file.close();
            return path;
        }
        return "../../tests/datasets/";
    }
    const std::string data_path = set_data_path();

    TEST(Discretizer, Version)
    {
        Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);
        auto version = disc->version();
        delete disc;
        std::cout << "Version computed: " << version;
        EXPECT_EQ("1.2.3", version);
    }

    // TEST(Discretizer, BinIrisUniform)
    // {
    //     ArffFiles file;
    //     Discretizer* disc = new BinDisc(4, strategy_t::UNIFORM);
    //     file.load(data_path + "iris.arff", true);
    //     vector<samples_t>& X = file.getX();
    //     auto y = labels_t();
    //     disc->fit(X[0], y);
    //     auto Xt = disc->transform(X[0]);
    //     labels_t expected = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 2, 2, 1, 2, 1, 2, 0, 2, 0, 0, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 0, 1, 2, 1, 3, 2, 2, 3, 0, 3, 2, 3, 2, 2, 2, 1, 1, 2, 2, 3, 3, 1, 2, 1, 3, 2, 2, 3, 2, 1, 2, 3, 3, 3, 2, 2, 1, 3, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };
    //     delete disc;
    //     EXPECT_EQ(expected, Xt);
    // }
    // TEST(Discretizer, BinIrisQuantile)
    // {
    //     ArffFiles file;
    //     Discretizer* disc = new BinDisc(4, strategy_t::QUANTILE);
    //     file.load(data_path + "iris.arff", true);
    //     vector<samples_t>& X = file.getX();
    //     auto y = labels_t();
    //     disc->fit(X[0], y);
    //     auto Xt = disc->transform(X[0]);
    //     labels_t expected = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 3, 3, 3, 1, 3, 1, 2, 0, 3, 1, 0, 2, 2, 2, 1, 3, 1, 2, 2, 1, 2, 2, 2, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2, 1, 2, 3, 2, 1, 1, 1, 2, 2, 0, 1, 1, 1, 2, 1, 1, 2, 2, 3, 2, 3, 3, 0, 3, 3, 3, 3, 3, 3, 1, 2, 3, 3, 3, 3, 2, 3, 1, 3, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 2, 2 };
    //     delete disc;
    //     EXPECT_EQ(expected, Xt);
    // }

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
}
