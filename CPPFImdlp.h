#ifndef CPPFIMDLP_H
#define CPPFIMDLP_H
#include "typesFImdlp.h"
#include "Metrics.h"
#include <utility>
#include <string>
namespace mdlp {
    class CPPFImdlp {
    protected:
        size_t min_length;
        int depth, max_depth;
        float proposed_cuts;
        indices_t indices;
        samples_t X;
        labels_t y;
        Metrics metrics;
        cutPoints_t cutPoints;
        size_t num_cut_points;

        static indices_t sortIndices(samples_t&, labels_t&);
        void computeCutPoints(size_t, size_t, int);
        bool mdlp(size_t, size_t, size_t);
        size_t getCandidate(size_t, size_t);
        size_t compute_max_num_cut_points();
        pair<precision_t, size_t> valueCutPoint(size_t, size_t, size_t);
    public:
        CPPFImdlp();
        CPPFImdlp(size_t, int, float);
        ~CPPFImdlp();
        void fit(samples_t&, labels_t&);
        cutPoints_t getCutPoints();
        int get_depth();
        inline string version() { return "1.1.1"; };
    };
}
#endif
