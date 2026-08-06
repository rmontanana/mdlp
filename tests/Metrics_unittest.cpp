// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <type_traits>
#include <utility>
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

    // entropyFromCounts is the single implementation shared by entropy() and by
    // CPPFImdlp::getCandidate()'s incremental scan. That sharing is what keeps the
    // batch and incremental paths bit-identical, so its contract is worth pinning.
    TEST(Metrics, EntropyFromCounts)
    {
        // Two equally likely classes carry exactly one bit.
        EXPECT_NEAR(1.0f, Metrics::entropyFromCounts(labels_t{ 5, 5 }, 10), 1e-6);
        // Four equally likely classes carry two.
        EXPECT_NEAR(2.0f, Metrics::entropyFromCounts(labels_t{ 1, 1, 1, 1 }, 4), 1e-6);
        // A single class carries none, wherever it sits.
        EXPECT_NEAR(0.0f, Metrics::entropyFromCounts(labels_t{ 10 }, 10), 1e-6);
        EXPECT_NEAR(0.0f, Metrics::entropyFromCounts(labels_t{ 0, 10, 0 }, 10), 1e-6);

        // Trailing zeros must not change the result. getCandidate sizes both of
        // its count arrays to the widest label in the containing interval, so each
        // side routinely carries zeros where the other side's labels live. The
        // equivalence with entropy() depends on this being exact, not close.
        EXPECT_EQ(Metrics::entropyFromCounts(labels_t{ 3, 1 }, 4),
            Metrics::entropyFromCounts(labels_t{ 3, 1, 0, 0, 0 }, 4));

        // Degenerate input: guards the division that would otherwise yield inf
        // and then NaN.
        EXPECT_EQ(0.0f, Metrics::entropyFromCounts(labels_t{}, 0));
        EXPECT_EQ(0.0f, Metrics::entropyFromCounts(labels_t{ 0, 0 }, 0));
    }

    TEST_F(TestMetrics, NumClassesOutOfRange)
    {
        EXPECT_EQ(0, computeNumClasses(0, 20)) << "end beyond indices.size()";
        EXPECT_EQ(0, computeNumClasses(10, 12)) << "start at indices.size()";
        EXPECT_EQ(0, computeNumClasses(11, 12)) << "start beyond indices.size()";

        indices_t empty_indices = {};
        setData(y_, empty_indices);
        EXPECT_EQ(0, computeNumClasses(0, 1)) << "empty indices";
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

    // Regression: y and indices used to be reference members bound at
    // construction. C++ cannot rebind a reference, so setData() assigned
    // *through* them and silently overwrote the caller's original vectors
    // instead of switching to the new ones.
    TEST_F(TestMetrics, SetDataDoesNotClobberTheConstructorVectors)
    {
        const labels_t y_before = y_;

        labels_t other_y = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
        setData(other_y, indices_);

        // The new labels are in effect: a single class means zero entropy.
        EXPECT_NEAR(0.0f, entropy(0, 10), precision);
        // ...and the vector handed to the constructor is untouched.
        EXPECT_EQ(y_before, y_);
    }

    TEST_F(TestMetrics, SetDataSwitchesToTheNewDataAndDropsStaleCache)
    {
        EXPECT_NEAR(1.0f, entropy(0, 10), precision);  // populates the cache

        labels_t other_y = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        indices_t other_indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        setData(other_y, other_indices);

        // A stale cache would still answer 1.0 here.
        EXPECT_NEAR(0.0f, entropy(0, 10), precision);

        // Mutating the caller's vector afterwards must not reach into Metrics.
        other_y = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
        EXPECT_NEAR(0.0f, entropy(0, 10), precision);
    }

    // Metrics used to hold a std::mutex, which made it neither copyable nor
    // movable and, by extension, made CPPFImdlp non-movable too.
    TEST(Metrics, IsCopyableAndMovable)
    {
        static_assert(std::is_copy_constructible_v<Metrics>, "Metrics must be copy constructible");
        static_assert(std::is_copy_assignable_v<Metrics>, "Metrics must be copy assignable");
        static_assert(std::is_move_constructible_v<Metrics>, "Metrics must be move constructible");
        static_assert(std::is_move_assignable_v<Metrics>, "Metrics must be move assignable");

        labels_t y = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };
        indices_t indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        Metrics original(y, indices);
        const precision_t expected = original.entropy(0, 10);

        Metrics copied(original);
        EXPECT_NEAR(expected, copied.entropy(0, 10), 1e-6);

        Metrics moved(std::move(original));
        EXPECT_NEAR(expected, moved.entropy(0, 10), 1e-6);

        // A copy must be independent of its source.
        labels_t other_y = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        copied.setData(other_y, indices);
        EXPECT_NEAR(0.0f, copied.entropy(0, 10), 1e-6);
        EXPECT_NEAR(expected, moved.entropy(0, 10), 1e-6);
    }
}
