// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef BINDISC_H
#define BINDISC_H

#include "typesFImdlp.h"
#include "Discretizer.h"
#include <string>

namespace mdlp {
    enum class strategy_t {
        UNIFORM, // Equal width
        QUANTILE // Equal frequency
    };
    class BinDisc : public Discretizer {
    public:
        BinDisc(int n_bins = 3, strategy_t strategy = strategy_t::UNIFORM);
        ~BinDisc();
        /**
         * @brief Fit the discretizer to data
         * @param X_ Input samples (continuous values to be discretized)
         * @param y_ Labels (accepted for interface consistency but NOT used in unsupervised binning)
         * 
         * This method performs k-bins discretization on the input data X_.
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
         * BinDisc disc(3, strategy_t::QUANTILE);  // 3 bins with equal frequency
         * disc.fit(X, y);  // y is ignored but required for interface
         * auto result = disc.transform(X);
         * @endcode
         */
        void fit(samples_t& X_, labels_t& y) override;
        /**
         * @brief Fit the discretizer, adopting the caller's buffer
         * @param X_ Input samples; surrendered by the caller
         * @param y Labels; accepted for interface consistency, still ignored
         *
         * Only the QUANTILE strategy benefits: it sorts a copy of the input, and
         * with an rvalue it sorts the caller's buffer in place instead. UNIFORM
         * never copied to begin with.
         *
         * @warning If this throws, X_ has already been moved from.
         */
        void fit(samples_t&& X_, labels_t&& y) override;
        /**
         * @brief Fit the discretizer to data (convenience overload)
         * @param X Input samples (continuous values to be discretized)
         * 
         * This overload is provided for convenience when the y parameter
         * is not available. It calls the main fit method with an empty labels vector.
         * 
         * Note: This method is not marked override as it's a new signature.
         */
        void fit(samples_t& X);
        /**
         * @brief Fit on samples alone, adopting the caller's buffer
         * @param X Input samples; surrendered by the caller
         */
        void fit(samples_t&& X);
    protected:
        std::vector<precision_t> linspace(precision_t start, precision_t end, int num);
        std::vector<precision_t> percentile(samples_t& data, const std::vector<precision_t>& percentiles);
        int n_bins;
        strategy_t strategy;
        // static constexpr, not a const member: a const non-static member would
        // delete the copy and move assignment operators for this class and PKIDisc.
        static constexpr int min_bins = 3;
    private:
        void validate_input(const samples_t& X) const;
        void fit_uniform(const samples_t&);
        // By value: the caller decides whether that costs a copy or a move.
        void fit_quantile(samples_t data);
    };
}
#endif
