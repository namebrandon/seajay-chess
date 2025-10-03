#pragma once

// Phase MP3: Incremental move picker built on top of staged selection.

#include "../core/types.h"
#include "../core/board.h"
#include "../core/move_list.h"
#include "../core/move_generation.h"
#include "history_heuristic.h"
#include "killer_moves.h"
#include "countermoves.h"
#include "countermove_history.h"
#include "move_ordering.h"
#include "types.h"  // For SearchData and SearchLimits

#include <array>

namespace seajay {
namespace search {

class RankedMovePicker {
public:
    enum class YieldCategory {
        None,
        TT,
        GoodCapture,
        Killer,
        CounterMove,
        Quiet,
        BadCapture,
    };

    RankedMovePicker(const Board& board,
                     Move ttMove,
                     const KillerMoves* killers,
                     const HistoryHeuristic* history,
                     const CounterMoves* counterMoves,
                     const CounterMoveHistory* counterMoveHistory,
                     Move prevMove,
                     int ply,
                     int depth,
                     int countermoveBonus,
                     const SearchLimits* limits,
                     SearchData* searchData = nullptr,
                     QuietOrderingRequest quietRequest = QuietOrderingRequest::Full);

    Move next();
    int currentYieldIndex() const { return static_cast<int>(m_yieldIndex); }
    bool wasInShortlist(Move move) const;
    YieldCategory lastYieldCategory() const { return m_lastYieldCategory; }

private:
    struct QuietEntry {
        Move move = NO_MOVE;
        int32_t score = 0;
        bool used = false;
    };

    void generateMoves();
    void prepareCaptures();
    void prepareKillers();
    void prepareQuiets();
    Move emitFromGoodCaptures();
    Move emitFromKillers();
    Move emitFromQuiets();
    Move emitFromBadCaptures();
    bool alreadyEmitted(Move move) const;
    void markEmitted(Move move, bool priorityStage);
    bool quietCandidateExists(Move move) const;
    bool quietCandidateIsLegal(Move move) const;
    int32_t quietHistoryScore(Move move) const;
    const char* quietStageLabel() const;
    void applyRootShortlistOrdering();

    const Board& m_board;
    MoveList m_moves;
    Move m_ttMove;
    const KillerMoves* m_killers;
    const HistoryHeuristic* m_history;
    const CounterMoves* m_counterMoves;
    const CounterMoveHistory* m_counterMoveHistory;
    Move m_prevMove;
    int m_ply;
    int m_depth;
    int m_countermoveBonus;
    const SearchLimits* m_limits;
    SearchData* m_searchData;
    QuietOrderingRequest m_quietRequest;

    MovePickerStage m_stage = MovePickerStage::TT;
    bool m_inCheck = false;

    std::array<Move, seajay::MAX_MOVES> m_captureMoves{};
    std::array<int, seajay::MAX_MOVES> m_captureScores{};
    std::array<std::size_t, seajay::MAX_MOVES> m_goodCaptureIndices{};
    std::array<std::size_t, seajay::MAX_MOVES> m_badCaptureIndices{};
    std::size_t m_goodCaptureCount = 0;
    std::size_t m_badCaptureCount = 0;
    std::size_t m_goodCaptureCursor = 0;
    std::size_t m_badCaptureCursor = 0;
    bool m_capturesPrepared = false;

    std::array<Move, seajay::MAX_MOVES> m_killerMoves{};
    std::array<uint8_t, seajay::MAX_MOVES> m_killerSources{}; // 0=killer, 1=countermove
    std::size_t m_killerCount = 0;
    std::size_t m_killerCursor = 0;

    std::array<QuietEntry, seajay::MAX_MOVES> m_quietPool{};
    std::size_t m_quietCount = 0;
    std::size_t m_quietCursor = 0;
    bool m_quietsPrepared = false;

    std::array<Move, seajay::MAX_MOVES> m_emitted{};
    std::size_t m_emittedCount = 0;
    std::array<Move, seajay::MAX_MOVES> m_priority{};
    std::size_t m_priorityCount = 0;

    std::size_t m_yieldIndex = 0;  // 1-based index of yielded moves
    YieldCategory m_lastYieldCategory = YieldCategory::None;
    bool m_reportedQuietStage = false;
};

class RankedMovePickerQS {
public:
    RankedMovePickerQS(const Board& board, Move ttMove);
    Move next();
    bool hasNext() const { return false; }

private:
    const Board& m_board;
    Move m_ttMove;
};

} // namespace search
} // namespace seajay
