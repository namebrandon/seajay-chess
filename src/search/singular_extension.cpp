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
    int ply,
    eval::Score ttScore,
    eval::Score alpha,
    eval::Score beta,
    SearchInfo& searchInfo,
    SearchData& searchData,
    const SearchLimits& limits,
    TranspositionTable* tt,
    TriangularPV* pv,
    SingularVerifyStats* stats) {
    constexpr int kVerificationReduction = 3;
    (void)alpha;

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

    const Move excludedMove = context.excludedMove();
    if (excludedMove == NO_MOVE) {
#ifdef DEBUG
        if (stats) {
            stats->ineligible++;
        }
#endif
        return eval::Score::zero();
    }

    // Stage SE2.1b: Build verification window around TT score using margin table.
    const eval::Score margin = singular_margin(depth);
    const int singularBetaRaw = ttScore.value() - margin.value();
    const eval::Score singularBeta = clamp_singular_score(eval::Score(singularBetaRaw));
    const eval::Score probeAlphaCandidate = clamp_singular_score(eval::Score(singularBeta.value() - 1));
    if (!(probeAlphaCandidate < singularBeta)) {
#ifdef DEBUG
        if (stats) {
            stats->windowCollapsed++;
        }
#endif
        return eval::Score::zero();
    }

    const eval::Score singularAlpha = probeAlphaCandidate;
    NodeContext verifyContext = makeExcludedContext(context, excludedMove);

    // Stage SE1.1d: Issue verification search via negamax (still a no-op null window result).
    TranspositionTable::StorePolicyGuard storeGuard(
        tt ? TranspositionTable::StorePolicy::Verification
           : TranspositionTable::StorePolicy::Primary);
    const eval::Score verificationScore = negamax(
        board,
        verifyContext,
        singularDepth,
        ply,
        singularAlpha,
        singularBeta,
        searchInfo,
        searchData,
        limits,
        tt,
        pv);

    return verificationScore;
}

} // namespace seajay::search
