// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

// The configuration objects and the static discretize() helpers.
//
// Two properties matter most and are easy to lose in a refactor: a config must
// produce *exactly* the same discretizer as the positional constructor, and the
// fluent setters must not mutate the object they are called on.

#include <string>
#include "gtest/gtest.h"
#include "DiscretizerConfig.h"
#include "CPPFImdlp.h"
#include "BinDisc.h"
#include "PKIDisc.h"

namespace mdlp {

    // ---- defaults and chaining -------------------------------------------- //

    TEST(Config, MDLPDefaultsMatchTheConstructorDefaults)
    {
        const MDLPConfig config;
        EXPECT_EQ(3u, config.min_length);
        EXPECT_EQ(std::numeric_limits<int>::max(), config.max_depth);
        EXPECT_FLOAT_EQ(0.0f, config.proposed_cuts);
    }

    TEST(Config, BinDiscDefaultsMatchTheConstructorDefaults)
    {
        const BinDiscConfig config;
        EXPECT_EQ(MIN_BINS, config.n_bins);
        EXPECT_EQ(strategy_t::UNIFORM, config.strategy);
    }

    // The setters return a copy on purpose, so a shared baseline can be varied
    // per experiment without one variation leaking into the next.
    TEST(Config, SettersDoNotMutateTheOriginal)
    {
        const MDLPConfig base;
        const auto derived = base.withMinLength(7).withMaxDepth(4).withProposedCuts(0.5f);

        EXPECT_EQ(3u, base.min_length) << "base was mutated";
        EXPECT_EQ(std::numeric_limits<int>::max(), base.max_depth) << "base was mutated";
        EXPECT_FLOAT_EQ(0.0f, base.proposed_cuts) << "base was mutated";

        EXPECT_EQ(7u, derived.min_length);
        EXPECT_EQ(4, derived.max_depth);
        EXPECT_FLOAT_EQ(0.5f, derived.proposed_cuts);

        const BinDiscConfig bin_base;
        const auto bin_derived = bin_base.withNBins(9).withStrategy(strategy_t::QUANTILE);
        EXPECT_EQ(MIN_BINS, bin_base.n_bins) << "base was mutated";
        EXPECT_EQ(strategy_t::UNIFORM, bin_base.strategy) << "base was mutated";
        EXPECT_EQ(9, bin_derived.n_bins);
        EXPECT_EQ(strategy_t::QUANTILE, bin_derived.strategy);
    }

    TEST(Config, OneBaselineCanBeVariedIndependently)
    {
        const auto base = MDLPConfig{}.withMaxDepth(10);
        const auto a = base.withMinLength(3);
        const auto b = base.withMinLength(7);
        EXPECT_EQ(3u, a.min_length);
        EXPECT_EQ(7u, b.min_length);
        EXPECT_EQ(10, a.max_depth);
        EXPECT_EQ(10, b.max_depth);
    }

    // ---- validation is shared with the constructors ------------------------ //

    TEST(Config, ValidateRejectsWhatTheConstructorRejects)
    {
        EXPECT_THROW(MDLPConfig{}.withMinLength(2).validate(), InvalidParameter);
        EXPECT_THROW(MDLPConfig{}.withMaxDepth(0).validate(), InvalidParameter);
        EXPECT_THROW(MDLPConfig{}.withProposedCuts(-1.0f).validate(), InvalidParameter);
        EXPECT_THROW(BinDiscConfig{}.withNBins(2).validate(), InvalidParameter);
        EXPECT_NO_THROW(MDLPConfig{}.validate());
        EXPECT_NO_THROW(BinDiscConfig{}.validate());
    }

    // The constructors delegate to validate(), so the messages must be identical
    // — that is the whole point of having one implementation.
    TEST(Config, ValidateAndConstructorGiveTheSameMessage)
    {
        std::string from_validate;
        std::string from_constructor;
        try { MDLPConfig{}.withMinLength(2).validate(); }
        catch (const InvalidParameter& e) { from_validate = e.what(); }
        try { CPPFImdlp(2, 10, 0); }
        catch (const InvalidParameter& e) { from_constructor = e.what(); }
        EXPECT_FALSE(from_validate.empty());
        EXPECT_EQ(from_validate, from_constructor);

        from_validate.clear();
        from_constructor.clear();
        try { BinDiscConfig{}.withNBins(2).validate(); }
        catch (const InvalidParameter& e) { from_validate = e.what(); }
        try { BinDisc(2); }
        catch (const InvalidParameter& e) { from_constructor = e.what(); }
        EXPECT_FALSE(from_validate.empty());
        EXPECT_EQ(from_validate, from_constructor);
    }

    TEST(Config, ConstructorFromConfigRejectsBadValues)
    {
        EXPECT_THROW(CPPFImdlp(MDLPConfig{}.withMinLength(2)), InvalidParameter);
        EXPECT_THROW(CPPFImdlp(MDLPConfig{}.withMaxDepth(0)), InvalidParameter);
        EXPECT_THROW(BinDisc(BinDiscConfig{}.withNBins(1)), InvalidParameter);
    }

