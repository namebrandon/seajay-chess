#pragma once

#include "node_context.h"
#include "types.h"
#include "search_info.h"
#include "../evaluation/types.h"
#include <array>

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
    uint64_t ineligible = 0;    // Helper bailed due to insufficient depth
    uint64_t windowCollapsed = 0; // Helper saw clamped window collapse
    uint64_t cutoffs = 0;       // Future use: verification yielded cutoff
#else
    // Release builds keep structure trivially empty to avoid overhead
    uint8_t unused = 0;
#endif
};

namespace detail {
constexpr std::array<int, 64> build_singular_margin_table() noexcept {
    std::array<int, 64> margins{};
    for (std::size_t depth = 0; depth < margins.size(); ++depth) {
        if (depth >= 12) {
            margins[depth] = 32;
        } else if (depth >= 10) {
            margins[depth] = 36;
        } else if (depth >= 8) {
            margins[depth] = 40;
        } else if (depth >= 6) {
            margins[depth] = 48;
        } else if (depth >= 4) {
            margins[depth] = 56;
        } else {
            margins[depth] = 64;
        }
    }
    return margins;
}

constexpr auto kSingularMarginTable = build_singular_margin_table();
} // namespace detail

constexpr int kSingularVerificationReduction = 3; // Depth reduction applied during verification probes

// Clamp score into valid mate bounds for singular verification windows.
[[nodiscard]] constexpr eval::Score clamp_singular_score(eval::Score score) noexcept {
    const int minBound = -eval::Score::mate().value() + MAX_PLY;
    const int maxBound = eval::Score::mate().value() - MAX_PLY;
    const int raw = score.value();
    const int clamped = raw < minBound ? minBound : (raw > maxBound ? maxBound : raw);
    return eval::Score(clamped);
}

// Compile-time margin lookup for singular verification searches with adaptive adjustments.
[[nodiscard]] constexpr eval::Score singular_margin(
    int depth,
    int ttDepth,
    eval::Score ttScore,
    eval::Score beta) noexcept {
    if (depth < 0) {
        depth = 0;
    }
    const int maxIndex = static_cast<int>(detail::kSingularMarginTable.size()) - 1;
    if (depth > maxIndex) {
        depth = maxIndex;
    }
    int margin = detail::kSingularMarginTable[static_cast<std::size_t>(depth)];

    // Use TT depth gap to tighten margin when the stored node searched deeper.
    const int ttDepthGap = ttDepth - depth;
    if (ttDepthGap >= 2) {
        margin -= 8;
    } else if (ttDepthGap == 1) {
        margin -= 4;
    } else if (ttDepthGap <= -1) {
        margin += 4;
    }

    // Shrink margin when the TT score is close to beta; expand if it significantly undershoots.
    const int betaGap = beta.value() - ttScore.value();
    if (betaGap <= 0) {
        margin += 8;  // TT score already exceeds beta; keep verification conservative.
    } else if (betaGap <= 8) {
        margin -= 4;
    } else if (betaGap >= 48) {
        margin += 4;
    }

    // Deeper parent depth and verification horizon justify tighter margins.
    const int singularDepth = depth - 1 - kSingularVerificationReduction;
    if (singularDepth >= 12) {
        margin -= 4;
    }

    constexpr int kMinMargin = 4;
    constexpr int kMaxMargin = 96;
    if (margin < kMinMargin) {
        margin = kMinMargin;
    } else if (margin > kMaxMargin) {
        margin = kMaxMargin;
    }

    return eval::Score(margin);
}

// Stage SE1.1a: Verification helper scaffold (no-op until SE2/SE3 hook it up).
[[nodiscard]] eval::Score verify_exclusion(
    Board& board,
    NodeContext context,
    int depth,
    int ply,
    int ttDepth,
    eval::Score ttScore,
    eval::Score alpha,
    eval::Score beta,
    SearchInfo& searchInfo,
    SearchData& searchData,
    const SearchLimits& limits,
    TranspositionTable* tt,
    TriangularPV* pv,
    SingularVerifyStats* stats = nullptr);

} // namespace seajay::search
