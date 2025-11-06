// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2025 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <gtest/gtest.h>
#include "PKIDisc.h"
#include "typesFImdlp.h"
#include <vector>
#include <cmath>

TEST(PKIDisc, fit)
{
    mdlp::labels_t y = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3 }; // 16 samples
    mdlp::samples_t X = { { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } };
    mdlp::PKIDisc discretizer;
    discretizer.fit(X, y);
    auto cut_points = discretizer.getCutPoints();
    // n_bins = sqrt(16) = 4
    // strategy = QUANTILE
    // min = 1, max = 16
    // cut points should be at 1 + 1*(16-1)/4, 1 + 2*(16-1)/4, 1 + 3*(16-1)/4
    // 1 + 15/4 = 1 + 3.75 = 4.75
    // 1 + 30/4 = 1 + 7.5 = 8.5
    // 1 + 45/4 = 1 + 11.25 = 12.25

    ASSERT_EQ(cut_points.size(), 5); // 4 bins = 5 cut points
    EXPECT_NEAR(cut_points[0], 1.0, 0.001);
    EXPECT_NEAR(cut_points[1], 4.75, 0.001);
    EXPECT_NEAR(cut_points[2], 8.5, 0.001);
    EXPECT_NEAR(cut_points[3], 12.25, 0.001);
    EXPECT_NEAR(cut_points[4], 16.0, 0.001);
}

TEST(PKIDisc, fit_less_samples)
{
    mdlp::labels_t y = { 1, 1, 1, 1, 1, 2, 2, 2, 2 }; // 9 samples
    mdlp::samples_t X = { { 1, 2, 3, 4, 5, 6, 7, 8, 9 } };
    mdlp::PKIDisc discretizer;
    discretizer.fit(X, y);
    auto cut_points = discretizer.getCutPoints();
    // n_bins = sqrt(9) = 3
    // strategy = QUANTILE
    // min = 1, max = 9
    // cut points should be at 1 + 1*(9-1)/3, 1 + 2*(9-1)/3
    // 1 + 8/3 = 1 + 2.666... = 3.666...
    // 1 + 16/3 = 1 + 5.333... = 6.333...
    ASSERT_EQ(cut_points.size(), 4); // 3 bins = 4 cut points
    EXPECT_NEAR(cut_points[0], 1.0, 0.001);
    EXPECT_NEAR(cut_points[1], 3.666, 0.001);
    EXPECT_NEAR(cut_points[2], 6.333, 0.001);
    EXPECT_NEAR(cut_points[3], 9.0, 0.001);
}

TEST(PKIDisc, no_cut_points_for_unique_value)
{
    mdlp::labels_t y = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }; // 16 samples
    mdlp::samples_t X = { { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 } };
    mdlp::PKIDisc discretizer;
    discretizer.fit(X, y);
    auto cut_points = discretizer.getCutPoints();
    EXPECT_EQ(cut_points.size(), 2); // Only one cut point since all values are the same
    for (const auto& point : cut_points) {
        EXPECT_NEAR(point, 5.0, 0.001);
    }
}
