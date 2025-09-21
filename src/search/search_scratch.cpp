#include "types.h"
#include <array>

namespace seajay::search {
namespace {

constexpr int SCRATCH_PLY = KillerMoves::MAX_PLY;

// Pre-allocated thread-local scratch buffers avoid heap churn and
// guarantee destruction happens after all in-flight references are done.
thread_local std::array<MoveList, SCRATCH_PLY> t_moveScratch;
thread_local std::array<TriangularPV, SCRATCH_PLY + 1> t_pvScratch;
thread_local TriangularPV t_rootPv;

} // namespace

MoveList& getMoveScratch(int ply) {
    return t_moveScratch[ply];
}

TriangularPV* getPvScratch(int ply) {
    return &t_pvScratch[ply];
}

TriangularPV& getRootPvScratch() {
    return t_rootPv;
}

void resetScratchBuffers() {
    for (int i = 0; i < SCRATCH_PLY; ++i) {
        t_moveScratch[i].clear();
    }
    for (int i = 0; i < SCRATCH_PLY + 1; ++i) {
        t_pvScratch[i].clear();
    }
    t_rootPv.clear();
}

} // namespace seajay::search
