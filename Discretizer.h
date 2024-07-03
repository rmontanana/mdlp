// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef DISCRETIZER_H
#define DISCRETIZER_H

#include <string>
#include <algorithm>
#include <torch/torch.h>
#include "typesFImdlp.h"

namespace mdlp {
    enum class bound_dir_t {
        LEFT,
        RIGHT
    };
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
        static inline std::string version() { return "1.2.3"; };
    protected:
        labels_t discretizedData = labels_t();
        cutPoints_t cutPoints; // At least two cutpoints must be provided, the first and the last will be ignored in transform
        bound_dir_t direction; // used in transform
    };
}
#endif
