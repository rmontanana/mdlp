// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef CCMETRICS_H
#define CCMETRICS_H

#include "typesFImdlp.h"

namespace mdlp {
    /**
     * @brief Entropy and information gain over a labelled sample in index order
     *
     * Metrics evaluates entropy and information gain on half-open intervals
     * [start, end) of an index array, which supplies the order in which the
     * labels are visited. Results are memoized per interval.
     *
     * ## Ownership
     *
     * Metrics holds its own copies of the labels and the index order. Earlier
     * versions stored references, which C++ cannot rebind: setData() assigned
     * *through* them, silently overwriting the vectors passed at construction
     * instead of switching to the new ones. Owning the data also keeps the class
     * copyable and movable, which a reference or pointer member pointing into a
     * sibling object could not offer safely.
     *
     * ## Thread safety
     *
     * Metrics is **not** thread-safe, and callers must synchronize externally to
     * share one instance across threads. Note that entropy() and
     * informationGain() mutate internal state even though they read like queries:
     * both memoize their results.
     *
     * Distinct instances share nothing and may be used concurrently without any
     * synchronization.
     *
     * Earlier versions held a mutex around the caches. That advertised a
     * guarantee the class did not deliver — the data members stayed unguarded, so
     * a concurrent setData() still raced — while costing lock traffic on every
     * lookup in what is a single-threaded recursion in
     * CPPFImdlp::computeCutPoints(). The lock is gone and the contract is now
     * stated plainly instead.
     */
    class Metrics {
    protected:
        labels_t y;
        indices_t indices;
        cacheEnt_t entropyCache = cacheEnt_t();
        cacheIg_t igCache = cacheIg_t();
    public:
        Metrics() = default;

        /**
         * @brief Construct with the labels and index order to evaluate
         * @param y Labels; copied
         * @param indices Visit order into y; copied
         */
        Metrics(const labels_t& y, const indices_t& indices);

        /**
         * @brief Replace the labels and index order, discarding memoized results
         * @param y New labels; copied
         * @param indices New visit order into y; copied
         *
         * The vectors passed here are copied, never aliased, so the caller keeps
         * full ownership of the originals and they are left untouched.
         */
        void setData(const labels_t& y, const indices_t& indices);

        /**
         * @brief Count distinct labels in [start, end)
         * @return Number of distinct labels, or 0 if the interval is out of range
         */
        int computeNumClasses(size_t start, size_t end) const;

        /**
         * @brief Entropy of [start, end)
         * @return Entropy in bits, or 0 for intervals shorter than 2 or out of range
         * @note Memoizes; not const.
         */
        precision_t entropy(size_t start, size_t end);

        /**
         * @brief Information gain of splitting [start, end) at cut
         * @note Memoizes; not const.
         */
        precision_t informationGain(size_t start, size_t cut, size_t end);
    };
}
#endif
