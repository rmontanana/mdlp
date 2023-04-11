#include "gtest/gtest.h"
#include "../Metrics.h"
#include "../CPPFImdlp.h"
#include <fstream>
#include <iostream>
#include "ArffFiles.h"

#define EXPECT_THROW_WITH_MESSAGE(stmt, etype, whatstring) EXPECT_THROW( \
try { \
stmt; \
} catch (const etype& ex) { \
EXPECT_EQ(whatstring, std::string(ex.what())); \
throw; \
} \
, etype)

namespace mdlp {
    class TestFImdlp : public CPPFImdlp, public testing::Test {
    public:
        precision_t precision = 0.000001f;

        TestFImdlp() : CPPFImdlp() {}

        string data_path;

        void SetUp() override {
            X = {4.7f, 4.7f, 4.7f, 4.7f, 4.8f, 4.8f, 4.8f, 4.8f, 4.9f, 4.95f, 5.7f, 5.3f, 5.2f, 5.1f, 5.0f, 5.6f, 5.1f,
                 6.0f, 5.1f, 5.9f};
            y = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2};
            fit(X, y);
            data_path = set_data_path();
        }

        static string set_data_path() {
            string path = "../datasets/";
            ifstream file(path + "iris.arff");
            if (file.is_open()) {
                file.close();
                return path;
            }
            return "../../tests/datasets/";
        }

        void checkSortedVector() {
            indices_t testSortedIndices = sortIndices(X, y);
            precision_t prev = X[testSortedIndices[0]];
            for (unsigned long i = 0; i < X.size(); ++i) {
                EXPECT_EQ(testSortedIndices[i], indices[i]);
                EXPECT_LE(prev, X[testSortedIndices[i]]);
                prev = X[testSortedIndices[i]];
            }
        }

        void checkCutPoints(cutPoints_t &computed, cutPoints_t &expected) const {
            EXPECT_EQ(computed.size(), expected.size());
            for (unsigned long i = 0; i < computed.size(); i++) {
                cout << "(" << computed[i] << ", " << expected[i] << ") ";
                EXPECT_NEAR(computed[i], expected[i], precision);
            }
        }

        bool test_result(const samples_t &X_, size_t cut, float midPoint, size_t limit, const string &title) {
            pair<precision_t, size_t> result;
            labels_t y_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            X = X_;
            y = y_;
            indices = sortIndices(X, y);
            cout << "* " << title << endl;
            result = valueCutPoint(0, cut, 10);
            EXPECT_NEAR(result.first, midPoint, precision);
            EXPECT_EQ(result.second, limit);
            return true;
        }

