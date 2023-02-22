#include <numeric>
#include <algorithm>
#include <set>
#include <cmath>
#include <limits>
#include "CPPFImdlp.h"
#include "Metrics.h"

namespace mdlp {

    CPPFImdlp::CPPFImdlp(): indices(indices_t()), X(samples_t()), y(labels_t()),
        metrics(Metrics(y, indices))
    {
    }
    CPPFImdlp::~CPPFImdlp() = default;

    CPPFImdlp& CPPFImdlp::fit(samples_t& X_, labels_t& y_)
    {
        X = X_;
        y = y_;
        cutPoints.clear();
        if (X.size() != y.size()) {
            throw invalid_argument("X and y must have the same size");
        }
        if (X.empty() || y.empty()) {
            throw invalid_argument("X and y must have at least one element");
        }
        indices = sortIndices(X_, y_);
        metrics.setData(y, indices);
        computeCutPoints(0, X.size());
        return *this;
    }

    pair<precision_t, size_t> CPPFImdlp::valueCutPoint(size_t start, size_t cut, size_t end)
    {
        size_t n, m, idxPrev = cut - 1 >= start ? cut - 1 : cut;
        size_t idxNext = cut + 1 < end ? cut + 1 : cut;
        bool backWall; // true if duplicates reach begining of the interval
        precision_t previous, actual, next;
        previous = X[indices[idxPrev]];
        actual = X[indices[cut]];
        next = X[indices[idxNext]];
        // definition 2 of the paper => X[t-1] < X[t]
        // get the first equal value of X in the interval
        while (idxPrev > start && actual == previous) {
            previous = X[indices[--idxPrev]];
        }
        backWall = idxPrev == start && actual == previous;
        // get the last equal value of X in the interval
        while (idxNext < end - 1 && actual == next) {
            next = X[indices[++idxNext]];
        }
        // # of duplicates before cutpoint
        n = cut - 1 - idxPrev;
        // # of duplicates after cutpoint
        m = idxNext - cut - 1;
        // Decide which values to use
        cut = cut + (backWall ? m + 1 : -n);
        actual = X[indices[cut]];
        return { (actual + previous) / 2, cut };
    }

    void CPPFImdlp::computeCutPoints(size_t start, size_t end)
    {
        size_t cut;
        pair<precision_t, size_t> result;
        if (end - start < 3)
            return;
        cut = getCandidate(start, end);
        if (cut == numeric_limits<size_t>::max())
            return;
        if (mdlp(start, cut, end)) {
            result = valueCutPoint(start, cut, end);
            cut = result.second;
            cutPoints.push_back(result.first);
            computeCutPoints(start, cut);
            computeCutPoints(cut, end);
        }
    }

    size_t CPPFImdlp::getCandidate(size_t start, size_t end)
    {
        /* Definition 1: A binary discretization for A is determined by selecting the cut point TA for which
        E(A, TA; S) is minimal amongst all the candidate cut points. */
        size_t candidate = numeric_limits<size_t>::max(), elements = end - start;
        bool sameValues = true;
        precision_t entropy_left, entropy_right, minEntropy;
        // Check if all the values of the variable in the interval are the same
        for (size_t idx = start + 1; idx < end; idx++) {
            if (X[indices[idx]] != X[indices[start]]) {
                sameValues = false;
                break;
            }
        }
        if (sameValues)
            return candidate;
        minEntropy = metrics.entropy(start, end);
        for (size_t idx = start + 1; idx < end; idx++) {
            // Cutpoints are always on boundaries (definition 2)
            if (y[indices[idx]] == y[indices[idx - 1]])
                continue;
            entropy_left = precision_t(idx - start) / elements * metrics.entropy(start, idx);
            entropy_right = precision_t(end - idx) / elements * metrics.entropy(idx, end);
            if (entropy_left + entropy_right < minEntropy) {
                minEntropy = entropy_left + entropy_right;
                candidate = idx;
            }
        }
        return candidate;
    }

    bool CPPFImdlp::mdlp(size_t start, size_t cut, size_t end)
    {
        int k, k1, k2;
        precision_t ig, delta;
        precision_t ent, ent1, ent2;
        auto N = precision_t(end - start);
        if (N < 2) {
            return false;
        }
        k = metrics.computeNumClasses(start, end);
        k1 = metrics.computeNumClasses(start, cut);
        k2 = metrics.computeNumClasses(cut, end);
        ent = metrics.entropy(start, end);
        ent1 = metrics.entropy(start, cut);
        ent2 = metrics.entropy(cut, end);
        ig = metrics.informationGain(start, cut, end);
        delta = log2(pow(3, precision_t(k)) - 2) -
            (precision_t(k) * ent - precision_t(k1) * ent1 - precision_t(k2) * ent2);
        precision_t term = 1 / N * (log2(N - 1) + delta);
        return ig > term;
    }

    // Argsort from https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes
    indices_t CPPFImdlp::sortIndices(samples_t& X_, labels_t& y_)
    {
        indices_t idx(X_.size());
        iota(idx.begin(), idx.end(), 0);
        for (size_t i = 0; i < X_.size(); i++)
            stable_sort(idx.begin(), idx.end(), [&X_, &y_](size_t i1, size_t i2) {
            if (X_[i1] == X_[i2])
                return y_[i1] < y_[i2];
            else
                return X_[i1] < X_[i2];
                });
        return idx;
    }

    cutPoints_t CPPFImdlp::getCutPoints()
    {
        // Remove duplicates and sort
        cutPoints_t output(cutPoints.size());
        set<precision_t> s;
        unsigned size = cutPoints.size();
        for (unsigned i = 0; i < size; i++)
            s.insert(cutPoints[i]);
        output.assign(s.begin(), s.end());
        sort(output.begin(), output.end());
        return output;
    }
}
