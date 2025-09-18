#include "types.h"
#include <memory>

namespace seajay::search {
namespace {

thread_local std::unique_ptr<MoveList[]> t_moveScratch;
thread_local std::unique_ptr<TriangularPV[]> t_pvScratch;
thread_local std::unique_ptr<TriangularPV> t_rootPv;

constexpr int SCRATCH_PLY = KillerMoves::MAX_PLY;

void ensureScratchAllocated() {
    if (!t_moveScratch) {
        t_moveScratch = std::make_unique<MoveList[]>(SCRATCH_PLY);
    }
    if (!t_pvScratch) {
        t_pvScratch = std::make_unique<TriangularPV[]>(SCRATCH_PLY + 1);
    }
    if (!t_rootPv) {
        t_rootPv = std::make_unique<TriangularPV>();
    }
}

} // namespace

MoveList& getMoveScratch(int ply) {
    ensureScratchAllocated();
    return t_moveScratch[ply];
}

TriangularPV* getPvScratch(int ply) {
    ensureScratchAllocated();
    return &t_pvScratch[ply];
}

TriangularPV& getRootPvScratch() {
    ensureScratchAllocated();
    return *t_rootPv;
}

void resetScratchBuffers() {
    if (t_moveScratch) {
        for (int i = 0; i < SCRATCH_PLY; ++i) {
            t_moveScratch[i].clear();
        }
    }
    if (t_pvScratch) {
        for (int i = 0; i < SCRATCH_PLY + 1; ++i) {
            t_pvScratch[i].clear();
        }
    }
    if (t_rootPv) {
        t_rootPv->clear();
    }
}

} // namespace seajay::search
