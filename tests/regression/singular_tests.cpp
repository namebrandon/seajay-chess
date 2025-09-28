#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "core/board.h"
#include "core/transposition_table.h"
#include "core/magic_bitboards.h"
#include "evaluation/pawn_structure.h"
#include "search/lmr.h"
#include "search/negamax.h"
#include "search/search.h"
#include "search/types.h"

using namespace seajay;
using namespace seajay::search;

struct SingularRegressionCase {
    std::string name;
    std::string fen;
};

namespace {

SearchLimits makeLimits(bool enableSingular, std::vector<SingularDebugEvent>* sink) {
    SearchLimits limits;
    limits.maxDepth = 12;
    limits.useSearchNodeAPIRefactor = true;
    limits.enableExcludedMoveParam = true;
    limits.useSingularExtensions = enableSingular;
    limits.allowStackedExtensions = true;
    limits.bypassSingularTTExact = false;
    limits.disableCheckDuringSingular = false;
    limits.singularDepthMin = 7;
    limits.singularMarginBase = 51;
    limits.singularVerificationReduction = 4;
    limits.singularExtensionDepth = 2;
    limits.useQuiescence = true;
    limits.useRankedMovePicker = true;
    limits.useRankAwareGates = true;
    limits.singularDebugLog = sink != nullptr;
    limits.singularDebugSink = sink;
    limits.singularDebugMaxEvents = 128;
    limits.usePhaseStability = false;
    limits.movetime = std::chrono::milliseconds(1000);
    limits.time[WHITE] = std::chrono::milliseconds(0);
    limits.time[BLACK] = std::chrono::milliseconds(0);
    return limits;
}

bool runTestCase(const SingularRegressionCase& testCase) {
    std::cout << "\n=== " << testCase.name << " ===" << std::endl;

    Board board;
    if (!board.fromFEN(testCase.fen)) {
        std::cerr << "Failed to parse FEN: " << testCase.fen << std::endl;
        return false;
    }
    board.clearGameHistory();

    TranspositionTable tt(16);

    // Baseline (singular disabled)
    std::vector<SingularDebugEvent> baselineEvents;
    {
        auto limits = makeLimits(false, &baselineEvents);
        baselineEvents.clear();
        searchIterativeTest(board, limits, &tt);
    }

    if (!baselineEvents.empty()) {
        std::cerr << "[FAIL] Singular disabled but debug produced "
                  << baselineEvents.size() << " events" << std::endl;
        return false;
    }
    std::cout << "[OK] Singular disabled produced no events." << std::endl;

    // Reset board/TT for singular-enabled run
    board.fromFEN(testCase.fen);
    board.clearGameHistory();
    tt.clear();

    std::vector<SingularDebugEvent> singularEvents;
    {
        auto limits = makeLimits(true, &singularEvents);
        singularEvents.clear();
        searchIterativeTest(board, limits, &tt);
    }

    const bool hasVerification = !singularEvents.empty();
    const auto extensionCount = std::count_if(
        singularEvents.begin(), singularEvents.end(),
        [](const SingularDebugEvent& evt) {
            return evt.extensionAmount > 0;
        }
    );

    if (!hasVerification) {
        std::cerr << "[FAIL] Singular enabled but no verification events recorded." << std::endl;
        return false;
    }

    std::cout << "[OK] Singular enabled recorded " << singularEvents.size()
              << " events with " << extensionCount << " extensions." << std::endl;
    return true;
}

} // namespace

int main() {
    // Mirror UCI engine initialization for deterministic search
    seajay::magic_v2::initMagics();
    PawnStructure::initPassedPawnMasks();
    search::initLMRTable();

    const std::vector<SingularRegressionCase> cases = {
        {"Karpov-Kasparov 1985 G16", "8/8/4kpp1/3p1b2/p6P/2B5/6P1/6K1 b - - 0 47"},
        {"Deep defensive resource", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"},
        {"Endgame pawn race", "8/5pk1/4p1p1/7P/5P2/6K1/8/8 w - - 0 1"},
        {"Complex middlegame", "r1bqkb1r/pp3ppp/2n1pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R b KQkq - 0 6"},
        {"Singular exchange sac", "3r1rk1/p1q2pbp/1np1p1p1/8/2PNP3/1P3P2/PB3QPP/3R1RK1 w - - 0 1"}
    };

    bool allPassed = true;
    for (const auto& testCase : cases) {
        if (!runTestCase(testCase)) {
            allPassed = false;
        }
    }

    if (!allPassed) {
        std::cerr << "\nSingular regression tests FAILED." << std::endl;
        return 1;
    }

    std::cout << "\nAll singular regression tests passed." << std::endl;
    return 0;
}
