#include "singular_extension.h"

#include "negamax.h"          // For TriangularPV forward references
#include "search_info.h"      // Stage 6: context scaffolding
#include "iterative_search_data.h" // For SearchData definition
#include "../core/board.h"
#include "../core/transposition_table.h"

namespace seajay::search {

eval::Score verify_exclusion(
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
    SingularVerifyStats* stats) {
    (void)board;
    (void)context;
    constexpr int kVerificationReduction = 3;
    (void)alpha;
    (void)beta;
    (void)searchInfo;
    (void)searchData;
    (void)tt;
    (void)pv;

    const bool singularDisabled = !limits.useSingularExtensions;
    const bool excludedParamDisabled = !limits.enableExcludedMoveParam;
    if ((singularDisabled || excludedParamDisabled)) [[unlikely]] {
#ifdef DEBUG
        if (stats) {
            stats->bypassed++;
        }
#endif
        return eval::Score::zero();
    }

#ifdef DEBUG
    if (stats) {
        stats->invoked++;
    }
#endif

    // Stage SE1.1b: Clamp verification depth before issuing a reduced search.
    const int singularDepth = depth - 1 - kVerificationReduction;
    if (singularDepth <= 0) {
#ifdef DEBUG
        if (stats) {
            stats->ineligible++;
        }
#endif
        return eval::Score::zero();
    }

    // Stage SE1.1a: Skeleton helper – full verification search will land in later stages.
    return eval::Score::zero();
}

} // namespace seajay::search
