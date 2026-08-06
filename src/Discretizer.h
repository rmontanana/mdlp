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
#include "Exceptions.h"

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
         * @brief Fit the discretizer, taking ownership of the inputs
         * @param X_ Input samples; surrendered by the caller
         * @param y_ Labels; surrendered by the caller
         *
         * Equivalent in result to the lvalue overload, but lets implementations
         * adopt the caller's buffers instead of copying them. Use it when the
         * caller has no further use for X_ and y_.
         *
         * The default implementation simply forwards to the copying overload, so
         * a discretizer that does not override this still behaves correctly.
         *
         * @note Measured gain is memory, not speed: at n = 100 000 the copies this
         *       avoids are ~0.0001% of a CPPFImdlp::fit call. See docs/benchmarks.md.
         * @warning If this throws, X_ and y_ have already been moved from and are
         *          in a valid but unspecified state.
         * @warning A derived class that declares any `fit` **hides every** `fit`
         *          overload inherited from here, including this one. Add
         *          `using Discretizer::fit;` (or override this too) or callers
         *          will not be able to pass rvalues to your subclass.
         */
        virtual void fit(samples_t&& X_, labels_t&& y_);

        /**
         * @brief Transform data using previously computed cut points
         * @param data Input samples to discretize
         * @return Discretized labels as a reference to internal storage
         *
         * @note The returned reference is invalidated by the next call to
         *       transform() on this object. Use the two-argument overload to
         *       write into a buffer you own.
         */
        labels_t& transform(const samples_t& data);

        /**
         * @brief Transform data into a caller-supplied buffer
         * @param data Input samples to discretize
         * @param out Destination; cleared and resized to match data
         *
         * Avoids the copy a caller would otherwise make to keep the result, and
         * lets a buffer be reused across calls. Unlike the returning overload,
         * this does not touch the object's internal storage, so it is const.
         */
        void transform(const samples_t& data, labels_t& out) const;

        /**
         * @brief Fit and transform in a single call
         * @param X_ Input samples
         * @param y_ Labels (required for interface consistency)
         * @return Discretized labels
         */
        labels_t& fit_transform(samples_t& X_, labels_t& y_);

        /**
         * @brief Fit the discretizer using PyTorch tensors
         * @param X_ Input tensor (Float32, 1D, CPU)
         * @param y_ Labels tensor (Int32, 1D, CPU)
         * @throws ValidationError (also a `std::invalid_argument`) if a tensor is
         *         not 1D, not on the CPU, has the wrong dtype, is empty, or if
         *         sizes do not match
         *
         * @note Non-contiguous tensors are accepted. A column view of a 2-D
         *       dataset (e.g. `dataset.select(1, col)`) is not contiguous, and
         *       this is the most natural way to feed one feature at a time, so
         *       such inputs are copied into contiguous storage rather than
         *       rejected. The copy is only made when the input is not already
         *       contiguous.
         */
        void fit_t(const torch::Tensor& X_, const torch::Tensor& y_);

        /**
         * @brief Transform PyTorch tensor using previously computed cut points
         * @param X_ Input tensor (Float32, 1D, CPU)
         * @return Discretized tensor (Int32)
         * @throws ValidationError (also a `std::invalid_argument`) if X_ is not 1D,
         *         not on the CPU, has the wrong dtype, or is empty
         *
         * @note Non-contiguous tensors are accepted; see fit_t().
         */
        torch::Tensor transform_t(const torch::Tensor& X_);

        /**
         * @brief Fit and transform PyTorch tensors in a single call
         * @param X_ Input tensor (Float32, 1D, CPU)
         * @param y_ Labels tensor (Int32, 1D, CPU)
         * @return Discretized tensor (Int32)
         * @throws ValidationError (also a `std::invalid_argument`) under the same
         *         conditions as fit_t()
         *
         * @note Non-contiguous tensors are accepted; see fit_t().
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
        // Used in transform. Must have an initializer: a default-constructed
        // CPPFImdlp never assigns it, and transform() would otherwise branch on
        // an indeterminate value.
        bound_dir_t direction = bound_dir_t::RIGHT;

    private:
        /**
         * @brief Validate a 1-D CPU tensor and return a contiguous equivalent
         * @param t Tensor to validate
         * @param expected_type Required scalar type
         * @param name Tensor name used in error messages ("X" or "y")
         * @param type_name Human-readable type name used in error messages
         * @param empty_message Message thrown when the tensor has no elements
         * @return A contiguous tensor; the input itself when already contiguous
         * @throws ValidationError (also a `std::invalid_argument`) if any check fails
         */
        static torch::Tensor validate_tensor(
            const torch::Tensor& t,
            torch::ScalarType expected_type,
            const std::string& name,
            const std::string& type_name,
            const std::string& empty_message);

        /**
         * @brief Validate an (X, y) tensor pair and return contiguous equivalents
         * @throws ValidationError (also a `std::invalid_argument`) if either tensor is
         *         invalid or the sizes differ
         */
        static std::pair<torch::Tensor, torch::Tensor> validate_pair(
            const torch::Tensor& X_,
            const torch::Tensor& y_);
    };
}
#endif
