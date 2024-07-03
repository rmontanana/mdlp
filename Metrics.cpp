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
    Metrics::Metrics(labels_t& y_, indices_t& indices_) : y(y_), indices(indices_),
        numClasses(computeNumClasses(0, indices_.size()))
    {
    }

    int Metrics::computeNumClasses(size_t start, size_t end)
    {
        set<int> nClasses;
        for (auto i = start; i < end; ++i) {
            nClasses.insert(y[indices[i]]);
        }
        return static_cast<int>(nClasses.size());
    }

    void Metrics::setData(const labels_t& y_, const indices_t& indices_)
    {
        indices = indices_;
        y = y_;
        numClasses = computeNumClasses(0, indices.size());
        entropyCache.clear();
        igCache.clear();
    }

    precision_t Metrics::entropy(size_t start, size_t end)
    {
        precision_t p;
        precision_t ventropy = 0;
        int nElements = 0;
        labels_t counts(numClasses + 1, 0);
        if (end - start < 2)
            return 0;
        if (entropyCache.find({ start, end }) != entropyCache.end()) {
            return entropyCache[{start, end}];
        }
        for (auto i = &indices[start]; i != &indices[end]; ++i) {
            counts[y[*i]]++;
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
        precision_t iGain;
        precision_t entropyInterval;
        precision_t entropyLeft;
        precision_t entropyRight;
        size_t nElementsLeft = cut - start;
        size_t nElementsRight = end - cut;
        size_t nElements = end - start;
        if (igCache.find(make_tuple(start, cut, end)) != igCache.end()) {
            return igCache[make_tuple(start, cut, end)];
        }
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