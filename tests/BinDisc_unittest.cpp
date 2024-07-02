#include <fstream>
#include <string>
#include <iostream>
#include "gtest/gtest.h"
#include "ArffFiles.h"
#include "../BinDisc.h"
#include "Experiments.hpp"

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
    // TEST_F(TestBinDisc3U, Easy3BinsUniform)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
    //     auto y = labels_t();
    //     fit(X, y);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(4, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(3.66667, cuts.at(1), margin);
    //     EXPECT_NEAR(6.33333, cuts.at(2), margin);
    //     EXPECT_NEAR(9.0, cuts.at(3), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3Q, Easy3BinsQuantile)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(4, cuts.size());
    //     EXPECT_NEAR(1, cuts[0], margin);
    //     EXPECT_NEAR(3.666667, cuts[1], margin);
    //     EXPECT_NEAR(6.333333, cuts[2], margin);
    //     EXPECT_NEAR(9, cuts[3], margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3U, X10BinsUniform)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(4, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(4.0, cuts.at(1), margin);
    //     EXPECT_NEAR(7.0, cuts.at(2), margin);
    //     EXPECT_NEAR(10.0, cuts.at(3), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3Q, X10BinsQuantile)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(4, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(4.0, cuts.at(1), margin);
    //     EXPECT_NEAR(7.0, cuts.at(2), margin);
    //     EXPECT_NEAR(10.0, cuts.at(3), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3U, X11BinsUniform)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(4, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(4.33333, cuts.at(1), margin);
    //     EXPECT_NEAR(7.66667, cuts.at(2), margin);
    //     EXPECT_NEAR(11.0, cuts.at(3), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3U, X11BinsQuantile)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(4, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(4.33333, cuts.at(1), margin);
    //     EXPECT_NEAR(7.66667, cuts.at(2), margin);
    //     EXPECT_NEAR(11.0, cuts.at(3), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3U, ConstantUniform)
    // {
    //     samples_t X = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(2, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(1, cuts.at(1), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 0, 0 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3Q, ConstantQuantile)
    // {
    //     samples_t X = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(2, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(1, cuts.at(1), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 0, 0 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc3U, EmptyUniform)
    // {
    //     samples_t X = {};
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(2, cuts.size());
    //     EXPECT_NEAR(0, cuts.at(0), margin);
    //     EXPECT_NEAR(0, cuts.at(1), margin);
    // }
    // TEST_F(TestBinDisc3Q, EmptyQuantile)
    // {
    //     samples_t X = {};
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(2, cuts.size());
    //     EXPECT_NEAR(0, cuts.at(0), margin);
    //     EXPECT_NEAR(0, cuts.at(1), margin);
    // }
    // TEST(TestBinDisc3, ExceptionNumberBins)
    // {
    //     EXPECT_THROW(BinDisc(2), std::invalid_argument);
    // }
    // TEST_F(TestBinDisc3U, EasyRepeated)
    // {
    //     samples_t X = { 3.0, 1.0, 1.0, 3.0, 1.0, 1.0, 3.0, 1.0, 1.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(4, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(1.66667, cuts.at(1), margin);
    //     EXPECT_NEAR(2.33333, cuts.at(2), margin);
    //     EXPECT_NEAR(3.0, cuts.at(3), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 2, 0, 0, 2, 0, 0, 2, 0, 0 };
    //     EXPECT_EQ(expected, labels);
    //     ASSERT_EQ(3.0, X[0]); // X is not modified
    // }
    // TEST_F(TestBinDisc3Q, EasyRepeated)
    // {
    //     samples_t X = { 3.0, 1.0, 1.0, 3.0, 1.0, 1.0, 3.0, 1.0, 1.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(3, cuts.size());
    //     EXPECT_NEAR(1, cuts.at(0), margin);
    //     EXPECT_NEAR(1.66667, cuts.at(1), margin);
    //     EXPECT_NEAR(3.0, cuts.at(2), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 1, 0, 0, 1, 0, 0, 1, 0, 0 };
    //     EXPECT_EQ(expected, labels);
    //     ASSERT_EQ(3.0, X[0]); // X is not modified
    // }
    // TEST_F(TestBinDisc4U, Easy4BinsUniform)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(3.75, cuts.at(1), margin);
    //     EXPECT_NEAR(6.5, cuts.at(2), margin);
    //     EXPECT_NEAR(9.25, cuts.at(3), margin);
    //     EXPECT_NEAR(12.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4Q, Easy4BinsQuantile)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(3.75, cuts.at(1), margin);
    //     EXPECT_NEAR(6.5, cuts.at(2), margin);
    //     EXPECT_NEAR(9.25, cuts.at(3), margin);
    //     EXPECT_NEAR(12.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4U, X13BinsUniform)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(4.0, cuts.at(1), margin);
    //     EXPECT_NEAR(7.0, cuts.at(2), margin);
    //     EXPECT_NEAR(10.0, cuts.at(3), margin);
    //     EXPECT_NEAR(13.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4Q, X13BinsQuantile)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(4.0, cuts.at(1), margin);
    //     EXPECT_NEAR(7.0, cuts.at(2), margin);
    //     EXPECT_NEAR(10.0, cuts.at(3), margin);
    //     EXPECT_NEAR(13.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4U, X14BinsUniform)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(4.25, cuts.at(1), margin);
    //     EXPECT_NEAR(7.5, cuts.at(2), margin);
    //     EXPECT_NEAR(10.75, cuts.at(3), margin);
    //     EXPECT_NEAR(14.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4Q, X14BinsQuantile)
    // {
    //     samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(4.25, cuts.at(1), margin);
    //     EXPECT_NEAR(7.5, cuts.at(2), margin);
    //     EXPECT_NEAR(10.75, cuts.at(3), margin);
    //     EXPECT_NEAR(14.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4U, X15BinsUniform)
    // {
    //     samples_t X = { 15.0, 8.0, 12.0, 14.0, 6.0, 1.0, 13.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(4.5, cuts.at(1), margin);
    //     EXPECT_NEAR(8, cuts.at(2), margin);
    //     EXPECT_NEAR(11.5, cuts.at(3), margin);
    //     EXPECT_NEAR(15.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 3, 1, 3, 3, 1, 0, 3, 2, 2, 2, 1, 0, 0, 1, 0 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4Q, X15BinsQuantile)
    // {
    //     samples_t X = { 15.0, 13.0, 12.0, 14.0, 6.0, 1.0, 8.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0 };
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(1.0, cuts.at(0), margin);
    //     EXPECT_NEAR(4.5, cuts.at(1), margin);
    //     EXPECT_NEAR(8, cuts.at(2), margin);
    //     EXPECT_NEAR(11.5, cuts.at(3), margin);
    //     EXPECT_NEAR(15.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 3, 3, 3, 3, 1, 0, 1, 2, 2, 2, 1, 0, 0, 1, 0 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4U, RepeatedValuesUniform)
    // {
    //     samples_t X = { 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0 };
    //     //               0    1     2   3    4    5    6    7    8    9
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(0.0, cuts.at(0), margin);
    //     EXPECT_NEAR(1.0, cuts.at(1), margin);
    //     EXPECT_NEAR(2.0, cuts.at(2), margin);
    //     EXPECT_NEAR(3.0, cuts.at(3), margin);
    //     EXPECT_NEAR(4.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 2, 2, 2, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    // TEST_F(TestBinDisc4Q, RepeatedValuesQuantile)
    // {
    //     samples_t X = { 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0 };
    //     //               0    1     2   3    4    5    6    7    8    9
    //     fit(X);
    //     auto cuts = getCutPoints();
    //     ASSERT_EQ(5, cuts.size());
    //     EXPECT_NEAR(0.0, cuts.at(0), margin);
    //     EXPECT_NEAR(1.0, cuts.at(1), margin);
    //     EXPECT_NEAR(2.0, cuts.at(2), margin);
    //     EXPECT_NEAR(3.0, cuts.at(3), margin);
    //     EXPECT_NEAR(4.0, cuts.at(4), margin);
    //     auto labels = transform(X);
    //     labels_t expected = { 0, 0, 0, 0, 1, 1, 2, 2, 2, 3 };
    //     EXPECT_EQ(expected, labels);
    // }
    TEST(TestBinDiscGeneric, Fileset)
    {
        Experiments exps(data_path + "tests.txt");
        int num = 0;
        while (exps.is_next()) {
            ++num;
            Experiment exp = exps.next();
            BinDisc disc(exp.n_bins_, exp.strategy_[0] == 'Q' ? strategy_t::QUANTILE : strategy_t::UNIFORM);
            std::vector<float> test;
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
            for (int i = 0; i < exp.discretized_data_.size(); ++i) {
                if (exp.discretized_data_.at(i) != Xt.at(i)) {
                    if (!flag) {
                        std::cout << "Exp #: " << num << " From: " << exp.from_ << " To: " << exp.to_ << " Step: " << exp.step_ << " Bins: " << exp.n_bins_ << " Strategy: " << exp.strategy_ << std::endl;
                        std::cout << "Error at " << i << " Expected: " << exp.discretized_data_.at(i) << " Got: " << Xt.at(i) << std::endl;
                        flag = true;
                        EXPECT_EQ(exp.discretized_data_.at(i), Xt.at(i));
                    }
                    n_errors++;
                }
            }
            if (flag) {
                std::cout << "*** Found " << n_errors << " mistakes in this experiment dataset" << std::endl;
            }
            EXPECT_EQ(exp.cutpoints_.size(), cuts.size());
            for (int i = 0; i < exp.cutpoints_.size(); ++i) {
                EXPECT_NEAR(exp.cutpoints_.at(i), cuts.at(i), margin);
            }
        }
        std::cout << "* Number of experiments tested: " << num << std::endl;
    }
}
