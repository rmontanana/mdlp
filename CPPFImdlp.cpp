#include <numeric>
#include <algorithm>
#include <set>
#include <cmath>
#include "CPPFImdlp.h"
#include "Metrics.h"

namespace mdlp {

    CPPFImdlp::CPPFImdlp(size_t min_length_, int max_depth_, float proposed) : min_length(min_length_),
        max_depth(max_depth_),
        proposed_cuts(proposed)
    {
    }

    CPPFImdlp::CPPFImdlp() = default;

    CPPFImdlp::~CPPFImdlp() = default;

    size_t CPPFImdlp::compute_max_num_cut_points() const
    {
        // Set the actual maximum number of cut points as a number or as a percentage of the number of samples
        if (proposed_cuts == 0) {
            return numeric_limits<size_t>::max();
        }
        if (proposed_cuts < 0 || proposed_cuts > static_cast<float>(X.size())) {
            throw invalid_argument("wrong proposed num_cuts value");
        }
        if (proposed_cuts < 1)
            return static_cast<size_t>(round(static_cast<float>(X.size()) * proposed_cuts));
        return static_cast<size_t>(proposed_cuts);
    }

    void CPPFImdlp::fit(samples_t& X_, labels_t& y_)
    {
        X = X_;
        y = y_;
        num_cut_points = compute_max_num_cut_points();
        depth = 0;
        discretizedData.clear();
        cutPoints.clear();
        if (X.size() != y.size()) {
            throw invalid_argument("X and y must have the same size");
        }
        if (X.empty() || y.empty()) {
            throw invalid_argument("X and y must have at least one element");
        }
        if (min_length < 3) {
            throw invalid_argument("min_length must be greater than 2");
        }
        if (max_depth < 1) {
            throw invalid_argument("max_depth must be greater than 0");
        }
        indices = sortIndices(X_, y_);
        metrics.setData(y, indices);
        computeCutPoints(0, X.size(), 1);
        sort(cutPoints.begin(), cutPoints.end());
        if (num_cut_points > 0) {
            // Select the best (with lower entropy) cut points
            while (cutPoints.size() > num_cut_points) {
                resizeCutPoints();
            }
        }
    }

    pair<precision_t, size_t> CPPFImdlp::valueCutPoint(size_t start, size_t cut, size_t end)
    {
        size_t n;
        size_t m;
        size_t idxPrev = cut - 1 >= start ? cut - 1 : cut;
        size_t idxNext = cut + 1 < end ? cut + 1 : cut;
        bool backWall; // true if duplicates reach beginning of the interval
        precision_t previous;
        precision_t actual;
        precision_t next;
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

    void CPPFImdlp::computeCutPoints(size_t start, size_t end, int depth_)
    {
        size_t cut;
        pair<precision_t, size_t> result;
        // Check if the interval length and the depth are Ok
        if (end - start < min_length || depth_ > max_depth)
            return;
        depth = depth_ > depth ? depth_ : depth;
        cut = getCandidate(start, end);
        if (cut == numeric_limits<size_t>::max())
            return;
        if (mdlp(start, cut, end)) {
            result = valueCutPoint(start, cut, end);
            cut = result.second;
            cutPoints.push_back(result.first);
            computeCutPoints(start, cut, depth_ + 1);
            computeCutPoints(cut, end, depth_ + 1);
        }
    }

    size_t CPPFImdlp::getCandidate(size_t start, size_t end)
    {
        /* Definition 1: A binary discretization for A is determined by selecting the cut point TA for which
        E(A, TA; S) is minimal amongst all the candidate cut points. */
        size_t candidate = numeric_limits<size_t>::max();
        size_t elements = end - start;
        bool sameValues = true;
        precision_t entropy_left;
        precision_t entropy_right;
        precision_t minEntropy;
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
            entropy_left = precision_t(idx - start) / static_cast<precision_t>(elements) * metrics.entropy(start, idx);
            entropy_right = precision_t(end - idx) / static_cast<precision_t>(elements) * metrics.entropy(idx, end);
            if (entropy_left + entropy_right < minEntropy) {
                minEntropy = entropy_left + entropy_right;
                candidate = idx;
            }
        }
        return candidate;
    }

    bool CPPFImdlp::mdlp(size_t start, size_t cut, size_t end)
    {
        int k;
        int k1;
        int k2;
        precision_t ig;
        precision_t delta;
        precision_t ent;
        precision_t ent1;
        precision_t ent2;
        auto N = precision_t(end - start);
        k = metrics.computeNumClasses(start, end);
        k1 = metrics.computeNumClasses(start, cut);
        k2 = metrics.computeNumClasses(cut, end);
        ent = metrics.entropy(start, end);
        ent1 = metrics.entropy(start, cut);
        ent2 = metrics.entropy(cut, end);
        ig = metrics.informationGain(start, cut, end);
        delta = static_cast<precision_t>(log2(pow(3, precision_t(k)) - 2) -
            (precision_t(k) * ent - precision_t(k1) * ent1 - precision_t(k2) * ent2));
        precision_t term = 1 / N * (log2(N - 1) + delta);
        return ig > term;
    }

    // Argsort from https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes
    indices_t CPPFImdlp::sortIndices(samples_t& X_, labels_t& y_)
    {
        indices_t idx(X_.size());
        iota(idx.begin(), idx.end(), 0);
        stable_sort(idx.begin(), idx.end(), [&X_, &y_](size_t i1, size_t i2) {
            if (X_[i1] == X_[i2])
                return y_[i1] < y_[i2];
            else
                return X_[i1] < X_[i2];
            });
        return idx;
    }

    void CPPFImdlp::resizeCutPoints()
    {
        //Compute entropy of each of the whole cutpoint set and discards the biggest value
        precision_t maxEntropy = 0;
        precision_t entropy;
        size_t maxEntropyIdx = 0;
        size_t begin = 0;
        size_t end;
        for (size_t idx = 0; idx < cutPoints.size(); idx++) {
            end = begin;
            while (X[indices[end]] < cutPoints[idx] && end < X.size())
                end++;
            entropy = metrics.entropy(begin, end);
            if (entropy > maxEntropy) {
                maxEntropy = entropy;
                maxEntropyIdx = idx;
            }
            begin = end;
        }
        cutPoints.erase(cutPoints.begin() + static_cast<long>(maxEntropyIdx));
    }
    labels_t& CPPFImdlp::transform(const samples_t& data)
    {
        discretizedData.reserve(data.size());
        for (const precision_t& item : data) {
            auto upper = upper_bound(cutPoints.begin(), cutPoints.end(), item);
            discretizedData.push_back(upper - cutPoints.begin());
        }
        return discretizedData;
    }
}
