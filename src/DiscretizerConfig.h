// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef MDLP_DISCRETIZER_CONFIG_H
#define MDLP_DISCRETIZER_CONFIG_H

#include <limits>
#include <string>
#include "typesFImdlp.h"
#include "Exceptions.h"

namespace mdlp {

    /** @brief Fewest bins BinDisc and PKIDisc accept */
    inline constexpr int MIN_BINS = 3;

    /**
     * @brief Parameters for CPPFImdlp, as a named and chainable value
     *
     * Positional constructor arguments stop being readable at three:
     * @code
     * CPPFImdlp disc(3, 10, 0.5f);                 // which is which?
     *
     * CPPFImdlp disc(MDLPConfig{}                  // this one says
     *     .withMinLength(3)
     *     .withMaxDepth(10)
     *     .withProposedCuts(0.5f));
     * @endcode
     *
     * The setters return a modified copy rather than mutating, so a config can be
     * kept as a shared baseline and varied per experiment without aliasing:
     * @code
     * const auto base = MDLPConfig{}.withMaxDepth(10);
     * run(base.withMinLength(3));
     * run(base.withMinLength(7));   // base is untouched
     * @endcode
     *
     * @note This *is* the builder. A separate `builder()` would be a second way to
     *       say the same thing.
     */
    struct MDLPConfig {
        /** @brief Minimum number of samples an interval must have to be split */
        size_t min_length = 3;
        /** @brief Maximum recursion depth */
        int max_depth = std::numeric_limits<int>::max();
        /**
         * @brief Limit on the number of cut points
         *
         * 0 means no limit. Below 1 it is a fraction of the sample count; at 1 or
         * above it is an absolute count.
         */
        float proposed_cuts = 0.0f;

        MDLPConfig withMinLength(size_t value) const
        {
            auto copy = *this;
            copy.min_length = value;
            return copy;
        }

        MDLPConfig withMaxDepth(int value) const
        {
            auto copy = *this;
            copy.max_depth = value;
            return copy;
        }

        MDLPConfig withProposedCuts(float value) const
        {
            auto copy = *this;
            copy.proposed_cuts = value;
            return copy;
        }

        /**
         * @brief Reject an invalid combination before it reaches a constructor
         * @throws InvalidParameter with the same message the constructor would give
         *
         * Optional: constructing a discretizer validates anyway. Useful when a
         * config is assembled far from where it is used, e.g. parsed from a file.
         *
         * The constructor delegates here, so there is one set of rules and one set
         * of messages rather than two that can drift apart.
         */
        void validate() const
        {
            if (min_length < 3) {
                throw InvalidParameter("min_length must be at least 3, got " + std::to_string(min_length));
            }
            if (max_depth < 1) {
                throw InvalidParameter("max_depth must be at least 1, got " + std::to_string(max_depth));
            }
            if (proposed_cuts < 0.0f) {
                throw InvalidParameter("proposed_cuts must be non-negative, got " + detail::str(proposed_cuts));
            }
        }
    };

    /**
     * @brief Parameters for BinDisc, as a named and chainable value
     *
     * @code
     * BinDisc disc(BinDiscConfig{}.withNBins(5).withStrategy(strategy_t::QUANTILE));
     * @endcode
     */
    struct BinDiscConfig {
        /** @brief Number of bins; at least 3 */
        int n_bins = 3;
        /** @brief Equal width or equal frequency */
        strategy_t strategy = strategy_t::UNIFORM;

        BinDiscConfig withNBins(int value) const
        {
            auto copy = *this;
            copy.n_bins = value;
            return copy;
        }

        BinDiscConfig withStrategy(strategy_t value) const
        {
            auto copy = *this;
            copy.strategy = value;
            return copy;
        }

        /**
         * @brief Reject an invalid combination before it reaches a constructor
         * @throws InvalidParameter with the same message the constructor would give
         */
        void validate() const
        {
            if (n_bins < MIN_BINS) {
                throw InvalidParameter("n_bins must be at least " + std::to_string(MIN_BINS) + ", got " + std::to_string(n_bins));
            }
        }
    };

    // PKIDisc takes a single parameter and derives everything else from the data,
    // so a config struct for it would carry one field and add a second way to
    // write what its constructor already says plainly.
}
#endif
