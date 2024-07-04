// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#include "Discretizer.h"

namespace mdlp {

    labels_t& Discretizer::transform(const samples_t& data)
    {
        discretizedData.clear();
        discretizedData.reserve(data.size());
        // CutPoints always have at least two items
        // Have to ignore first and last cut points provided
        auto first = cutPoints.begin() + 1;
        auto last = cutPoints.end() - 1;
        auto bound = direction == bound_dir_t::LEFT ? std::lower_bound<std::vector<precision_t>::iterator, precision_t> : std::upper_bound<std::vector<precision_t>::iterator, precision_t>;
        for (const precision_t& item : data) {
            auto pos = bound(first, last, item);
            auto number = pos - first;
            discretizedData.push_back(number);
        }
        return discretizedData;
    }
    labels_t& Discretizer::fit_transform(samples_t& X_, labels_t& y_)
    {
        fit(X_, y_);
        return transform(X_);
    }
    void Discretizer::fit_t(const torch::Tensor& X_, const torch::Tensor& y_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<precision_t>(), X_.data_ptr<precision_t>() + num_elements);
        labels_t y(y_.data_ptr<int>(), y_.data_ptr<int>() + num_elements);
        fit(X, y);
    }
    torch::Tensor Discretizer::transform_t(const torch::Tensor& X_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<precision_t>(), X_.data_ptr<precision_t>() + num_elements);
        auto result = transform(X);
        return torch::tensor(result, torch::kInt32);
    }
    torch::Tensor Discretizer::fit_transform_t(const torch::Tensor& X_, const torch::Tensor& y_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<precision_t>(), X_.data_ptr<precision_t>() + num_elements);
        labels_t y(y_.data_ptr<int>(), y_.data_ptr<int>() + num_elements);
        auto result = fit_transform(X, y);
        return torch::tensor(result, torch::kInt32);
    }
}