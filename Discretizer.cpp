#include "Discretizer.h"

namespace mdlp {
    labels_t& Discretizer::transform(const samples_t& data)
    {
        discretizedData.clear();
        discretizedData.reserve(data.size());
        for (const precision_t& item : data) {
            auto upper = std::upper_bound(cutPoints.begin(), cutPoints.end(), item);
            discretizedData.push_back(upper - cutPoints.begin());
        }
        return discretizedData;
    }
    labels_t& Discretizer::fit_transform(samples_t& X_, labels_t& y_)
    {
        fit(X_, y_);
        return transform(X_);
    }
    void Discretizer::fit_t(torch::Tensor& X_, torch::Tensor& y_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<precision_t>(), X_.data_ptr<precision_t>() + num_elements);
        labels_t y(y_.data_ptr<int64_t>(), y_.data_ptr<int64_t>() + num_elements);
        fit(X, y);
    }
    torch::Tensor Discretizer::transform_t(torch::Tensor& X_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<float>(), X_.data_ptr<float>() + num_elements);
        auto result = transform(X);
        return torch::tensor(result, torch::kInt64);
    }
    torch::Tensor Discretizer::fit_transform_t(torch::Tensor& X_, torch::Tensor& y_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<precision_t>(), X_.data_ptr<precision_t>() + num_elements);
        labels_t y(y_.data_ptr<int64_t>(), y_.data_ptr<int64_t>() + num_elements);
        auto result = fit_transform(X, y);
        return torch::tensor(result, torch::kInt64);
    }
}