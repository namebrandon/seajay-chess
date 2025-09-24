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
        if (depth >= 8) {
            margins[depth] = 60;
        } else if (depth >= 6) {
            margins[depth] = 80;
        } else {
            margins[depth] = 100;
        }
    }
    return margins;
}

constexpr auto kSingularMarginTable = build_singular_margin_table();
} // namespace detail

// Compile-time margin lookup for singular verification searches.
[[nodiscard]] constexpr eval::Score singular_margin(int depth) noexcept {
    if (depth < 0) {
        depth = 0;
    }
    const int maxIndex = static_cast<int>(detail::kSingularMarginTable.size()) - 1;
    if (depth > maxIndex) {
        depth = maxIndex;
    }
    return eval::Score(detail::kSingularMarginTable[static_cast<std::size_t>(depth)]);
}

// Stage SE1.1a: Verification helper scaffold (no-op until SE2/SE3 hook it up).
[[nodiscard]] ALWAYS_INLINE eval::Score verify_exclusion(
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
    SingularVerifyStats* stats = nullptr);

} // namespace seajay::search
