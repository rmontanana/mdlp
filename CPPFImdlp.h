#ifndef CPPFIMDLP_H
#define CPPFIMDLP_H
#include "typesFImdlp.h"
#include "Metrics.h"
#include <utility>
#include <tuple>
#include <string>
namespace mdlp {
    class CPPFImdlp {
    protected:
        int algorithm;
        indices_t indices; // sorted indices to use with X and y
        samples_t X;
        labels_t y;
        Metrics metrics;
        cutPoints_t cutPoints;

        static indices_t sortIndices(samples_t&);
        static indices_t sortIndices2(samples_t&, labels_t&);
        void computeCutPoints(size_t, size_t);
        bool mdlp(size_t, size_t, size_t);
        size_t getCandidate(size_t, size_t);
        size_t getCandidateWeka(size_t, size_t);
        void computeCutPointsAlternative(size_t, size_t);
        void computeCutPointsProposal(size_t, size_t);
        precision_t value_cut_point(size_t, size_t);
        tuple<precision_t, size_t> value_proposal_cut_point(size_t, size_t, size_t);

    public:
        CPPFImdlp(int);
        ~CPPFImdlp();
        CPPFImdlp& fit(samples_t&, labels_t&);
        samples_t getCutPoints();
        inline string version() { return "0.8.1"; };
    };
}
#endif