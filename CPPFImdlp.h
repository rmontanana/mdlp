#ifndef CPPFIMDLP_H
#define CPPFIMDLP_H
#include "typesFImdlp.h"
#include "Metrics.h"
#include <utility>
#include <string>
namespace mdlp {
    class CPPFImdlp {
    protected:
        indices_t indices;
        samples_t X;
        labels_t y;
        Metrics metrics;
        cutPoints_t cutPoints;

        static indices_t sortIndices(samples_t&, labels_t&);
        void computeCutPoints(size_t, size_t);
        bool mdlp(size_t, size_t, size_t);
        size_t getCandidate(size_t, size_t);
        pair<precision_t, size_t> valueCutPoint(size_t, size_t, size_t);
    public:
        CPPFImdlp();
        ~CPPFImdlp();
        CPPFImdlp& fit(samples_t&, labels_t&);
        samples_t getCutPoints();
        inline string version() { return "1.1.0"; };
    };
}
#endif