        void test_dataset(CPPFImdlp &test, const string &filename, vector<cutPoints_t> &expected,
                          vector<int> &depths) const {
            ArffFiles file;
            file.load(data_path + filename + ".arff", true);
            vector<samples_t> &X = file.getX();
            labels_t &y = file.getY();
            auto attributes = file.getAttributes();
            for (auto feature = 0; feature < attributes.size(); feature++) {
                test.fit(X[feature], y);
                EXPECT_EQ(test.get_depth(), depths[feature]);
                auto computed = test.getCutPoints();
                cout << "Feature " << feature << ": ";
                checkCutPoints(computed, expected[feature]);
                cout << endl;
            }
        }
    };

    TEST_F(TestFImdlp, FitErrorEmptyDataset) {
        X = samples_t();
        y = labels_t();
        EXPECT_THROW_WITH_MESSAGE(fit(X, y), invalid_argument, "X and y must have at least one element");
    }

    TEST_F(TestFImdlp, FitErrorDifferentSize) {
        X = {1, 2, 3};
        y = {1, 2};
        EXPECT_THROW_WITH_MESSAGE(fit(X, y), invalid_argument, "X and y must have the same size");
    }

    TEST_F(TestFImdlp, FitErrorMinLengtMaxDepth) {
        auto testLength = CPPFImdlp(2, 10, 0);
        auto testDepth = CPPFImdlp(3, 0, 0);
        X = {1, 2, 3};
        y = {1, 2, 3};
        EXPECT_THROW_WITH_MESSAGE(testLength.fit(X, y), invalid_argument, "min_length must be greater than 2");
        EXPECT_THROW_WITH_MESSAGE(testDepth.fit(X, y), invalid_argument, "max_depth must be greater than 0");
    }

    TEST_F(TestFImdlp, JoinFit) {
        samples_t X_ = {1, 2, 2, 3, 4, 2, 3};
        labels_t y_ = {0, 0, 1, 2, 3, 4, 5};
        cutPoints_t expected = {1.5f, 2.5f};
        fit(X_, y_);
        auto computed = getCutPoints();
        EXPECT_EQ(computed.size(), expected.size());
        checkCutPoints(computed, expected);
    }

    TEST_F(TestFImdlp, FitErrorMaxCutPoints) {
        auto testmin = CPPFImdlp(2, 10, -1);
        auto testmax = CPPFImdlp(3, 0, 200);
        X = {1, 2, 3};
        y = {1, 2, 3};
        EXPECT_THROW_WITH_MESSAGE(testmin.fit(X, y), invalid_argument, "wrong proposed num_cuts value");
        EXPECT_THROW_WITH_MESSAGE(testmax.fit(X, y), invalid_argument, "wrong proposed num_cuts value");
    }

    TEST_F(TestFImdlp, SortIndices) {
        X = {5.7f, 5.3f, 5.2f, 5.1f, 5.0f, 5.6f, 5.1f, 6.0f, 5.1f, 5.9f};
        y = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2};
        indices = {4, 3, 6, 8, 2, 1, 5, 0, 9, 7};
        checkSortedVector();
        X = {5.77f, 5.88f, 5.99f};
        y = {1, 2, 1};
        indices = {0, 1, 2};
        checkSortedVector();
        X = {5.33f, 5.22f, 5.11f};
        y = {1, 2, 1};
        indices = {2, 1, 0};
        checkSortedVector();
        X = {5.33f, 5.22f, 5.33f};
        y = {2, 2, 1};
        indices = {1, 2, 0};
    }

    TEST_F(TestFImdlp, TestShortDatasets) {
        vector<precision_t> computed;
        X = {1};
        y = {1};
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 0);
        X = {1, 3};
        y = {1, 2};
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 0);
        X = {2, 4};
        y = {1, 2};
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 0);
        X = {1, 2, 3};
        y = {1, 2, 2};
        fit(X, y);
        computed = getCutPoints();
        EXPECT_EQ(computed.size(), 1);
        EXPECT_NEAR(computed[0], 1.5, precision);
    }

    TEST_F(TestFImdlp, TestArtificialDataset) {
        fit(X, y);
        cutPoints_t expected = {5.05f};
        vector<precision_t> computed = getCutPoints();
        EXPECT_EQ(computed.size(), expected.size());
        for (unsigned long i = 0; i < computed.size(); i++) {
            EXPECT_NEAR(computed[i], expected[i], precision);
        }
    }

    TEST_F(TestFImdlp, TestIris) {
        vector<cutPoints_t> expected = {
                {5.45f, 5.75f},
                {2.75f, 2.85f, 2.95f, 3.05f, 3.35f},
                {2.45f, 4.75f, 5.05f},
                {0.8f,  1.75f}
        };
        vector<int> depths = {3, 5, 4, 3};
        auto test = CPPFImdlp();
        test_dataset(test, "iris", expected, depths);
    }

    TEST_F(TestFImdlp, ComputeCutPointsGCase) {
        cutPoints_t expected;
        expected = {1.5};
        samples_t X_ = {0, 1, 2, 2, 2};
        labels_t y_ = {1, 1, 1, 2, 2};
        fit(X_, y_);
        auto computed = getCutPoints();
        checkCutPoints(computed, expected);
    }

    TEST_F(TestFImdlp, ValueCutPoint) {
        // Case titles as stated in the doc
        samples_t X1a{3.1f, 3.2f, 3.3f, 3.4f, 3.5f, 3.6f, 3.7f, 3.8f, 3.9f, 4.0f};
        test_result(X1a, 6, 7.3f / 2, 6, "1a");
        samples_t X2a = {3.1f, 3.2f, 3.3f, 3.4f, 3.7f, 3.7f, 3.7f, 3.8f, 3.9f, 4.0f};
        test_result(X2a, 6, 7.1f / 2, 4, "2a");
        samples_t X2b = {3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.8f, 3.9f, 4.0f};
        test_result(X2b, 6, 7.5f / 2, 7, "2b");
        samples_t X3a = {3.f, 3.2f, 3.3f, 3.4f, 3.7f, 3.7f, 3.7f, 3.8f, 3.9f, 4.0f};
        test_result(X3a, 4, 7.1f / 2, 4, "3a");
        samples_t X3b = {3.1f, 3.2f, 3.3f, 3.4f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f};
        test_result(X3b, 4, 7.1f / 2, 4, "3b");
        samples_t X4a = {3.1f, 3.2f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.9f, 4.0f};
        test_result(X4a, 4, 6.9f / 2, 2, "4a");
        samples_t X4b = {3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.8f, 3.9f, 4.0f};
        test_result(X4b, 4, 7.5f / 2, 7, "4b");
        samples_t X4c = {3.1f, 3.2f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f, 3.7f};
        test_result(X4c, 4, 6.9f / 2, 2, "4c");
    }

    TEST_F(TestFImdlp, MaxDepth) {
        // Set max_depth to 1
        auto test = CPPFImdlp(3, 1, 0);
        vector<cutPoints_t> expected = {
                {5.45f},
                {3.35f},
                {2.45f},
                {0.8f}
        };
        vector<int> depths = {1, 1, 1, 1};
        test_dataset(test, "iris", expected, depths);
    }

    TEST_F(TestFImdlp, MinLength) {
        auto test = CPPFImdlp(75, 100, 0);
        // Set min_length to 75
        vector<cutPoints_t> expected = {
                {5.45f, 5.75f},
                {2.85f, 3.35f},
                {2.45f, 4.75f},
                {0.8f,  1.75f}
        };
        vector<int> depths = {3, 2, 2, 2};
        test_dataset(test, "iris", expected, depths);
    }

    TEST_F(TestFImdlp, MinLengthMaxDepth) {
        // Set min_length to 75
        auto test = CPPFImdlp(75, 2, 0);
        vector<cutPoints_t> expected = {
                {5.45f, 5.75f},
                {2.85f, 3.35f},
                {2.45f, 4.75f},
                {0.8f,  1.75f}
        };
        vector<int> depths = {2, 2, 2, 2};
        test_dataset(test, "iris", expected, depths);
    }

    TEST_F(TestFImdlp, MaxCutPointsInteger) {
        // Set min_length to 75
        auto test = CPPFImdlp(75, 2, 1);
        vector<cutPoints_t> expected = {
                {5.45f},
                {3.35f},
                {2.45f},
                {0.8f}
        };
        vector<int> depths = {1, 1, 1, 1};
        test_dataset(test, "iris", expected, depths);
    }

    TEST_F(TestFImdlp, MaxCutPointsFloat) {
        // Set min_length to 75
        auto test = CPPFImdlp(75, 2, 0.2f);
        vector<cutPoints_t> expected = {
                {5.45f, 5.75f},
                {2.85f, 3.35f},
                {2.45f, 4.75f},
                {0.8f,  1.75f}
        };
        vector<int> depths = {2, 2, 2, 2};
        test_dataset(test, "iris", expected, depths);
    }

    TEST_F(TestFImdlp, ProposedCuts) {
        vector<pair<float, size_t>> proposed_list = {{0.1f,  2},
                                                     {0.5f,  10},
                                                     {0.07f, 1},
                                                     {1.0f,  1},
                                                     {2.0f,  2}};
        size_t expected;
        size_t computed;
        for (auto proposed_item: proposed_list) {
            tie(proposed_cuts, expected) = proposed_item;
            computed = compute_max_num_cut_points();
            ASSERT_EQ(expected, computed);
        }

    }
}
