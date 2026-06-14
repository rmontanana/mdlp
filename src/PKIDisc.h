// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2025 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef PKIDISC_H
#define PKIDISC_H

#include "BinDisc.h"

namespace mdlp {
    enum class compute_strategy_t {
        LOG, // Logarithmic
        SQRT // Square root
    };
    class PKIDisc : public BinDisc {
    public:
        /**
         * @brief Construct a Proportional k-Interval Discretizer
         * @param compute_strategy_ Strategy for computing the number of bins
         * 
         * PKIDisc (Proportional k-Interval Discretization) is based on the paper
         * by Yang & Webb, "Proportional k-Interval Discretization for Naive-Bayes Classifiers".
         * 
         * The number of bins is computed as:
         * - SQRT strategy: ceil(sqrt(n_samples))
         * - LOG strategy: ceil(log(n_samples))
         * 
         * The strategy defaults to SQRT which provides good results for most datasets.
         */
        PKIDisc(compute_strategy_t compute_strategy_ = compute_strategy_t::SQRT);
        ~PKIDisc() = default;
        /**
         * @brief Fit the discretizer to data
         * @param X_ Input samples (continuous values to be discretized)
         * @param y_ Labels (accepted for interface consistency but NOT used in unsupervised discretization)
         * 
         * This method performs Proportional k-Interval discretization on the input data X_.
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
    private:
        compute_strategy_t compute_strategy;
    };
}
#endif
