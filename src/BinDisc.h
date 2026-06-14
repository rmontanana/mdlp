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
         * @brief Fit the discretizer to data (convenience overload)
         * @param X Input samples (continuous values to be discretized)
         * 
         * This overload is provided for convenience when the y parameter
         * is not available. It calls the main fit method with an empty labels vector.
         * 
         * Note: This method is not marked override as it's a new signature.
         */
        void fit(samples_t& X);
    protected:
        std::vector<precision_t> linspace(precision_t start, precision_t end, int num);
        std::vector<precision_t> percentile(samples_t& data, const std::vector<precision_t>& percentiles);
        int n_bins;
        strategy_t strategy;
        const int min_bins = 3;
    private:
        void fit_uniform(const samples_t&);
        void fit_quantile(const samples_t&);
    };
}
#endif
