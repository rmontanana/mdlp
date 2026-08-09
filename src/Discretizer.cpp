// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include <cmath>
#include "Discretizer.h"

namespace mdlp {

    void Discretizer::validate_finite(const samples_t& data)
    {
        // Branch-free reduction so the compiler can vectorize the common case.
        // The obvious version — test and throw per element — cost 14% on
        // CPPFImdlp::fit and 16% on BinDisc::fit, because the throw inside the
        // loop blocks vectorization. Locating the offender is left to a second
        // pass that only runs when there is one.
        bool all_finite = true;
        for (const precision_t value : data) {
            all_finite &= std::isfinite(value);
        }
        if (all_finite) {
            return;
        }
        for (size_t i = 0; i < data.size(); ++i) {
            if (!std::isfinite(data[i])) {
                throw ValidationError("Sample at index " + std::to_string(i)
                    + " is not a finite number: " + detail::str(data[i]));
            }
        }
    }

    void Discretizer::transform(const samples_t& data, labels_t& out) const
    {
        // Input validation
        if (data.empty()) {
            throw ValidationError("Data for transformation cannot be empty");
        }
        validate_finite(data);
        if (cutPoints.size() < 2) {
            throw NotFittedError("Discretizer not fitted yet or no valid cut points found");
        }

        out.clear();
        out.reserve(data.size());
        // CutPoints always have at least two items
        // Have to ignore first and last cut points provided
        auto first = cutPoints.begin() + 1;
        auto last = cutPoints.end() - 1;
        auto bound = direction == bound_dir_t::LEFT ? std::lower_bound<std::vector<precision_t>::const_iterator, precision_t> : std::upper_bound<std::vector<precision_t>::const_iterator, precision_t>;
        for (const precision_t& item : data) {
            auto pos = bound(first, last, item);
            auto number = pos - first;
            out.push_back(static_cast<label_t>(number));
        }
    }
    labels_t& Discretizer::transform(const samples_t& data)
    {
        transform(data, discretizedData);
        return discretizedData;
    }
    void Discretizer::fit(samples_t&& X_, labels_t&& y_)
    {
        // Default: no move-specific handling. Named rvalue references are
        // lvalues, so this binds to the copying overload.
        fit(X_, y_);
    }
    labels_t& Discretizer::fit_transform(samples_t& X_, labels_t& y_)
    {
        fit(X_, y_);
        return transform(X_);
    }
    torch::Tensor Discretizer::validate_tensor(
        const torch::Tensor& t,
        torch::ScalarType expected_type,
        const std::string& name,
        const std::string& type_name,
        const std::string& empty_message)
    {
        if (t.dim() != 1) {
            throw ValidationError("Only 1D tensors supported");
        }
        if (!t.is_cpu()) {
            throw ValidationError(name + " tensor must reside on the CPU"); // LCOV_EXCL_LINE
        }
        if (t.scalar_type() != expected_type) {
            throw ValidationError(name + " tensor must be " + type_name + " type");
        }
        if (t.numel() == 0) {
            throw ValidationError(empty_message);
        }
        // Accept non-contiguous inputs. A column view of a 2-D dataset has a
        // stride greater than one, so walking data_ptr() + numel() would read
        // unrelated memory and silently produce wrong cut points. contiguous()
        // returns the tensor unchanged when it is already contiguous, so the
        // common case costs nothing. The caller must keep the returned tensor
        // alive while it reads through data_ptr().
        return t.contiguous();
    }

    std::pair<torch::Tensor, torch::Tensor> Discretizer::validate_pair(
        const torch::Tensor& X_,
        const torch::Tensor& y_)
    {
        auto X_valid = validate_tensor(X_, torch::kFloat32, "X", "Float32", "Tensors cannot be empty");
        auto y_valid = validate_tensor(y_, torch::kInt32, "y", "Int32", "Tensors cannot be empty");
        if (X_valid.numel() != y_valid.numel()) {
            throw ValidationError("X and y tensors must have same number of elements");
        }
        return { X_valid, y_valid };
    }

    void Discretizer::fit_t(const torch::Tensor& X_, const torch::Tensor& y_)
    {
        auto [X_valid, y_valid] = validate_pair(X_, y_);
        auto num_elements = X_valid.numel();
        samples_t X(X_valid.data_ptr<precision_t>(), X_valid.data_ptr<precision_t>() + num_elements);
        labels_t y(y_valid.data_ptr<int>(), y_valid.data_ptr<int>() + num_elements);
        fit(X, y);
    }
    torch::Tensor Discretizer::transform_t(const torch::Tensor& X_)
    {
        auto X_valid = validate_tensor(X_, torch::kFloat32, "X", "Float32", "Tensor cannot be empty");
        auto num_elements = X_valid.numel();
        samples_t X(X_valid.data_ptr<precision_t>(), X_valid.data_ptr<precision_t>() + num_elements);
        auto result = transform(X);
        return torch::tensor(result, torch_label_t);
    }
    torch::Tensor Discretizer::fit_transform_t(const torch::Tensor& X_, const torch::Tensor& y_)
    {
        auto [X_valid, y_valid] = validate_pair(X_, y_);
        auto num_elements = X_valid.numel();
        samples_t X(X_valid.data_ptr<precision_t>(), X_valid.data_ptr<precision_t>() + num_elements);
        labels_t y(y_valid.data_ptr<int>(), y_valid.data_ptr<int>() + num_elements);
        auto result = fit_transform(X, y);
        return torch::tensor(result, torch_label_t);
    }
}