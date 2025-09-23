#pragma once

#include "node_context.h"
#include "types.h"
#include "search_info.h"
#include "../evaluation/types.h"

namespace seajay {
class Board;
class TranspositionTable;
}

namespace seajay::search {

struct SearchData;
struct SearchLimits;
class TriangularPV;

struct SingularVerifyStats {
#ifdef DEBUG
    uint64_t bypassed = 0;      // Helper exited early (feature disabled)
    uint64_t invoked = 0;       // Helper entered verification search
    uint64_t cutoffs = 0;       // Future use: verification yielded cutoff
#else
    // Release builds keep structure trivially empty to avoid overhead
    uint8_t unused = 0;
#endif
};

// Stage SE1.1a: Verification helper scaffold (no-op until SE2/SE3 hook it up).
[[nodiscard]] ALWAYS_INLINE eval::Score verify_exclusion(
    Board& board,
    NodeContext context,
    int depth,
    eval::Score alpha,
    eval::Score beta,
    SearchInfo& searchInfo,
    SearchData& searchData,
    const SearchLimits& limits,
    TranspositionTable* tt,
    TriangularPV* pv,
    SingularVerifyStats* stats = nullptr);

} // namespace seajay::search
