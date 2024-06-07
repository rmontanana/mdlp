#include <fstream>
#include <string>
#include <iostream>
#include "gtest/gtest.h"
#include "ArffFiles.h"
#include "../BinDisc.h"

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
    TEST_F(TestBinDisc3U, Easy3BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
        auto y = labels_t();
        fit(X, y);
        auto cuts = getCutPoints();
        ASSERT_EQ(3, cuts.size());
        EXPECT_NEAR(3.66667, cuts.at(0), margin);
        EXPECT_NEAR(6.33333, cuts.at(1), margin);
        EXPECT_EQ(numeric_limits<float>::max(), cuts.at(2));
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3Q, Easy3BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(3, cuts.size());
        EXPECT_NEAR(3.666667, cuts[0], margin);
        EXPECT_NEAR(6.333333, cuts[1], margin);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[2]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, X10BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(3, cuts.size());
        EXPECT_EQ(4.0, cuts[0]);
        EXPECT_EQ(7.0, cuts[1]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[2]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3Q, X10BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(3, cuts.size());
        EXPECT_EQ(4, cuts[0]);
        EXPECT_EQ(7, cuts[1]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[2]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, X11BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(3, cuts.size());
        EXPECT_NEAR(4.33333, cuts[0], margin);
        EXPECT_NEAR(7.66667, cuts[1], margin);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[2]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, X11BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(3, cuts.size());
        EXPECT_NEAR(4.33333, cuts[0], margin);
        EXPECT_NEAR(7.66667, cuts[1], margin);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[2]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, ConstantUniform)
    {
        samples_t X = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
        fit(X);
        auto cuts = getCutPoints();
        ASSERT_EQ(1, cuts.size());
        EXPECT_EQ(numeric_limits<float>::max(), cuts[0]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 0, 0 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3Q, ConstantQuantile)
    {
        samples_t X = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(1, cuts.size());
        EXPECT_EQ(numeric_limits<float>::max(), cuts[0]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 0, 0 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc3U, EmptyUniform)
    {
        samples_t X = {};
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(1, cuts.size());
        EXPECT_EQ(numeric_limits<float>::max(), cuts[0]);
    }
    TEST_F(TestBinDisc3Q, EmptyQuantile)
    {
        samples_t X = {};
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(1, cuts.size());
        EXPECT_EQ(numeric_limits<float>::max(), cuts[0]);
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
        ASSERT_EQ(3, cuts.size());
        EXPECT_NEAR(1.66667, cuts[0], margin);
        EXPECT_NEAR(2.33333, cuts[1], margin);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[2]);
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
        EXPECT_EQ(2, cuts.size());
        EXPECT_NEAR(1.66667, cuts[0], margin);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[1]);
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
        EXPECT_EQ(4, cuts.size());
        ASSERT_EQ(3.75, cuts[0]);
        EXPECT_EQ(6.5, cuts[1]);
        EXPECT_EQ(9.25, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, Easy4BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(4, cuts.size());
        ASSERT_EQ(3.75, cuts[0]);
        EXPECT_EQ(6.5, cuts[1]);
        EXPECT_EQ(9.25, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, X13BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(4, cuts.size());
        EXPECT_EQ(4.0, cuts[0]);
        EXPECT_EQ(7.0, cuts[1]);
        EXPECT_EQ(10.0, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, X13BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(4, cuts.size());
        EXPECT_EQ(4.0, cuts[0]);
        EXPECT_EQ(7.0, cuts[1]);
        EXPECT_EQ(10.0, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, X14BinsUniform)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(4, cuts.size());
        EXPECT_EQ(4.25, cuts[0]);
        EXPECT_EQ(7.5, cuts[1]);
        EXPECT_EQ(10.75, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, X14BinsQuantile)
    {
        samples_t X = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(4, cuts.size());
        EXPECT_EQ(4.25, cuts[0]);
        EXPECT_EQ(7.5, cuts[1]);
        EXPECT_EQ(10.75, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, X15BinsUniform)
    {
        samples_t X = { 15.0, 8.0, 12.0, 14.0, 6.0, 1.0, 13.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(4, cuts.size());
        EXPECT_EQ(4.5, cuts[0]);
        EXPECT_EQ(8, cuts[1]);
        EXPECT_EQ(11.5, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
        auto labels = transform(X);
        labels_t expected = { 3, 2, 3, 3, 1, 0, 3, 2, 2, 2, 1, 0, 0, 1, 0 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4Q, X15BinsQuantile)
    {
        samples_t X = { 15.0, 13.0, 12.0, 14.0, 6.0, 1.0, 8.0, 11.0, 10.0, 9.0, 7.0, 4.0, 3.0, 5.0, 2.0 };
        fit(X);
        auto cuts = getCutPoints();
        EXPECT_EQ(4, cuts.size());
        EXPECT_EQ(4.5, cuts[0]);
        EXPECT_EQ(8, cuts[1]);
        EXPECT_EQ(11.5, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
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
        EXPECT_EQ(4, cuts.size());
        EXPECT_EQ(1.0, cuts[0]);
        EXPECT_EQ(2.0, cuts[1]);
        ASSERT_EQ(3.0, cuts[2]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[3]);
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
        ASSERT_EQ(3, cuts.size());
        EXPECT_EQ(2.0, cuts[0]);
        ASSERT_EQ(3.0, cuts[1]);
        EXPECT_EQ(numeric_limits<float>::max(), cuts[2]);
        auto labels = transform(X);
        labels_t expected = { 0, 0, 0, 0, 1, 1, 2, 2, 2, 2 };
        EXPECT_EQ(expected, labels);
    }
    TEST_F(TestBinDisc4U, irisUniform)
    {
        ArffFiles file;
        file.load(data_path + "iris.arff", true);
        vector<samples_t>& X = file.getX();
        fit(X[0]);
        auto Xt = transform(X[0]);
        labels_t expected = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 2, 2, 1, 2, 1, 2, 0, 2, 0, 0, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 0, 1, 2, 1, 3, 2, 2, 3, 0, 3, 2, 3, 2, 2, 2, 1, 1, 2, 2, 3, 3, 1, 2, 1, 3, 2, 2, 3, 2, 1, 2, 3, 3, 3, 2, 2, 1, 3, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };
        EXPECT_EQ(expected, Xt);
        auto Xtt = fit_transform(X[0], file.getY());
        EXPECT_EQ(expected, Xtt);
        auto Xt_t = torch::tensor(X[0], torch::kFloat32);
        auto y_t = torch::tensor(file.getY(), torch::kInt64);
        auto Xtt_t = fit_transform_t(Xt_t, y_t);
        for (int i = 0; i < expected.size(); i++)
            EXPECT_EQ(expected[i], Xtt_t[i].item<int64_t>());
    }
    TEST_F(TestBinDisc4Q, irisQuantile)
    {
        ArffFiles file;
        file.load(data_path + "iris.arff", true);
        vector<samples_t>& X = file.getX();
        fit(X[0]);
        auto Xt = transform(X[0]);
        labels_t expected = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 3, 3, 3, 1, 3, 1, 2, 0, 3, 1, 0, 2, 2, 2, 1, 3, 1, 2, 2, 1, 2, 2, 2, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2, 1, 2, 3, 2, 1, 1, 1, 2, 2, 0, 1, 1, 1, 2, 1, 1, 2, 2, 3, 2, 3, 3, 0, 3, 3, 3, 3, 3, 3, 1, 2, 3, 3, 3, 3, 2, 3, 1, 3, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 2, 2 };
        EXPECT_EQ(expected, Xt);
        auto Xtt = fit_transform(X[0], file.getY());
        EXPECT_EQ(expected, Xtt);
        auto Xt_t = torch::tensor(X[0], torch::kFloat32);
        auto y_t = torch::tensor(file.getY(), torch::kInt64);
        auto Xtt_t = fit_transform_t(Xt_t, y_t);
        for (int i = 0; i < expected.size(); i++)
            EXPECT_EQ(expected[i], Xtt_t[i].item<int64_t>());
        fit_t(Xt_t, y_t);
        auto Xt_t2 = transform_t(Xt_t);
        for (int i = 0; i < expected.size(); i++)
            EXPECT_EQ(expected[i], Xt_t2[i].item<int64_t>());
    }
}
