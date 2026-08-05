// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include "Metrics.h"
#include <set>
#include <cmath>

using namespace std;
namespace mdlp {
    Metrics::Metrics(const labels_t& y_, const indices_t& indices_) : y(y_), indices(indices_)
    {
    }

    int Metrics::computeNumClasses(size_t start, size_t end) const
    {
        set<int> nClasses;
        if (indices.empty() || start >= indices.size() || end > indices.size()) {
            return 0;
        }
        for (auto i = start; i < end; ++i) {
            if (i < indices.size() && indices[i] < y.size()) {
                nClasses.insert(y[indices[i]]);
            }
        }
        return static_cast<int>(nClasses.size());
    }

    void Metrics::setData(const labels_t& y_, const indices_t& indices_)
    {
        // Copies, never aliases: the caller's vectors are left untouched.
        y = y_;
        indices = indices_;
        entropyCache.clear();
        igCache.clear();
    }

    precision_t Metrics::entropy(size_t start, size_t end)
    {
        // end <= start must be tested first: the subtraction is unsigned, so an
        // inverted interval would wrap to a huge value and slip past the guard.
        if (end <= start || end - start < 2)
            return 0;

        if (const auto cached = entropyCache.find({ start, end }); cached != entropyCache.end()) {
            return cached->second;
        }

        precision_t p;
        precision_t ventropy = 0;
        int nElements = 0;

        if (indices.empty() || start >= indices.size() || end > indices.size()) {
            return 0;
        }

        // First pass: find max label to size counts array properly
        size_t max_label = 0;
        for (size_t i = start; i < end; ++i) {
            if (i >= indices.size()) break;
            size_t idx = indices[i];
            if (idx >= y.size()) continue;
            size_t label = y[idx];
            if (label > max_label) {
                max_label = label;
            }
        }

        labels_t counts(max_label + 1, 0);

        // Second pass: count occurrences
        for (size_t i = start; i < end; ++i) {
            if (i >= indices.size()) break;
            size_t idx = indices[i];
            if (idx >= y.size()) continue;
            size_t label = y[idx];
            counts[label]++;
            nElements++;
        }
        for (auto count : counts) {
            if (count > 0) {
                p = static_cast<precision_t>(count) / static_cast<precision_t>(nElements);
                ventropy -= p * log2(p);
            }
        }

        entropyCache[{start, end}] = ventropy;
        return ventropy;
    }

    precision_t Metrics::informationGain(size_t start, size_t cut, size_t end)
    {
        if (const auto cached = igCache.find(make_tuple(start, cut, end)); cached != igCache.end()) {
            return cached->second;
        }

        precision_t iGain;
        precision_t entropyInterval;
        precision_t entropyLeft;
        precision_t entropyRight;
        size_t nElementsLeft = cut - start;
        size_t nElementsRight = end - cut;
        size_t nElements = end - start;

        entropyInterval = entropy(start, end);
        entropyLeft = entropy(start, cut);
        entropyRight = entropy(cut, end);
        iGain = entropyInterval -
            (static_cast<precision_t>(nElementsLeft) * entropyLeft +
                static_cast<precision_t>(nElementsRight) * entropyRight) /
            static_cast<precision_t>(nElements);

        igCache[make_tuple(start, cut, end)] = iGain;
        return iGain;
    }

}
