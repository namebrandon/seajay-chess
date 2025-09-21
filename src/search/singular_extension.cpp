#include "singular_extension.h"

#include "negamax.h"          // For TriangularPV forward references
#include "search_info.h"      // Stage 6: context scaffolding
#include "iterative_search_data.h" // For SearchData definition
#include "../core/board.h"
#include "../core/transposition_table.h"

namespace seajay::search {

namespace {
    template<bool EnableSingular>
    ALWAYS_INLINE eval::Score verify_exclusion_impl(
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
        (void)depth;
        (void)alpha;
        (void)beta;
        (void)searchInfo;
        (void)searchData;
        (void)limits;
        (void)tt;
        (void)pv;
        if constexpr (!EnableSingular) {
#ifdef DEBUG
            if (stats) {
                stats->bypassed++;
            }
#endif
            return eval::Score::zero();
        } else {
#ifdef DEBUG
            if (stats) {
                stats->invoked++;
            }
#endif
            // Future implementation will reuse negamax with reduced depth and narrow window
            // For now, return neutral score to preserve NoOp behaviour
            return eval::Score::zero();
        }
    }
} // namespace

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
    constexpr bool kSingularEnabled = false;
    return verify_exclusion_impl<kSingularEnabled>(
        board,
        context,
        depth,
        alpha,
        beta,
        searchInfo,
        searchData,
        limits,
        tt,
        pv,
        stats);
}

} // namespace seajay::search