    // ---- a config builds the same discretizer as the positional form ------- //

    TEST(Config, ProducesTheSameModelAsThePositionalConstructor)
    {
        const samples_t X_source = { 4.7f, 4.7f, 4.8f, 4.8f, 4.9f, 5.1f, 5.2f, 5.3f, 5.7f, 6.0f };
        const labels_t y_source = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };

        samples_t X_a = X_source;
        labels_t y_a = y_source;
        CPPFImdlp positional(3, 10, 0.0f);
        positional.fit(X_a, y_a);

        samples_t X_b = X_source;
        labels_t y_b = y_source;
        CPPFImdlp configured(MDLPConfig{}.withMinLength(3).withMaxDepth(10).withProposedCuts(0.0f));
        configured.fit(X_b, y_b);

        EXPECT_EQ(positional.getCutPoints(), configured.getCutPoints());
        EXPECT_EQ(positional.get_depth(), configured.get_depth());
    }

    TEST(Config, BinDiscConfigProducesTheSameModelAsThePositionalConstructor)
    {
        const samples_t X_source = { 5.0f, 1.0f, 9.0f, 3.0f, 7.0f, 2.0f, 8.0f, 4.0f, 6.0f, 0.0f };
        const labels_t y_source = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };

        samples_t X_a = X_source;
        labels_t y_a = y_source;
        BinDisc positional(4, strategy_t::QUANTILE);
        positional.fit(X_a, y_a);

        samples_t X_b = X_source;
        labels_t y_b = y_source;
        BinDisc configured(BinDiscConfig{}.withNBins(4).withStrategy(strategy_t::QUANTILE));
        configured.fit(X_b, y_b);

        EXPECT_EQ(positional.getCutPoints(), configured.getCutPoints());
    }

    // ---- the static discretize() helpers ----------------------------------- //

    TEST(Config, DiscretizeMatchesFitThenTransform)
    {
        const samples_t X_source = { 4.7f, 4.7f, 4.8f, 4.8f, 4.9f, 5.1f, 5.2f, 5.3f, 5.7f, 6.0f };
        const labels_t y_source = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2 };

        samples_t X = X_source;
        labels_t y = y_source;
        CPPFImdlp disc;
        disc.fit(X, y);
        const labels_t expected = disc.transform(X_source);

        EXPECT_EQ(expected, CPPFImdlp::discretize(X_source, y_source));
    }

    TEST(Config, DiscretizeHonoursItsConfig)
    {
        const samples_t X_source = { 5.0f, 1.0f, 9.0f, 3.0f, 7.0f, 2.0f, 8.0f, 4.0f, 6.0f, 0.0f };
        const labels_t y_source = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };

        const auto uniform = BinDisc::discretize(X_source, y_source,
            BinDiscConfig{}.withNBins(4).withStrategy(strategy_t::UNIFORM));
        const auto quantile = BinDisc::discretize(X_source, y_source,
            BinDiscConfig{}.withNBins(4).withStrategy(strategy_t::QUANTILE));

        samples_t X = X_source;
        labels_t y = y_source;
        BinDisc reference(4, strategy_t::UNIFORM);
        reference.fit(X, y);
        EXPECT_EQ(reference.transform(X_source), uniform);
        EXPECT_EQ(X_source.size(), quantile.size());
    }

    TEST(Config, PKIDiscDiscretizeMatchesFitThenTransform)
    {
        const labels_t y_source(1000, 1);
        samples_t X_source(1000);
        for (size_t i = 0; i < X_source.size(); ++i) {
            X_source[i] = static_cast<precision_t>(i);
        }

        samples_t X = X_source;
        labels_t y = y_source;
        PKIDisc disc(compute_strategy_t::LOG);
        disc.fit(X, y);
        const labels_t expected = disc.transform(X_source);

        EXPECT_EQ(expected, PKIDisc::discretize(X_source, y_source, compute_strategy_t::LOG));
    }

    // discretize() returns by value precisely so this is safe; the member
    // fit_transform returns a reference into storage that a temporary destroys.
    TEST(Config, DiscretizeIsSafeOnATemporary)
    {
        const samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
        const labels_t y = { 0, 0, 0, 0, 1, 1, 1, 1 };

        const labels_t result = CPPFImdlp::discretize(X, y);
        ASSERT_EQ(X.size(), result.size());
        for (const auto bin : result) {
            EXPECT_GE(bin, 0);
        }
    }

    TEST(Config, DiscretizeLeavesTheCallersDataUntouched)
    {
        const samples_t X_before = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
        const labels_t y_before = { 0, 0, 0, 0, 1, 1, 1, 1 };
        samples_t X = X_before;
        labels_t y = y_before;

        CPPFImdlp::discretize(X, y);

        EXPECT_EQ(X_before, X);
        EXPECT_EQ(y_before, y);
    }
}
