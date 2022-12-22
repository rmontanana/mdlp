#include <numeric>
#include <algorithm>
#include <set>
#include <cmath>
#include "CPPFImdlp.h"
#include "Metrics.h"
namespace mdlp {
    CPPFImdlp::CPPFImdlp(int algorithm):algorithm(algorithm), indices(indices_t()), X(samples_t()), y(labels_t()), metrics(Metrics(y, indices))
    {
    }
    CPPFImdlp::~CPPFImdlp()
        = default;
    CPPFImdlp& CPPFImdlp::fit(samples_t& X_, labels_t& y_)
    {
        X = X_;
        y = y_;
        cutPoints.clear();
        if (X.size() != y.size()) {
            throw invalid_argument("X and y must have the same size");
        }
        if (X.size() == 0 || y.size() == 0) {
            throw invalid_argument("X and y must have at least one element");
        }
        indices = sortIndices(X_, y_);
        metrics.setData(y, indices);
        switch (algorithm) {
            case 0:
                computeCutPoints(0, X.size());
                break;
            case 1:
                computeCutPointsAlternative(0, X.size());
                break;
            default:
                throw invalid_argument("algorithm must be 0 or 1");
        }
        return *this;
    }
    precision_t CPPFImdlp::halfWayValueCutPoint(size_t start, size_t idx)
    {
        size_t idxPrev = idx - 1;
        precision_t previous = X[indices[idxPrev]], actual = X[indices[idx]];
        // definition 2 of the paper => X[t-1] < X[t]
        while (idxPrev-- > start && actual == previous) {
            previous = X[indices[idxPrev]];
        }
        return (previous + actual) / 2;
    }
    tuple<precision_t, size_t> CPPFImdlp::completeValueCutPoint(size_t start, size_t cut, size_t end)
    {
        size_t idxPrev = cut - 1;
        precision_t previous, actual;
        previous = X[indices[idxPrev]];
        actual = X[indices[cut]];
        // definition 2 of the paper => X[t-1] < X[t]
        while (idxPrev-- > start && actual == previous) {
            previous = X[indices[idxPrev]];
        }
        // get the last equal value of X in the interval
        while (actual == X[indices[cut++]] && cut < end);
        if (previous == actual && cut < end)
            actual = X[indices[cut]];
        cut--;
        return make_tuple((previous + actual) / 2, cut);
    }
    void CPPFImdlp::computeCutPoints(size_t start, size_t end)
    {
        size_t cut;
        tuple<precision_t, size_t> result;
        if (end - start < 2)
            return;
        cut = getCandidate(start, end);
        if (cut == numeric_limits<size_t>::max())
            return;
        if (mdlp(start, cut, end)) {
            result = completeValueCutPoint(start, cut, end);
            cut = get<1>(result);
            cutPoints.push_back(get<0>(result));
            computeCutPoints(start, cut);
            computeCutPoints(cut, end);
        }
    }
    void CPPFImdlp::computeCutPointsAlternative(size_t start, size_t end)
    {
        size_t cut;
        if (end - start < 2)
            return;
        cut = getCandidate(start, end);
        if (cut == numeric_limits<size_t>::max())
            return;
        if (mdlp(start, cut, end)) {
            cutPoints.push_back(halfWayValueCutPoint(start, cut));
            computeCutPointsAlternative(start, cut);
            computeCutPointsAlternative(cut, end);
        }
    }
    size_t CPPFImdlp::getCandidate(size_t start, size_t end)
    {
        /* Definition 1: A binary discretization for A is determined by selecting the cut point TA for which
        E(A, TA; S) is minimal amogst all the candidate cut points. */
        size_t candidate = numeric_limits<size_t>::max(), elements = end - start;
        precision_t entropy_left, entropy_right, minEntropy;
        minEntropy = metrics.entropy(start, end);
        for (auto idx = start + 1; idx < end; idx++) {
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
            stable_sort(idx.begin(), idx.end(), [&X_, &y_](size_t i1, size_t i2)
                {
                    if (X_[i1] == X_[i2]) return y_[i1] < y_[i2];
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
