// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// Full-dataset regression tests over real data.
//
// These run MDLP across *every* feature of datasets that were previously only
// usable in truncated form, because fit() was quadratic: discretizing all 216
// features of mfeat-factors took 762 ms and all 50 features of a 130k-row set
// took over twelve minutes. Phase 7 made the full pass cheap, so the full pass is
// now what gets tested.
//
// They also act as a soft guard against reintroducing that complexity: a
// quadratic regression would not fail an assertion, but it would make this file
// take minutes instead of seconds, which is hard to miss.
//
// Assertions are structural invariants rather than exact cut points on purpose.
// Entropy goes through log2(), and libm is not guaranteed to agree on the last
// ulp between platforms, so a hard-coded cut point list could pass on macOS and
// fail on Linux over a difference that means nothing.

#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <ArffFiles.hpp>
#include "gtest/gtest.h"
#include "CPPFImdlp.h"

namespace mdlp {

    static std::string real_data_path()
    {
        std::string path = "datasets/";
        std::ifstream probe(path + "iris.arff");
        if (probe.is_open()) {
            probe.close();
            return path;
        }
        return "tests/datasets/";
    }

    struct DatasetSpec {
        std::string file;
        bool class_last;        // ArffFiles: false puts the class in the first attribute
        size_t samples;
        size_t features;
        int classes;
    };

    // Verifies the contract fit()/transform() promise, for one feature.
    // Returns whether MDLP found any real cut point, so the caller does not have
    // to fit a third time just to count.
    static bool checkFeature(const DatasetSpec& spec, size_t feature,
        samples_t& X, labels_t& y)
    {
        EXPECT_FALSE(X.empty());
        CPPFImdlp disc;
        samples_t X_fit = X;
        labels_t y_fit = y;
        disc.fit(X_fit, y_fit);
        const auto cuts = disc.getCutPoints();

        const std::string where = spec.file + " feature " + std::to_string(feature);

        // fit() always brackets the data with the observed minimum and maximum,
        // which transform() ignores; so there are at least two.
        EXPECT_GE(cuts.size(), 2u) << where;
        if (cuts.size() < 2) {
            return false;
        }
        const auto [vmin, vmax] = std::minmax_element(X.begin(), X.end());
        EXPECT_EQ(*vmin, cuts.front()) << where;
        EXPECT_EQ(*vmax, cuts.back()) << where;

        // Sorted, because fit() sorts before bracketing.
        for (size_t i = 1; i < cuts.size(); ++i) {
            EXPECT_LE(cuts[i - 1], cuts[i]) << where << " cut " << i;
        }

        // transform() must map every sample into a bin that exists.
        const auto labels = disc.transform(X);
        EXPECT_EQ(X.size(), labels.size()) << where;
        const auto max_bin = static_cast<label_t>(cuts.size() - 2);
        for (size_t i = 0; i < labels.size(); ++i) {
            EXPECT_GE(labels[i], 0) << where << " sample " << i;
            EXPECT_LE(labels[i], max_bin) << where << " sample " << i;
        }

        // Refitting the same data must reproduce the same model.
        CPPFImdlp again;
        samples_t X_again = X;
        labels_t y_again = y;
        again.fit(X_again, y_again);
        EXPECT_EQ(cuts, again.getCutPoints()) << where << " is not deterministic";

        return cuts.size() > 2;
    }

    static void runDataset(const DatasetSpec& spec)
    {
        ArffFiles file;
        file.load(real_data_path() + spec.file, spec.class_last);
        std::vector<samples_t>& X = file.getX();
        labels_t& y = file.getY();

        ASSERT_EQ(spec.features, X.size()) << spec.file << ": unexpected feature count";
        ASSERT_EQ(spec.samples, y.size()) << spec.file << ": unexpected sample count";
        const std::set<label_t> distinct(y.begin(), y.end());
        ASSERT_EQ(static_cast<size_t>(spec.classes), distinct.size())
            << spec.file << ": unexpected class count";
        for (size_t f = 0; f < X.size(); ++f) {
            ASSERT_EQ(spec.samples, X[f].size()) << spec.file << " feature " << f;
        }

        size_t features_with_cuts = 0;
        for (size_t f = 0; f < X.size(); ++f) {
            if (checkFeature(spec, f, X[f], y)) {
                ++features_with_cuts;
            }
        }
        // On a real classification dataset MDLP should find structure in a decent
        // share of the features; if it found none anywhere, something is broken
        // rather than merely uninformative.
        EXPECT_GT(features_with_cuts, X.size() / 4)
            << spec.file << ": MDLP found cut points in only " << features_with_cuts
            << " of " << X.size() << " features";
    }

    // 216 features, 10 classes: the widest feature count in the suite, and enough
    // classes to exercise the per-class loop that Phase 7's incremental counts
    // added to the inner scan.
    TEST(RealDatasets, MfeatFactors)
    {
        runDataset({ "mfeat-factors.arff", true, 2000, 216, 10 });
    }

    // 26 classes: the largest k in the suite. Phase 7 replaced an O(n^2) term with
    // O(n*k), so this is the dataset that stresses what the fix costs.
    TEST(RealDatasets, Letter)
    {
        runDataset({ "letter.arff", true, 20000, 16, 26 });
    }

    // Class is the *first* attribute here (speaker), hence class_last = false.
    // ~10k rows, so this is the sample-count end of the range.
    TEST(RealDatasets, JapaneseVowels)
    {
        runDataset({ "kdd_JapaneseVowels.arff", false, 9961, 14, 9 });
    }
}
