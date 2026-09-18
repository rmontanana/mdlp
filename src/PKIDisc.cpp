// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2025 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <cmath>
#include <utility>
#include "PKIDisc.h"

namespace mdlp {

    PKIDisc::PKIDisc(compute_strategy_t compute_strategy_)
        : BinDisc(), compute_strategy(compute_strategy_) {}

    void PKIDisc::select_bins(size_t n_samples)
    {
        // Truncated, not rounded up: log(22) gives 3 bins, sqrt(50) gives 7. The
        // count is derived from X, never from y, so every fit() overload — with
        // or without labels — agrees on it.
        const auto n = static_cast<double>(n_samples);
        if (compute_strategy == compute_strategy_t::LOG) {
            n_bins = static_cast<int>(std::log(n));
        } else {
            n_bins = static_cast<int>(std::sqrt(n));
        }
        strategy = strategy_t::QUANTILE;
        if (n_bins < min_bins) {
            n_bins = min_bins;
        }
    }

    // Every overload picks the bin count here before delegating. The samples-only
    // ones used to be inherited from BinDisc through a using-declaration, which
    // skipped select_bins entirely: PKIDisc::fit(X) ran BinDisc's defaults, three
    // UNIFORM bins, instead of sqrt(n) QUANTILE bins.
    void PKIDisc::fit(samples_t& X, labels_t& y)
    {
        select_bins(X.size());
        BinDisc::fit(X, y);
    }

    void PKIDisc::fit(samples_t&& X, labels_t&& y)
    {
        select_bins(X.size());
        BinDisc::fit(std::move(X), std::move(y));
    }

    void PKIDisc::fit(samples_t& X)
    {
        select_bins(X.size());
        BinDisc::fit(X);
    }

    void PKIDisc::fit(samples_t&& X)
    {
        select_bins(X.size());
        BinDisc::fit(std::move(X));
    }

    labels_t PKIDisc::discretize(const samples_t& X, const labels_t& y, compute_strategy_t compute_strategy)
    {
        PKIDisc disc(compute_strategy);
        samples_t X_copy = X;
        labels_t y_copy = y;
        disc.fit(std::move(X_copy), std::move(y_copy));
        labels_t out;
        disc.transform(X, out);
        return out;
    }
}
