// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef TYPES_H
#define TYPES_H

#include <cstddef>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace mdlp {
    using precision_t = float;
    using label_t = int;
    using samples_t = std::vector<precision_t>;
    using labels_t = std::vector<label_t>;
    using indices_t = std::vector<size_t>;
    using cutPoints_t = std::vector<precision_t>;
    // Keys are size_t to match the interval indices used throughout the library.
    // Narrower keys silently truncated those indices, so distinct intervals could
    // collide onto the same cache entry past INT_MAX elements.
    using cacheEnt_t = std::map<std::pair<size_t, size_t>, precision_t>;
    using cacheIg_t = std::map<std::tuple<size_t, size_t, size_t>, precision_t>;

    // Strategy enums live here rather than beside the classes that use them, so
    // Config.h can name them without including those classes and creating a
    // cycle. Including BinDisc.h or PKIDisc.h still brings them in, as before.

    /** @brief How BinDisc places its bin edges */
    enum class strategy_t {
        UNIFORM,  ///< Equal width
        QUANTILE  ///< Equal frequency
    };

    /** @brief How PKIDisc derives its bin count from the sample count */
    enum class compute_strategy_t {
        LOG,  ///< log(n)
        SQRT  ///< sqrt(n)
    };
}
#endif
