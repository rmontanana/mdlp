#ifndef DISCRETIZER_H
#define DISCRETIZER_H

#include <string>
#include <algorithm>
#include <torch/torch.h>
#include "typesFImdlp.h"

namespace mdlp {
    class Discretizer {
    public:
        Discretizer() = default;
        virtual ~Discretizer() = default;
        inline cutPoints_t getCutPoints() const { return cutPoints; };
        virtual void fit(samples_t& X_, labels_t& y_) = 0;
        labels_t& transform(const samples_t& data);
        labels_t& fit_transform(samples_t& X_, labels_t& y_);
        void fit_t(torch::Tensor& X_, torch::Tensor& y_);
        torch::Tensor transform_t(torch::Tensor& X_);
        torch::Tensor fit_transform_t(torch::Tensor& X_, torch::Tensor& y_);
        static inline std::string version() { return "1.2.2"; };
    protected:
        labels_t discretizedData = labels_t();
        cutPoints_t cutPoints;
    };
}
#endif
