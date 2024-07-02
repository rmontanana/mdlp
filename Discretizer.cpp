#include "Discretizer.h"

namespace mdlp {
    // The next to templates have been taken to have the chance to customize them to match 
    // np.searchsorted that is used in scikit-learn KBinsDiscretizer
    // Code Taken from https://cplusplus.com/reference/algorithm/upper_bound/?kw=upper_bound
    template <class ForwardIterator, class T>
    ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, const T& val)
    {
        ForwardIterator it;
        typename iterator_traits<ForwardIterator>::difference_type count, step;
        count = std::distance(first, last);
        while (count > 0) {
            it = first; step = count / 2; std::advance(it, step);
            if (!(val < *it))                 // or: if (!comp(val,*it)), for version (2)
            {
                first = ++it; count -= step + 1;
            } else count = step;
        }
        return first;
    }
    // Code Taken from https://cplusplus.com/reference/algorithm/lower_bound/?kw=lower_bound
    template <class ForwardIterator, class T>
    ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& val)
    {
        ForwardIterator it;
        typename iterator_traits<ForwardIterator>::difference_type count, step;
        count = distance(first, last);
        while (count > 0) {
            it = first; step = count / 2; advance(it, step);
            if (*it < val) {                 // or: if (comp(*it,val)), for version (2)
                first = ++it;
                count -= step + 1;
            } else count = step;
        }
        return first;
    }
    labels_t& Discretizer::transform(const samples_t& data)
    {
        discretizedData.clear();
        discretizedData.reserve(data.size());
        // CutPoints always have at least two items
        // Have to ignore first and last cut points provided
        auto first = cutPoints.begin() + 1;
        auto last = cutPoints.end() - 1;
        auto bound = direction == bound_dir_t::LEFT ? my_lower_bound<std::vector<float>::iterator, float> : my_upper_bound<std::vector<float>::iterator, float>;
        for (const precision_t& item : data) {
            auto pos = bound(first, last, item);
            int number = pos - first;
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