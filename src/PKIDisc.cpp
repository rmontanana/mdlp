// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2025 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <utility>
#include "PKIDisc.h"

namespace mdlp {

    PKIDisc::PKIDisc(compute_strategy_t compute_strategy_)
        : BinDisc(), compute_strategy(compute_strategy_) {}

    void PKIDisc::select_bins(size_t n_samples)
    {
        if (compute_strategy == compute_strategy_t::LOG) {
            n_bins = static_cast<int>(std::log(static_cast<int>(n_samples)));
        } else {
            n_bins = static_cast<int>(sqrt(static_cast<int>(n_samples)));
        }
        strategy = strategy_t::QUANTILE;
        if (n_bins < min_bins) {
            n_bins = min_bins;
        }
    }

    void PKIDisc::fit(samples_t& X, labels_t& y)
    {
        select_bins(y.size());
        BinDisc::fit(X, y);
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

    void PKIDisc::fit(samples_t&& X, labels_t&& y)
    {
        // Read y's size before it is moved from.
        select_bins(y.size());
        BinDisc::fit(std::move(X), std::move(y));
    }
}
