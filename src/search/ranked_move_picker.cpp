#include "ranked_move_picker.h"

#include "move_ordering.h"
#include "../core/board_safety.h"
#include "../core/see.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <iostream>
#include <sstream>

namespace seajay {
namespace search {

namespace {

constexpr int HISTORY_GATING_DEPTH = 2;

struct DumpMoveOrderConfig {
    bool enabled = false;
    int plyLimit = 0;
    int countLimit = 0;
};

const DumpMoveOrderConfig& dumpMoveOrderConfig() {
    static const DumpMoveOrderConfig cfg = [] {
        DumpMoveOrderConfig config;
        const char* flag = std::getenv("MOVE_ORDER_DUMP");
        if (!flag || flag[0] == '\0') {
            return config;
        }
        config.enabled = true;
        config.plyLimit = 2;
        config.countLimit = 64;
        int parsedPly = 0;
        int parsedCount = 0;
        if (std::sscanf(flag, "%d:%d", &parsedPly, &parsedCount) >= 1) {
            if (parsedPly > 0) {
                config.plyLimit = parsedPly;
            }
            if (parsedCount > 0) {
                config.countLimit = parsedCount;
            }
        }
        return config;
    }();
    return cfg;
}

bool dumpMoveOrderEnabled() {
    return dumpMoveOrderConfig().enabled;
}

struct QuietStageLogConfig {
    bool enabled = false;
    int plyLimit = 0;
};

const QuietStageLogConfig& quietStageLogConfig() {
    static const QuietStageLogConfig cfg = [] {
        QuietStageLogConfig config;
        const char* flag = std::getenv("MOVE_PICKER_STAGE_LOG");
        if (!flag || flag[0] == '\0') {
            return config;
        }
        config.enabled = true;
        config.plyLimit = 2;
        int parsedPly = 0;
        if (std::sscanf(flag, "%d", &parsedPly) == 1 && parsedPly >= 0) {
            config.plyLimit = parsedPly;
        }
        return config;
    }();
    return cfg;
}

bool quietStageLogEnabled() {
    return quietStageLogConfig().enabled;
}

bool isQuietCandidate(Move move) noexcept {
    return !isPromotion(move) && !isCapture(move) && !isEnPassant(move);
}

bool moveInList(const MoveList& moves, Move target) {
    return std::find(moves.begin(), moves.end(), target) != moves.end();
}

int32_t rootQuietAdjustment(const Board& board, const SearchLimits* limits, Move move) {
    if (!limits) {
        return 0;
    }

    int32_t bonus = 0;
    Piece fromPiece = board.pieceAt(moveFrom(move));
    if (fromPiece != NO_PIECE && typeOf(fromPiece) == KING && !isCastling(move)) {
        bonus -= limits->rootKingPenalty;
    }

    const Square to = moveTo(move);
    if (to == 27 || to == 28 || to == 35 || to == 36) {
        if (fromPiece != NO_PIECE && typeOf(fromPiece) == PAWN) {
            bonus += 120;
        }
    }
    return bonus;
}

} // namespace

RankedMovePicker::RankedMovePicker(const Board& board,
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
                                   SearchData* searchData,
                                   QuietOrderingRequest quietRequest)
    : m_board(board)
    , m_ttMove(ttMove)
    , m_killers(killers)
    , m_history(history)
    , m_counterMoves(counterMoves)
    , m_counterMoveHistory(counterMoveHistory)
    , m_prevMove(prevMove)
    , m_ply(ply)
    , m_depth(depth)
    , m_countermoveBonus(countermoveBonus)
    , m_limits(limits)
    , m_searchData(searchData)
    , m_quietRequest(quietRequest) {
    generateMoves();
}

void RankedMovePicker::generateMoves() {
    m_stage = MovePickerStage::TT;
    m_goodCaptureCount = 0;
    m_badCaptureCount = 0;
    m_goodCaptureCursor = 0;
    m_badCaptureCursor = 0;
    m_capturesPrepared = false;
    m_killerCount = 0;
    m_killerCursor = 0;
    m_quietCount = 0;
    m_quietCursor = 0;
    m_quietsPrepared = false;
    m_emittedCount = 0;
    m_priorityCount = 0;
    m_yieldIndex = 0;

    MoveGenerator::generateMovesForSearch(m_board, m_moves, false);
    recordMoveListForStats(m_moves.size());

    m_inCheck = m_board.isAttacked(m_board.kingSquare(m_board.sideToMove()), ~m_board.sideToMove());

    applyRootShortlistOrdering();

    if (dumpMoveOrderEnabled() && m_ply <= dumpMoveOrderConfig().plyLimit) {
        static int dumpCount = 0;
        if (dumpCount < dumpMoveOrderConfig().countLimit) {
            std::ostringstream oss;
            oss << "info string PickerOrder ply=" << m_ply
                << " hash=" << m_board.zobristKey()
                << " moves:";
            for (const Move& move : m_moves) {
                oss << ' ' << SafeMoveExecutor::moveToString(move);
            }
            std::cout << oss.str() << std::endl;
            ++dumpCount;
        }
    }
}

bool RankedMovePicker::alreadyEmitted(Move move) const {
    for (std::size_t idx = 0; idx < m_emittedCount; ++idx) {
        if (m_emitted[idx] == move) {
            return true;
        }
    }
    return false;
}

void RankedMovePicker::markEmitted(Move move, bool priorityStage) {
    if (m_emittedCount < m_emitted.size()) {
        m_emitted[m_emittedCount++] = move;
    }
    if (priorityStage && m_priorityCount < m_priority.size()) {
        m_priority[m_priorityCount++] = move;
    }
}

bool RankedMovePicker::wasInShortlist(Move move) const {
    for (std::size_t idx = 0; idx < m_priorityCount; ++idx) {
        if (m_priority[idx] == move) {
            return true;
        }
    }
    return false;
}

bool RankedMovePicker::quietCandidateExists(Move move) const {
    return std::find_if(m_moves.begin(), m_moves.end(),
                        [&](const Move& candidate) {
                            return candidate == move && isQuietCandidate(candidate);
                        }) != m_moves.end();
}

bool RankedMovePicker::quietCandidateIsLegal(Move move) const {
    Square from = moveFrom(move);
    Piece piece = m_board.pieceAt(from);
    return piece != NO_PIECE && colorOf(piece) == m_board.sideToMove();
}

void RankedMovePicker::prepareCaptures() {
    if (m_capturesPrepared) {
        return;
    }

    std::size_t captureIdx = 0;
    for (const Move& move : m_moves) {
        if (!isCapture(move) && !isPromotion(move) && !isEnPassant(move)) {
            continue;
        }

        m_captureMoves[captureIdx] = move;
        m_captureScores[captureIdx] = MvvLvaOrdering::scoreMove(m_board, move);

#ifdef SEARCH_STATS
        if (m_limits && m_limits->showMovePickerStats && m_searchData) {
            m_searchData->movePickerStats.capturesTotal++;
        }
#endif

        bool good = false;
        if (isPromotion(move)) {
            good = true;
        } else if (m_captureScores[captureIdx] >= 0) {
            good = true;
        } else {
            good = seeGE(m_board, move, 0);
        }

        if (good) {
            m_goodCaptureIndices[m_goodCaptureCount++] = captureIdx;
        } else {
            m_badCaptureIndices[m_badCaptureCount++] = captureIdx;
        }
        ++captureIdx;
    }

    recordCapturePartitionForStats(captureIdx);
    m_capturesPrepared = true;
}

void RankedMovePicker::prepareKillers() {
    if (m_inCheck) {
        m_killerCount = 0;
        return;
    }

    m_killerCount = 0;

    auto enqueue = [&](Move move, uint8_t sourceTag) {
        if (move == NO_MOVE) {
            return;
        }
        if (!isQuietCandidate(move)) {
            return;
        }
        if (!quietCandidateIsLegal(move)) {
            return;
        }
        if (!quietCandidateExists(move)) {
            return;
        }
        for (std::size_t i = 0; i < m_killerCount; ++i) {
            if (m_killerMoves[i] == move) {
                return; // duplicate
            }
        }
        if (m_killerCount < m_killerMoves.size()) {
            m_killerMoves[m_killerCount] = move;
            m_killerSources[m_killerCount] = sourceTag;
            ++m_killerCount;
        }
    };

    if (m_killers) {
        enqueue(m_killers->getKiller(m_ply, 0), /*sourceTag=*/0);
        enqueue(m_killers->getKiller(m_ply, 1), /*sourceTag=*/0);
    }

    if (m_counterMoves && m_prevMove != NO_MOVE && m_countermoveBonus > 0) {
        enqueue(m_counterMoves->getCounterMove(m_prevMove), /*sourceTag=*/1);
    }
}

int32_t RankedMovePicker::quietHistoryScore(Move move) const {
    int32_t base = 0;
    const Color side = m_board.sideToMove();
    if (m_history) {
        base = m_history->getScore(side, moveFrom(move), moveTo(move));
    }

    bool useCMH = false;
    if (m_counterMoveHistory && m_prevMove != NO_MOVE && m_countermoveBonus > 0) {
        if (m_depth >= HISTORY_GATING_DEPTH) {
            useCMH = true;
        }
    }

    if (useCMH && m_counterMoveHistory) {
        float weight = 1.5f;
        if (m_limits) {
            weight = m_limits->counterMoveHistoryWeight;
        }
        const int32_t numerator = static_cast<int32_t>(weight * 2.0f + 0.5f);
        const int32_t cmhScore = m_counterMoveHistory->getScore(m_prevMove, move);
        constexpr int cmhDenominator = 2;
        return base * 3 + (cmhScore * numerator) / cmhDenominator;
    }

    return base * 2;
}

const char* RankedMovePicker::quietStageLabel() const {
    if (m_quietRequest == QuietOrderingRequest::ChecksOnly) {
        return "checks-only";
    }
    if (!m_history) {
        return "fallback";
    }
    if (m_counterMoveHistory && m_prevMove != NO_MOVE && m_countermoveBonus > 0 &&
        m_depth >= HISTORY_GATING_DEPTH) {
        return "cmh";
    }
    return "basic";
}

void RankedMovePicker::applyRootShortlistOrdering() {
    if (m_ply != 0) {
        return;
    }
    if (m_inCheck) {
        return; // evasions already handled separately
    }

    static MvvLvaOrdering rootOrdering;

    if (m_killers && m_history && m_counterMoves && m_counterMoveHistory && m_limits) {
        rootOrdering.orderMovesWithHistory(m_board,
                                           m_moves,
                                           *m_killers,
                                           *m_history,
                                           *m_counterMoves,
                                           *m_counterMoveHistory,
                                           m_prevMove,
                                           m_ply,
                                           m_countermoveBonus,
                                           m_limits->counterMoveHistoryWeight,
                                           m_quietRequest);
    } else if (m_killers && m_history && m_counterMoves) {
        rootOrdering.orderMovesWithHistory(m_board,
                                           m_moves,
                                           *m_killers,
                                           *m_history,
                                           *m_counterMoves,
                                           m_prevMove,
                                           m_ply,
                                           m_countermoveBonus,
                                           m_quietRequest);
    } else if (m_killers && m_history) {
        rootOrdering.orderMovesWithHistory(m_board,
                                           m_moves,
                                           *m_killers,
                                           *m_history,
                                           m_ply,
                                           m_quietRequest);
    } else {
        rootOrdering.orderMoves(m_board, m_moves);
    }
}

void RankedMovePicker::prepareQuiets() {
    if (m_quietsPrepared) {
        return;
    }

    std::array<uint8_t, seajay::MAX_MOVES> killerMask{};
    for (std::size_t idx = 0; idx < m_killerCount; ++idx) {
        for (std::size_t listIdx = 0; listIdx < m_moves.size(); ++listIdx) {
            if (m_moves[listIdx] == m_killerMoves[idx]) {
                killerMask[listIdx] = 1;
            }
        }
    }

    m_quietCount = 0;
    for (std::size_t idx = 0; idx < m_moves.size(); ++idx) {
        const Move move = m_moves[idx];
        if (!isQuietCandidate(move)) {
            continue;
        }
        if (killerMask[idx]) {
            continue; // handled in killer stage
        }
        auto& entry = m_quietPool[m_quietCount++];
        entry.move = move;
        entry.used = false;
        entry.score = quietHistoryScore(move);

        if (m_ply == 0) {
            entry.score += rootQuietAdjustment(m_board, m_limits, move);
        }
    }

    m_quietsPrepared = true;
}

Move RankedMovePicker::emitFromGoodCaptures() {
    while (m_goodCaptureCursor < m_goodCaptureCount) {
        std::size_t bestPos = m_goodCaptureCursor;
        int bestScore = m_captureScores[m_goodCaptureIndices[bestPos]];
        for (std::size_t pos = m_goodCaptureCursor + 1; pos < m_goodCaptureCount; ++pos) {
            const int candidateScore = m_captureScores[m_goodCaptureIndices[pos]];
            if (candidateScore > bestScore) {
                bestScore = candidateScore;
                bestPos = pos;
            }
        }

        std::swap(m_goodCaptureIndices[m_goodCaptureCursor], m_goodCaptureIndices[bestPos]);
        Move move = m_captureMoves[m_goodCaptureIndices[m_goodCaptureCursor++]];
        if (alreadyEmitted(move)) {
            continue;
        }
        markEmitted(move, true);
        m_lastYieldCategory = YieldCategory::GoodCapture;
        return move;
    }
    return NO_MOVE;
}

Move RankedMovePicker::emitFromKillers() {
    while (m_killerCursor < m_killerCount) {
        std::size_t sourceIndex = m_killerCursor;
        Move move = m_killerMoves[m_killerCursor++];
        if (alreadyEmitted(move)) {
            continue;
        }
        markEmitted(move, true);
        if (sourceIndex < m_killerSources.size() && m_killerSources[sourceIndex] == 1) {
            m_lastYieldCategory = YieldCategory::CounterMove;
        } else {
            m_lastYieldCategory = YieldCategory::Killer;
        }
        return move;
    }
    return NO_MOVE;
}

Move RankedMovePicker::emitFromQuiets() {
    if (!m_quietsPrepared) {
        prepareQuiets();
    }

    if (m_quietRequest == QuietOrderingRequest::ChecksOnly) {
        while (m_quietCursor < m_quietCount) {
            auto& entry = m_quietPool[m_quietCursor++];
            if (entry.used) {
                continue;
            }
            entry.used = true;
            if (alreadyEmitted(entry.move)) {
                continue;
            }
            markEmitted(entry.move, false);
#ifdef SEARCH_STATS
            if (m_limits && m_limits->showMovePickerStats && m_searchData) {
                m_searchData->movePickerStats.remainderYields++;
            }
#endif
            m_lastYieldCategory = YieldCategory::Quiet;
            return entry.move;
        }
        return NO_MOVE;
    }

    std::size_t bestIdx = seajay::MAX_MOVES;
    int32_t bestScore = std::numeric_limits<int32_t>::min();
    for (std::size_t idx = 0; idx < m_quietCount; ++idx) {
        auto& entry = m_quietPool[idx];
        if (entry.used) {
            continue;
        }
        if (alreadyEmitted(entry.move)) {
            entry.used = true;
            continue;
        }
        if (entry.score > bestScore || (entry.score == bestScore && idx < bestIdx)) {
            bestScore = entry.score;
            bestIdx = idx;
        }
    }

    if (bestIdx == seajay::MAX_MOVES) {
        return NO_MOVE;
    }

    m_quietPool[bestIdx].used = true;
    if (quietStageLogEnabled() && !m_reportedQuietStage &&
        m_ply <= quietStageLogConfig().plyLimit) {
        std::cout << "info string QuietStage ply=" << m_ply
                  << " stage=" << quietStageLabel()
                  << " hash=" << m_board.zobristKey()
                  << " move=" << SafeMoveExecutor::moveToString(m_quietPool[bestIdx].move)
                  << std::endl;
        m_reportedQuietStage = true;
    }
    markEmitted(m_quietPool[bestIdx].move, false);
#ifdef SEARCH_STATS
    if (m_limits && m_limits->showMovePickerStats && m_searchData) {
        m_searchData->movePickerStats.remainderYields++;
    }
#endif
    m_lastYieldCategory = YieldCategory::Quiet;
    return m_quietPool[bestIdx].move;
}

Move RankedMovePicker::emitFromBadCaptures() {
    while (m_badCaptureCursor < m_badCaptureCount) {
        std::size_t bestPos = m_badCaptureCursor;
        int bestScore = m_captureScores[m_badCaptureIndices[bestPos]];
        for (std::size_t pos = m_badCaptureCursor + 1; pos < m_badCaptureCount; ++pos) {
            const int candidateScore = m_captureScores[m_badCaptureIndices[pos]];
            if (candidateScore > bestScore) {
                bestScore = candidateScore;
                bestPos = pos;
            }
        }

        std::swap(m_badCaptureIndices[m_badCaptureCursor], m_badCaptureIndices[bestPos]);
        Move move = m_captureMoves[m_badCaptureIndices[m_badCaptureCursor++]];
        if (alreadyEmitted(move)) {
            continue;
        }
        markEmitted(move, false);
#ifdef SEARCH_STATS
        if (m_limits && m_limits->showMovePickerStats && m_searchData) {
            m_searchData->movePickerStats.remainderYields++;
        }
#endif
        m_lastYieldCategory = YieldCategory::BadCapture;
        return move;
    }
    return NO_MOVE;
}

Move RankedMovePicker::next() {
    while (true) {
        switch (m_stage) {
            case MovePickerStage::TT: {
                m_stage = MovePickerStage::GenerateGoodCaptures;
                if (m_ttMove == NO_MOVE) {
                    continue;
                }
                if (!moveInList(m_moves, m_ttMove)) {
                    continue;
                }
                if (alreadyEmitted(m_ttMove)) {
                    continue;
                }
                markEmitted(m_ttMove, true);
#ifdef SEARCH_STATS
                if (m_limits && m_limits->showMovePickerStats && m_searchData) {
                    m_searchData->movePickerStats.ttFirstYield++;
                }
#endif
                ++m_yieldIndex;
                m_lastYieldCategory = YieldCategory::TT;
                return m_ttMove;
            }

            case MovePickerStage::GenerateGoodCaptures: {
                prepareCaptures();
                m_stage = MovePickerStage::EmitGoodCaptures;
                continue;
            }

            case MovePickerStage::EmitGoodCaptures: {
                Move move = emitFromGoodCaptures();
                if (move != NO_MOVE) {
                    ++m_yieldIndex;
                    return move;
                }
                m_stage = MovePickerStage::GenerateKillers;
                continue;
            }

            case MovePickerStage::GenerateKillers: {
                prepareKillers();
                m_stage = MovePickerStage::EmitKillers;
                continue;
            }

            case MovePickerStage::EmitKillers: {
                Move move = emitFromKillers();
                if (move != NO_MOVE) {
                    ++m_yieldIndex;
                    return move;
                }
                m_stage = MovePickerStage::GenerateQuiets;
                continue;
            }

            case MovePickerStage::GenerateQuiets: {
                prepareQuiets();
                m_stage = MovePickerStage::EmitQuiets;
                continue;
            }

            case MovePickerStage::EmitQuiets: {
                Move move = emitFromQuiets();
                if (move != NO_MOVE) {
                    ++m_yieldIndex;
                    return move;
                }
                m_stage = MovePickerStage::GenerateBadCaptures;
                continue;
            }

            case MovePickerStage::GenerateBadCaptures: {
                // Captures were already prepared earlier
                m_stage = MovePickerStage::EmitBadCaptures;
                continue;
            }

            case MovePickerStage::EmitBadCaptures: {
                Move move = emitFromBadCaptures();
                if (move != NO_MOVE) {
                    ++m_yieldIndex;
                    return move;
                }
                m_stage = MovePickerStage::End;
                continue;
            }

            case MovePickerStage::End:
            default:
                return NO_MOVE;
        }
    }
}

RankedMovePickerQS::RankedMovePickerQS(const Board& board, Move ttMove)
    : m_board(board)
    , m_ttMove(ttMove) {}

Move RankedMovePickerQS::next() {
    (void)m_board;
    (void)m_ttMove;
    return NO_MOVE;
}

} // namespace search
} // namespace seajay
