// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2025 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef PKIDISC_H
#define PKIDISC_H

#include "BinDisc.h"

namespace mdlp {
    class PKIDisc : public BinDisc {
    public:
        /**
         * @brief Construct a Proportional k-Interval Discretizer
         * @param compute_strategy_ Strategy for computing the number of bins
         * 
         * PKIDisc (Proportional k-Interval Discretization) is based on the paper
         * by Yang & Webb, "Proportional k-Interval Discretization for Naive-Bayes Classifiers".
         * 
         * The number of bins is derived from the number of samples in X, truncated
         * and never below MIN_BINS:
         * - SQRT strategy: max(MIN_BINS, trunc(sqrt(n_samples)))
         * - LOG strategy: max(MIN_BINS, trunc(log(n_samples)))
         * 
         * The bins are then placed with BinDisc's QUANTILE strategy, so like any
         * quantile binning the number of cut points actually returned can be
         * lower than n_bins + 1 when the data has few distinct values; see
         * BinDisc::fit.
         * 
         * The strategy defaults to SQRT which provides good results for most datasets.
         */
        explicit PKIDisc(compute_strategy_t compute_strategy_ = compute_strategy_t::SQRT);
        ~PKIDisc() = default;
        /**
         * @brief Fit the discretizer to data
         * @param X_ Input samples (continuous values to be discretized)
         * @param y Labels (accepted for interface consistency but NOT used in unsupervised discretization)
         * 
         * This method performs Proportional k-Interval discretization on the input data X_.
         * The bin count is derived from X_.size(); y is not read at all.
         * 
         * Note: The y parameter is required for a uniform interface across supervised
         * and unsupervised discretization methods (all discretizers accept fit(X, y)),
         * but is not used in this unsupervised algorithm. This design allows using
         * the same code path for all discretizer types in experimentation platforms.
         * 
         * For supervised discretization with label information, use CPPFImdlp instead.
         * 
         * Example:
         * @code
         * PKIDisc disc(compute_strategy_t::SQRT);  // Use sqrt(n_samples) bins
         * disc.fit(X, y);  // y is ignored but required for interface
         * auto result = disc.transform(X);
         * @endcode
         */
        void fit(samples_t& X_, labels_t& y) override;
        /**
         * @brief Fit the discretizer, adopting the caller's buffer
         * @param X_ Input samples; surrendered by the caller
         * @param y Labels; accepted for interface consistency, ignored
         *
         * @warning If this throws, X_ has already been moved from.
         */
        void fit(samples_t&& X_, labels_t&& y) override;
        /**
         * @brief Fit on samples alone
         * @param X Input samples; the bin count is derived from X.size()
         *
         * Declared here rather than inherited: BinDisc's samples-only overloads
         * know nothing about select_bins(), so inheriting them ran a plain
         * three-bin UNIFORM fit under the PKIDisc name.
         */
        void fit(samples_t& X);
        /**
         * @brief Fit on samples alone, adopting the caller's buffer
         * @param X Input samples; surrendered by the caller
         */
        void fit(samples_t&& X);

        /**
         * @brief Fit and transform in one call, returning an owned result
         * @param X Input samples
         * @param y Labels; accepted for interface consistency, ignored
         * @param compute_strategy How to derive the bin count from the sample count
         * @return Discretized labels, by value
         *
         * Safe on a temporary, unlike the member `fit_transform`.
         */
        static labels_t discretize(const samples_t& X, const labels_t& y,
            compute_strategy_t compute_strategy = compute_strategy_t::SQRT);
    private:
        // Picks n_bins from the sample count; shared by both fit() overloads.
        void select_bins(size_t n_samples);
        compute_strategy_t compute_strategy;
    };
}
#endif
