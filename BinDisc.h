#ifndef BINDISC_H
#define BINDISC_H

#include "typesFImdlp.h"
#include <string>

namespace mdlp {

    enum class strategy_t {
        UNIFORM,
        QUANTILE
    };
    class BinDisc {
    public:
        BinDisc(int n_bins = 3, strategy_t strategy = strategy_t::UNIFORM);
        ~BinDisc();
        void fit(samples_t&);
        inline cutPoints_t getCutPoints() const { return cutPoints; };
        labels_t& transform(const samples_t&);
        static inline std::string version() { return "1.0.0"; };
    private:
        void fit_uniform(samples_t&);
        void fit_quantile(samples_t&);
        void normalizeCutPoints();
        int n_bins;
        strategy_t strategy;
        labels_t discretizedData = labels_t();
        cutPoints_t cutPoints;
    };
}
#endif
