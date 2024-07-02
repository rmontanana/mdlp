#include "Discretizer.h"

namespace mdlp {
    labels_t& Discretizer::transform(const samples_t& data)
    {
        discretizedData.clear();
        discretizedData.reserve(data.size());
        // CutPoints always have more than two items
        // Have to ignore first and last cut points provided
        auto first = cutPoints.begin() + 1;
        auto last = cutPoints.end() - 1;
        for (const precision_t& item : data) {
            auto upper = std::lower_bound(first, last, item);
            int number = upper - first;
            /*
            OJO
            */
            if (number < 0)
                throw std::runtime_error("number is less than 0 in discretizer::transform");
            discretizedData.push_back(number);
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
        labels_t y(y_.data_ptr<int>(), y_.data_ptr<int>() + num_elements);
        fit(X, y);
    }
    torch::Tensor Discretizer::transform_t(torch::Tensor& X_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<float>(), X_.data_ptr<float>() + num_elements);
        auto result = transform(X);
        return torch::tensor(result, torch::kInt32);
    }
    torch::Tensor Discretizer::fit_transform_t(torch::Tensor& X_, torch::Tensor& y_)
    {
        auto num_elements = X_.numel();
        samples_t X(X_.data_ptr<precision_t>(), X_.data_ptr<precision_t>() + num_elements);
        labels_t y(y_.data_ptr<int>(), y_.data_ptr<int>() + num_elements);
        auto result = fit_transform(X, y);
        return torch::tensor(result, torch::kInt32);
    }
}