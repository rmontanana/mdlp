// ****************************************************************
// SPDX - FileCopyrightText: Copyright 2024 Ricardo Montañana Gómez
// SPDX - FileType: SOURCE
// SPDX - License - Identifier: MIT
// ****************************************************************

#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <map>
#include <stdexcept>

using namespace std;
namespace mdlp {
    typedef float precision_t;
    typedef int label_t;
    typedef std::vector<precision_t> samples_t;
    typedef std::vector<label_t> labels_t;
    typedef std::vector<size_t> indices_t;
    typedef std::vector<precision_t> cutPoints_t;
    // Keys are size_t to match the interval indices used throughout the library.
    // Narrower keys silently truncated those indices, so distinct intervals could
    // collide onto the same cache entry past INT_MAX elements.
    typedef std::map<std::pair<size_t, size_t>, precision_t> cacheEnt_t;
    typedef std::map<std::tuple<size_t, size_t, size_t>, precision_t> cacheIg_t;

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
