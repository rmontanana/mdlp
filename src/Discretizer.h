// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef DISCRETIZER_H
#define DISCRETIZER_H

#include <string>
#include <algorithm>
#include "typesFImdlp.h"
#include <torch/torch.h>
#include "config.h"

namespace mdlp {
    enum class bound_dir_t {
        LEFT,
        RIGHT
    };
    const auto torch_label_t = torch::kInt32;

    /**
     * @brief Abstract base class for all discretization algorithms
     * 
     * This class provides a unified interface for both supervised and unsupervised
     * discretization methods. All discretizers implement the fit(X, y) method signature
     * for interface consistency, even when the y parameter is not used (unsupervised methods).
     * 
     * ## Interface Design Philosophy
     * 
     * All discretizers (supervised and unsupervised) accept the same fit(X, y) signature:
     * - **Supervised methods** (CPPFImdlp): Use y for label information during discretization
     * - **Unsupervised methods** (BinDisc, PKIDisc): Accept y for interface consistency but ignore it
     * 
     * This design enables a single code path in experimentation platforms:
     * @code
     * void run_experiment(Discretizer& disc, samples_t& X, labels_t& y) {
     *     disc.fit(X, y);  // Works for ALL discretizer types
     *     auto result = disc.transform(X);
     * }
     * @endcode
     * 
     * ## Available Discretization Algorithms
     * 
     * 1. **CPPFImdlp** - Fayyad & Irani's MDLP (supervised, uses label information)
     * 2. **BinDisc** - K-bins discretization (unsupervised, uniform or quantile strategies)
     * 3. **PKIDisc** - Proportional k-Interval Discretization (unsupervised)
     * 
     * ## Example Usage
     * 
     * @code
     * // Supervised discretization
     * CPPFImdlp mdlp_disc(3, std::numeric_limits<int>::max(), 0.0f);
     * mdlp_disc.fit(X, y);
     * auto result1 = mdlp_disc.transform(X);
     * 
     * // Unsupervised discretization (y accepted but ignored)
     * BinDisc bin_disc(3, strategy_t::QUANTILE);
     * bin_disc.fit(X, y);  // y is ignored
     * auto result2 = bin_disc.transform(X);
     * 
     * // PyTorch tensor support
     * auto X_torch = torch::tensor(X, torch::kFloat32);
     * auto y_torch = torch::tensor(y, torch::kInt32);
     * auto result3 = mdlp_disc.fit_transform_t(X_torch, y_torch);
     * @endcode
     */
    class Discretizer {
    public:
        Discretizer() = default;
        virtual ~Discretizer() = default;

        /**
         * @brief Get the cut points computed during fitting
         * @return Vector of cut point values
         */
        inline cutPoints_t getCutPoints() const { return cutPoints; };

        /**
         * @brief Fit the discretizer to data (pure virtual)
         * @param X_ Input samples (continuous values)
         * @param y_ Labels (required for interface consistency)
         * 
         * This method must be implemented by all derived classes.
         * Note: Unsupervised discretizers accept y but do not use it.
         */
        virtual void fit(samples_t& X_, labels_t& y_) = 0;

        /**
         * @brief Transform data using previously computed cut points
         * @param data Input samples to discretize
         * @return Discretized labels as a reference to internal storage
         */
        labels_t& transform(const samples_t& data);

        /**
         * @brief Fit and transform in a single call
         * @param X_ Input samples
         * @param y_ Labels (required for interface consistency)
         * @return Discretized labels
         */
        labels_t& fit_transform(samples_t& X_, labels_t& y_);

        /**
         * @brief Fit the discretizer using PyTorch tensors
         * @param X_ Input tensor (Float32, 1D)
         * @param y_ Labels tensor (Int32, 1D)
         */
        void fit_t(const torch::Tensor& X_, const torch::Tensor& y_);

        /**
         * @brief Transform PyTorch tensor using previously computed cut points
         * @param X_ Input tensor (Float32, 1D)
         * @return Discretized tensor (Int32)
         */
        torch::Tensor transform_t(const torch::Tensor& X_);

        /**
         * @brief Fit and transform PyTorch tensors in a single call
         * @param X_ Input tensor (Float32, 1D)
         * @param y_ Labels tensor (Int32, 1D)
         * @return Discretized tensor (Int32)
         */
        torch::Tensor fit_transform_t(const torch::Tensor& X_, const torch::Tensor& y_);

        /**
         * @brief Get the library version
         * @return Version string (e.g., "2.1.3")
         */
        static inline std::string version() { return { project_mdlp_version.begin(), project_mdlp_version.end() }; };

    protected:
        labels_t discretizedData = labels_t();
        cutPoints_t cutPoints; // At least two cutpoints must be provided, the first and the last will be ignored in transform
        bound_dir_t direction; // used in transform
    };
}
#endif
