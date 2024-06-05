#ifndef DISCRETIZER_H
#define DISCRETIZER_H

#include <string>
#include <algorithm>
#include "typesFImdlp.h"

namespace mdlp {
    class Discretizer {
    public:
        Discretizer() = default;
        virtual ~Discretizer() = default;
        virtual void fit(samples_t& X_, labels_t& y_) = 0;
        inline cutPoints_t getCutPoints() const { return cutPoints; };
        labels_t& transform(const samples_t& data)
        {
            discretizedData.clear();
            discretizedData.reserve(data.size());
            for (const precision_t& item : data) {
                auto upper = std::upper_bound(cutPoints.begin(), cutPoints.end(), item);
                discretizedData.push_back(upper - cutPoints.begin());
            }
            return discretizedData;
        };
        static inline std::string version() { return "1.1.3"; };
    protected:
        labels_t discretizedData = labels_t();
        cutPoints_t cutPoints;
    };
}
#endif
