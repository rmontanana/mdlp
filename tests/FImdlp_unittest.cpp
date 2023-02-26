#include "gtest/gtest.h"
#include "../Metrics.h"
#include "../CPPFImdlp.h"
#include "ArffFiles.h"
#include <iostream>

namespace mdlp {
    class TestFImdlp: public CPPFImdlp, public testing::Test {
    public:
        precision_t precision = 0.000001;
        //precision_t precision = 0.000000000001;
        TestFImdlp(): CPPFImdlp() {}
        void SetUp()
        {
            X = { 4.7, 4.7, 4.7, 4.7, 4.8, 4.8, 4.8, 4.8, 4.9, 4.95, 5.7, 5.3, 5.2, 5.1, 5.0, 5.6, 5.1, 6.0, 5.1, 5.9 };
            y = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
            fit(X, y);
        }
        void checkSortedVector()
        {
            indices_t testSortedIndices = sortIndices(X, y);
            precision_t prev = X[testSortedIndices[0]];
            for (unsigned long i = 0; i < X.size(); ++i) {
                EXPECT_EQ(testSortedIndices[i], indices[i]);
                EXPECT_LE(prev, X[testSortedIndices[i]]);
                prev = X[testSortedIndices[i]];
            }
        }
        void checkCutPoints(cutPoints_t& computed, cutPoints_t& expected)
        {
            EXPECT_EQ(computed.size(), expected.size());
            for (unsigned long i = 0; i < computed.size(); i++) {
                EXPECT_NEAR(computed[i], expected[i], precision);
            }
        }
        template<typename T, typename A>
        void checkVectors(std::vector<T, A> const& expected, std::vector<T, A> const& computed)
        {
            ASSERT_EQ(expected.size(), computed.size());
            for (auto i = 0; i < expected.size(); i++) {
                EXPECT_NEAR(expected[i], computed[i], precision);
            }
        }
        bool test_result(samples_t& X_, size_t cut, float midPoint, size_t limit, string title)
        {
            pair<precision_t, size_t> result;
            labels_t y_ = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            X = X_;
            y = y_;
            indices = sortIndices(X, y);
            cout << "* " << title << endl;
            result = valueCutPoint(0, cut, 10);
            EXPECT_NEAR(result.first, midPoint, precision);
            EXPECT_EQ(result.second, limit);
            return true;
        }
        void test_dataset(CPPFImdlp& test, string filename, vector<cutPoints_t>& expected, int depths[])
        {
            ArffFiles file;
            file.load("../datasets/" + filename + ".arff", true);
            vector<samples_t>& X = file.getX();
            labels_t& y = file.getY();
            auto attributes = file.getAttributes();
            for (auto feature = 0; feature < attributes.size(); feature++) {
                test.fit(X[feature], y);
                EXPECT_EQ(test.get_depth(), depths[feature]);
                auto computed = test.getCutPoints();
                cout << "Feature " << feature << ": ";
                for (auto item : computed)
                    cout << item << " ";
                cout << endl;
                checkCutPoints(computed, expected[feature]);
            }
        }
    };
    TEST_F(TestFImdlp, FitErrorEmptyDataset)
    {
        X = samples_t();
        y = labels_t();
        EXPECT_THROW(fit(X, y), std::invalid_argument);
    }
    TEST_F(TestFImdlp, FitErrorDifferentSize)
    {
        X = { 1, 2, 3 };
        y = { 1, 2 };
        EXPECT_THROW(fit(X, y), std::invalid_argument);
    }
    TEST_F(TestFImdlp, FitErrorMinLengtMaxDepth)
    {
        auto testLength = CPPFImdlp(2, 10);
        auto testDepth = CPPFImdlp(3, 0);
        X = { 1, 2, 3 };
        y = { 1, 2, 3 };
        EXPECT_THROW(testLength.fit(X, y), invalid_argument);
        EXPECT_THROW(testDepth.fit(X, y), invalid_argument);
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
    TEST_F(TestFImdlp, TestShortDatasets)
    {
        vector<precision_t> computed;
        X = { 1 };
        y = { 1 };
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 0);
        X = { 1, 3 };
        y = { 1, 2 };
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 0);
        X = { 2, 4 };
        y = { 1, 2 };
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 0);
        X = { 1, 2, 3 };
        y = { 1, 2, 2 };
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 1);
        EXPECT_NEAR(computed[0], 1.5, precision);
    }
    TEST_F(TestFImdlp, TestArtificialDataset)
    {
        fit(X, y);
        computeCutPoints(0, 20, 1);
        cutPoints_t expected = { 5.05 };
        vector<precision_t> computed = getCutPoints();
        computed = getCutPoints();
        int expectedSize = expected.size();
        EXPECT_EQ(computed.size(), expected.size());
        for (unsigned long i = 0; i < computed.size(); i++) {
            EXPECT_NEAR(computed[i], expected[i], precision);
        }
    }
    TEST_F(TestFImdlp, TestIris)
    {
        vector<cutPoints_t> expected = {
            { 5.45, 5.75 },
            { 2.75, 2.85, 2.95, 3.05, 3.35 },
            { 2.45, 4.75, 5.05 },
            { 0.8, 1.75 }
        };
        int depths[] = { 3, 5, 5, 5 };
        auto test = CPPFImdlp();
        //test_dataset(test, "iris.arff", expected, depths);
    }
    TEST_F(TestFImdlp, ComputeCutPointsGCase)
    {
        cutPoints_t expected;
        expected = { 1.5 };
        samples_t X_ = { 0, 1, 2, 2, 2 };
        labels_t y_ = { 1, 1, 1, 2, 2 };
        fit(X_, y_);
        auto computed = getCutPoints();
        checkCutPoints(computed, expected);
    }
    TEST_F(TestFImdlp, ValueCutPoint)
    {
        // Case titles as stated in the doc
        samples_t X1a{ 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0 };
        test_result(X1a, 6, 7.3 / 2, 6, "1a");
        samples_t X2a = { 3.1, 3.2, 3.3, 3.4, 3.7, 3.7, 3.7, 3.8, 3.9, 4.0 };
        test_result(X2a, 6, 7.1 / 2, 4, "2a");
        samples_t X2b = { 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.8, 3.9, 4.0 };
        test_result(X2b, 6, 7.5 / 2, 7, "2b");
        samples_t X3a = { 3.1, 3.2, 3.3, 3.4, 3.7, 3.7, 3.7, 3.8, 3.9, 4.0 };
        test_result(X3a, 4, 7.1 / 2, 4, "3a");
        samples_t X3b = { 3.1, 3.2, 3.3, 3.4, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7 };
        test_result(X3b, 4, 7.1 / 2, 4, "3b");
        samples_t X4a = { 3.1, 3.2, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.9, 4.0 };
        test_result(X4a, 4, 6.9 / 2, 2, "4a");
        samples_t X4b = { 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.8, 3.9, 4.0 };
        test_result(X4b, 4, 7.5 / 2, 7, "4b");
        samples_t X4c = { 3.1, 3.2, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7, 3.7 };
        test_result(X4c, 4, 6.9 / 2, 2, "4c");
    }
    TEST_F(TestFImdlp, MaxDepth)
    {
        // Set max_depth to 2
        auto test = CPPFImdlp(3, 1);
        vector<cutPoints_t> expected = {
            { 5.45 },
            { 3.35 },
            { 2.45 },
            {0.8 }
        };
        int depths[] = { 1, 1, 1, 1 };
        test_dataset(test, "iris", expected, depths);
    }
    TEST_F(TestFImdlp, MinLength)
    {
        auto test = CPPFImdlp(75, 100);
        // Set min_length to 75
        vector<cutPoints_t> expected = {
            { 5.45, 5.75 },
            { 2.85, 3.35 },
            { 2.45, 4.75 },
            { 0.8, 1.75 }
        };
        int depths[] = { 3, 2, 2, 2 };
        test_dataset(test, "iris", expected, depths);
    }
    TEST_F(TestFImdlp, MinLengthMaxDepth)
    {
        // Set min_length to 75
        auto test = CPPFImdlp(75, 2);
        vector<cutPoints_t> expected = {
            { 5.45, 5.75 },
            { 2.85, 3.35 },
            { 2.45, 4.75 },
            { 0.8, 1.75 }
        };
        int depths[] = { 2, 2, 2, 2 };
        test_dataset(test, "iris", expected, depths);
    }
}
