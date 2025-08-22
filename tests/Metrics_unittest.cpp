// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include "gtest/gtest.h"
#include "Metrics.h"

namespace mdlp {
    class TestMetrics : public Metrics, public testing::Test {
    public:
        labels_t y_ = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
        indices_t indices_ = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        precision_t precision = 1e-6;

        TestMetrics() : Metrics(y_, indices_) {};

        void SetUp() override
        {
            setData(y_, indices_);
        }
    };

    TEST_F(TestMetrics, NumClasses)
    {
        y = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 1 };
        EXPECT_EQ(1, computeNumClasses(4, 8));
        EXPECT_EQ(2, computeNumClasses(0, 10));
        EXPECT_EQ(2, computeNumClasses(8, 10));
    }

    TEST_F(TestMetrics, Entropy)
    {
        EXPECT_EQ(1, entropy(0, 10));
        EXPECT_EQ(0, entropy(0, 5));
        y = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 1 };
        setData(y, indices);
        ASSERT_NEAR(0.468996f, entropy(0, 10), precision);
        y = { 0, 0, 1, 2, 3 };
        ASSERT_NEAR(1.5f, entropy(0, 4), precision);
        ASSERT_NEAR(1.921928f, entropy(0, 5), precision);
    }

    TEST_F(TestMetrics, EntropyDouble)
    {
        y = { 0, 0, 1, 2, 3 };
        samples_t expected_entropies = { 0.0, 0.0, 0.91829583, 1.5, 1.9219280948873623 };
        for (auto idx = 0; idx < y.size(); ++idx) {
            ASSERT_NEAR(expected_entropies[idx], entropy(0, idx + 1), precision);
        }
    }

    TEST_F(TestMetrics, InformationGain)
    {
        ASSERT_NEAR(1, informationGain(0, 5, 10), precision);
        ASSERT_NEAR(1, informationGain(0, 5, 10), precision); // For cache
        y = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 1 };
        setData(y, indices);
        ASSERT_NEAR(0.108032f, informationGain(0, 5, 10), precision);
    }

    TEST_F(TestMetrics, EntropyBoundsChecking)
    {
        // Test the conditions that cause entropy to return 0
        
        // Test 1: Empty indices array
        indices_t empty_indices = {};
        labels_t test_y = { 1, 2, 3 };
        setData(test_y, empty_indices);
        EXPECT_EQ(0, entropy(0, 1)) << "Should return 0 when indices is empty";
        
        // Test 2: start >= indices.size()
        indices_t small_indices = { 0, 1 };
        setData(test_y, small_indices);
        EXPECT_EQ(0, entropy(2, 3)) << "Should return 0 when start >= indices.size()";
        EXPECT_EQ(0, entropy(5, 6)) << "Should return 0 when start >> indices.size()";
        
        // Test 3: end > indices.size()
        EXPECT_EQ(0, entropy(0, 3)) << "Should return 0 when end > indices.size()";
        EXPECT_EQ(0, entropy(1, 5)) << "Should return 0 when end >> indices.size()";
        
        // Test edge case: start == indices.size()
        EXPECT_EQ(0, entropy(2, 2)) << "Should return 0 when start == indices.size()";
    }
}
