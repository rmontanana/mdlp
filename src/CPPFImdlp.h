// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef CPPFIMDLP_H
#define CPPFIMDLP_H

#include "typesFImdlp.h"
#include <limits>
#include <utility>
#include <string>
#include "Metrics.h"
#include "Discretizer.h"

namespace mdlp {
    /**
     * @brief Fayyad & Irani's Multi-Interval Discretization (MDLP) algorithm
     * 
     * This class implements the MDLP algorithm based on the paper:
     * "Multi-Interval Discretization of Continuous-Valued Attributes for Classification Learning"
     * by Fayyad & Irani (IJCAI 1993).
     * 
     * This is a **supervised** discretization method that uses the Minimum Description Length
     * Principle (MDLP) to find the optimal cut points for a continuous attribute.
     * 
     * ## Algorithm Features
     * 
     * - Sorts values using label values as tie-breakers for identical values
     * - Checks for same label values with different variable values
     * - Intervals with same variable value are ignored for cutpoints
     * - Intervals must have more than two examples to be evaluated
     * - Returns cut points for the variable
     * - Transform method uses cut points with: cut[i-1] <= x < cut[i]
     * 
     * ## Usage Example
     * 
     * @code
     * // Create MDLP discretizer with custom parameters
     * CPPFImdlp disc(3, std::numeric_limits<int>::max(), 0.0f);
     * // min_length=3: minimum interval length
     * // max_depth=infinity: no limit on recursion depth
     * // proposed_cuts=0: no limit on number of cuts
     * 
     * disc.fit(X, y);  // y is REQUIRED - used for label information
     * auto result = disc.transform(X);
     * auto cut_points = disc.getCutPoints();
     * 
     * // PyTorch tensor support
     * auto X_torch = torch::tensor(X, torch::kFloat32);
     * auto y_torch = torch::tensor(y, torch::kInt32);
     * auto result_torch = disc.fit_transform_t(X_torch, y_torch);
     * @endcode
     * 
     * ## Constructor Parameters
     * 
     * - **min_length** (default: 3): Minimum number of samples in an interval
     * - **max_depth** (default: max int): Maximum recursion depth
     * - **proposed_cuts** (default: 0): Maximum number of cuts (0 = no limit)
     * 
     * ## Comparison with Unsupervised Methods
     * 
     * | Feature | CPPFImdlp | BinDisc | PKIDisc |
     * |---------|-----------|---------|---------|
     * | Type | Supervised | Unsupervised | Unsupervised |
     * | Uses y | Yes | No | No |
     * | Algorithm | MDLP | Quantile/Uniform | Proportional k-Interval |
     * | Cut Points | Data-dependent | Fixed/Quantile | sqrt(n) or log(n) |
     * | Interface | fit(X, y) | fit(X, y) | fit(X, y) |
     */
    class CPPFImdlp : public Discretizer {
    public:
        /**
         * @brief Default constructor (creates uninitialized instance)
         */
        CPPFImdlp() = default;

        /**
         * @brief Construct an MDLP discretizer with custom parameters
         * @param min_length_ Minimum number of samples in an interval (default: 3)
         * @param max_depth_ Maximum recursion depth (default: unlimited)
         * @param proposed Number of proposed cuts (default: 0 = no limit)
         * 
         * @throws std::invalid_argument if min_length < 3
         * @throws std::invalid_argument if max_depth < 1
         * @throws std::invalid_argument if proposed < 0
         */
        CPPFImdlp(size_t min_length_, int max_depth_, float proposed);

        virtual ~CPPFImdlp() = default;

        /**
         * @brief Fit the discretizer to data (supervised)
         * @param X_ Input samples (continuous values)
         * @param y_ Labels (REQUIRED - used for label information in MDLP algorithm)
         * 
         * This method computes the optimal cut points using the MDLP algorithm.
         * The y parameter is REQUIRED - it is used to sort data and compute entropy.
         * 
         * After fitting, cut points can be retrieved with getCutPoints().
         */
        void fit(samples_t& X_, labels_t& y_) override;

        /**
         * @brief Get the maximum depth reached during fitting
         * @return Maximum recursion depth
         */
        inline int get_depth() const { return depth; };

    protected:
        size_t min_length = 3;
        int depth = 0;
        int max_depth = numeric_limits<int>::max();
        float proposed_cuts = 0;
        indices_t indices = indices_t();
        samples_t X = samples_t();
        labels_t y = labels_t();
        Metrics metrics = Metrics(y, indices);
        size_t num_cut_points = numeric_limits<size_t>::max();
        static indices_t sortIndices(samples_t&, labels_t&);
        void computeCutPoints(size_t, size_t, int);
        void resizeCutPoints();
        bool mdlp(size_t, size_t, size_t);
        size_t getCandidate(size_t, size_t);
        size_t compute_max_num_cut_points() const;
        pair<precision_t, size_t> valueCutPoint(size_t, size_t, size_t);

        /**
         * @brief Safely access X array with bounds checking
         * @param idx Index in the indices array
         * @return Value from X array
         * @throws std::out_of_range if index is out of bounds
         */
        inline precision_t safe_X_access(size_t idx) const
        {
            if (indices.empty()) {
                throw std::out_of_range("Indices array is empty");
            }
            if (idx >= indices.size()) {
                throw std::out_of_range("Index out of bounds for indices array");
            }
            size_t real_idx = indices[idx];
            if (real_idx >= X.size()) {
                throw std::out_of_range("Index out of bounds for X array");
            }
            return X[real_idx];
        }

        /**
         * @brief Safely access y array with bounds checking
         * @param idx Index in the indices array
         * @return Value from y array
         * @throws std::out_of_range if index is out of bounds
         */
        inline label_t safe_y_access(size_t idx) const
        {
            if (indices.empty()) {
                throw std::out_of_range("Indices array is empty");
            }
            if (idx >= indices.size()) {
                throw std::out_of_range("Index out of bounds for indices array");
            }
            size_t real_idx = indices[idx];
            if (real_idx >= y.size()) {
                throw std::out_of_range("Index out of bounds for y array");
            }
            return y[real_idx];
        }

        /**
         * @brief Safely subtract two size_t values with underflow checking
         * @param a First value
         * @param b Value to subtract
         * @return Result of a - b
         * @throws std::underflow_error if b > a
         */
        inline size_t safe_subtract(size_t a, size_t b) const
        {
            if (b > a) {
                throw std::underflow_error("Subtraction would cause underflow");
            }
            return a - b;
        }
    };
}
#endif
