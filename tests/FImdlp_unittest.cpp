#include "gtest/gtest.h"
#include "../Metrics.h"
#include "../CPPFImdlp.h"
#include "ArffFiles.h"
#include <iostream>

namespace mdlp {
    class TestFImdlp: public CPPFImdlp, public testing::Test {
    public:
        precision_t precision = 0.000001;
        TestFImdlp(): CPPFImdlp() {}
        void SetUp()
        {
            X = { 4.7, 4.7, 4.7, 4.7, 4.8, 4.8, 4.8, 4.8, 4.9, 4.95, 5.7, 5.3, 5.2, 5.1, 5.0, 5.6, 5.1, 6.0, 5.1, 5.9 };
            y = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
            algorithm = false;
            fit(X, y);
        }
        void setalgorithm(bool value)
        {
            algorithm = value;
        }
        void checkSortedVector()
        {
            indices_t testSortedIndices = sortIndices(X, y);
            precision_t prev = X[testSortedIndices[0]];
            for (auto i = 0; i < X.size(); ++i) {
                EXPECT_EQ(testSortedIndices[i], indices[i]);
                EXPECT_LE(prev, X[testSortedIndices[i]]);
                prev = X[testSortedIndices[i]];
            }
        }
        void checkCutPoints(cutPoints_t& expected)
        {
            int expectedSize = expected.size();
            EXPECT_EQ(cutPoints.size(), expectedSize);
            for (auto i = 0; i < cutPoints.size(); i++) {
                EXPECT_NEAR(cutPoints[i], expected[i], precision);
            }
        }
        template<typename T, typename A>
        void checkVectors(std::vector<T, A> const& expected, std::vector<T, A> const& computed)
        {
            EXPECT_EQ(expected.size(), computed.size());
            ASSERT_EQ(expected.size(), computed.size());
            for (auto i = 0; i < expected.size(); i++) {
                EXPECT_NEAR(expected[i], computed[i], precision);
            }
        }
    };
    TEST_F(TestFImdlp, FitErrorEmptyDataset)
    {
        X = samples_t();
        y = labels_t();
        EXPECT_THROW(fit(X, y), std::invalid_argument);
    }
    TEST_F(TestFImdlp, FitErrorIncorrectAlgorithm)
    {
        algorithm = 2;
        EXPECT_THROW(fit(X, y), std::invalid_argument);
    }
    TEST_F(TestFImdlp, FitErrorDifferentSize)
    {
        X = { 1, 2, 3 };
        y = { 1, 2 };
        EXPECT_THROW(fit(X, y), std::invalid_argument);
    }
    TEST_F(TestFImdlp, SortIndices)
    {
        X = { 5.7, 5.3, 5.2, 5.1, 5.0, 5.6, 5.1, 6.0, 5.1, 5.9 };
        y = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
        indices = { 4, 3, 6, 8, 2, 1, 5, 0, 9, 7 };
        checkSortedVector();
        X = { 5.77, 5.88, 5.99 };
        y = { 1, 2, 1 };
        indices = { 0, 1, 2 };
        checkSortedVector();
        X = { 5.33, 5.22, 5.11 };
        y = { 1, 2, 1 };
        indices = { 2, 1, 0 };
        checkSortedVector();
        X = { 5.33, 5.22, 5.33 };
        y = { 2, 2, 1 };
        indices = { 1, 2, 0 };
    }
    TEST_F(TestFImdlp, TestArtificialDatasetAlternative)
    {
        algorithm = 1;
        fit(X, y);
        computeCutPoints(0, 20);
        cutPoints_t expected = { 5.0500001907348633 };
        vector<precision_t> computed = getCutPoints();
        computed = getCutPoints();
        int expectedSize = expected.size();
        EXPECT_EQ(computed.size(), expected.size());
        for (auto i = 0; i < computed.size(); i++) {
            EXPECT_NEAR(computed[i], expected[i], precision);
        }
    }
    TEST_F(TestFImdlp, TestArtificialDataset)
    {
        algorithm = 0;
        fit(X, y);
        computeCutPoints(0, 20);
        cutPoints_t expected = { 5.0500001907348633 };
        vector<precision_t> computed = getCutPoints();
        computed = getCutPoints();
        int expectedSize = expected.size();
        EXPECT_EQ(computed.size(), expected.size());
        for (auto i = 0; i < computed.size(); i++) {
            EXPECT_NEAR(computed[i], expected[i], precision);
        }
    }
    TEST_F(TestFImdlp, TestIris)
    {
        ArffFiles file;
        string path = "../datasets/";

        file.load(path + "iris.arff", true);
        int items = file.getSize();
        vector<samples_t>& X = file.getX();
        vector<cutPoints_t> expected = {
            { 5.4499998092651367, 6.25 },
            { 2.8499999046325684, 3, 3.0499999523162842, 3.3499999046325684 },
            { 2.4500000476837158, 4.75, 5.0500001907348633 },
            { 0.80000001192092896, 1.4500000476837158, 1.75 }
        };
        labels_t& y = file.getY();
        auto attributes = file.getAttributes();
        algorithm = 0;
        for (auto feature = 0; feature < attributes.size(); feature++) {
            fit(X[feature], y);
            vector<precision_t> computed = getCutPoints();
            EXPECT_EQ(computed.size(), expected[feature].size());
            for (auto i = 0; i < computed.size(); i++) {
                EXPECT_NEAR(computed[i], expected[feature][i], precision);
            }
        }
    }
    TEST_F(TestFImdlp, TestIrisAlternative)
    {
        ArffFiles file;
        string path = "../datasets/";

        file.load(path + "iris.arff", true);
        int items = file.getSize();
        vector<samples_t>& X = file.getX();
        vector<cutPoints_t> expected = {
            { 5.4499998092651367, 5.75 },
            { 2.8499999046325684, 3.3499999046325684 },
            { 2.4500000476837158, 4.75 },
            { 0.80000001192092896, 1.75 }
        };
        labels_t& y = file.getY();
        auto attributes = file.getAttributes();
        algorithm = 1;
        for (auto feature = 0; feature < attributes.size(); feature++) {
            fit(X[feature], y);
            vector<precision_t> computed = getCutPoints();
            EXPECT_EQ(computed.size(), expected[feature].size());
            for (auto i = 0; i < computed.size(); i++) {
                EXPECT_NEAR(computed[i], expected[feature][i], precision);
            }
        }
    }
    TEST_F(TestFImdlp, ComputeCutPointsGCase)
    {
        cutPoints_t expected;
        algorithm = 0;
        expected = { 1.5 };
        samples_t X_ = { 0, 1, 2, 2 };
        labels_t y_ = { 1, 1, 1, 2 };
        fit(X_, y_);
        checkCutPoints(expected);
    }
    TEST_F(TestFImdlp, ComputeCutPointsAlternativeGCase)
    {
        cutPoints_t expected;
        expected = { 1.5 };
        algorithm = true;
        samples_t X_ = { 0, 1, 2, 2 };
        labels_t y_ = { 1, 1, 1, 2 };
        fit(X_, y_);
        checkCutPoints(expected);
    }
}
