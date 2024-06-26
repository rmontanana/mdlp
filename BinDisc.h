#ifndef BINDISC_H
#define BINDISC_H

#include "typesFImdlp.h"
#include "Discretizer.h"
#include <string>

namespace mdlp {
    enum class strategy_t {
        UNIFORM,
        QUANTILE
    };
    class BinDisc : public Discretizer {
    public:
        BinDisc(int n_bins = 3, strategy_t strategy = strategy_t::UNIFORM);
        ~BinDisc();
        // y is included for compatibility with the Discretizer interface
        void fit(samples_t& X_, labels_t& y) override;
        void fit(samples_t& X);
    private:
        void fit_uniform(samples_t&);
        void fit_quantile(samples_t&);
        void normalizeCutPoints();
        int n_bins;
        strategy_t strategy;
    };
}
#endif
