#include "negamax.h"
#include "search_info.h"
#include "iterative_search_data.h"  // Stage 13 addition
#include "time_management.h"        // Stage 13, Deliverable 2.2a
#include "aspiration_window.h"       // Stage 13, Deliverable 3.2b
#include "window_growth_mode.h"      // Stage 13 Remediation Phase 4
#include "game_phase.h"              // Stage 13 Remediation Phase 4
#include "move_ordering.h"  // Stage 11: MVV-LVA ordering (always enabled)
#include "ranked_move_picker.h"      // Phase 2a: Ranked move picker
#include <optional>                   // For stack-allocated RankedMovePicker
#include "lmr.h"            // Stage 18: Late Move Reductions
#include "principal_variation.h"     // PV tracking infrastructure
#include "countermove_history.h"     // Phase 4.3.a: Counter-move history
#include "node_explosion_stats.h"     // Node explosion diagnostics
#include "singular_extension.h"       // Stage 6d: verification helper scaffold
#include "../core/board.h"
#include "../core/board_safety.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../core/engine_config.h"    // Phase 4: Runtime configuration
#include "../evaluation/evaluate.h"
#include "../evaluation/eval_trace.h"
#include "../uci/info_builder.h"     // Phase 5: Structured info building
#include "quiescence.h"  // Stage 14: Quiescence search
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <vector>
#include <memory>
#include <sstream>
#include <string>

namespace seajay::search {

namespace {
constexpr int HISTORY_GATING_DEPTH = 2;
constexpr int AGGRESSIVE_NULL_MARGIN_OFFSET = 120; // Phase 4.2: extra margin over standard null pruning

// TODO(SE1): Delete once legacy negamax is removed and NodeContext handles singular exclusions end-to-end.
struct ExcludedMoveGuard {
    SearchInfo& info;
    int ply;
    Move previous;
    bool shouldRestore;

    ExcludedMoveGuard(SearchInfo& info, int ply, Move previous, bool shouldRestore)
        : info(info), ply(ply), previous(previous), shouldRestore(shouldRestore) {}

    ~ExcludedMoveGuard() {
        if (shouldRestore) {
            info.setExcludedMove(ply, previous);
        }
    }
};
}


// Phase 3.2: Helper for move generation with lazy legality checking option
inline MoveList generateLegalMoves(const Board& board) {
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    return moves;
}

inline MoveList generateMovesForSearch(const Board& board, bool checkLegal = false) {
    MoveList moves;
    MoveGenerator::generateMovesForSearch(board, moves, checkLegal);
    return moves;
}

// Simple move ordering without MVV-LVA (fallback)
template<typename MoveContainer>
inline void orderMovesSimple(MoveContainer& moves) noexcept {
    auto first = moves.begin();
    auto last = moves.end();
    auto partition_point = first;
    
    // Phase 1: Move all promotions to the front
    // Queen promotions have highest priority
    for (auto it = first; it != last; ++it) {
        if (isPromotion(*it)) {
            // If it's a queen promotion, move it to the very front
            if (promotionType(*it) == QUEEN) {
                if (it != first) {
                    // Move queen promotion to front
                    Move temp = *it;
                    std::move_backward(first, it, it + 1);
                    *first = temp;
                    partition_point = std::next(first);
                }
            } else if (it != partition_point) {
                // Other promotions go after queen promotions
                std::iter_swap(it, partition_point);
                ++partition_point;
            } else {
                ++partition_point;
            }
        }
    }
    
    // Phase 2: Move captures after promotions
    for (auto it = partition_point; it != last; ++it) {
        if (isCapture(*it) && !isPromotion(*it)) {  // Skip promotion-captures (already ordered)
            if (it != partition_point) {
                std::iter_swap(it, partition_point);
            }
            ++partition_point;
        }
    }
    
    // Quiet moves remain at the end (no need to explicitly order them)
}

// Move ordering function for alpha-beta pruning efficiency
// Orders moves in-place: TT move first, then promotions, then captures (MVV-LVA), then killers, then quiet moves
template<typename MoveContainer>
inline void orderMoves(const Board& board, MoveContainer& moves, Move ttMove = NO_MOVE, 
                      SearchData* searchData = nullptr, int ply = 0,
                      Move prevMove = NO_MOVE, int countermoveBonus = 0,
                      const SearchLimits* limits = nullptr, int depth = 0) noexcept {
    // Stage 11: Base MVV-LVA ordering (always available)
    // Stage 15: Optional SEE ordering for captures when enabled via UCI
    // Stage 19/20/23: Killers, history, countermoves for quiets
    static MvvLvaOrdering mvvLva;
    
    // Phase 4.1: Depth gating - use CMH from depth >= HISTORY_GATING_DEPTH
    // Below that, fall back to basic history+countermove ordering
    // Optional SEE capture ordering first (prefix-only) when enabled
    if (search::g_seeMoveOrdering.getMode() != search::SEEMode::OFF) {
        // SEE policy orders only captures/promotions prefix; quiets preserved
        search::g_seeMoveOrdering.orderMoves(board, moves);
    } else {
        // Default capture ordering
        mvvLva.orderMoves(board, moves);
    }

    if (searchData) {
        searchData->clearHistoryContext(ply);
    }

    if (depth >= HISTORY_GATING_DEPTH && searchData != nullptr && searchData->killers && searchData->history && 
        searchData->counterMoves && searchData->counterMoveHistory) {
        searchData->registerHistoryApplication(ply, SearchData::HistoryContext::Counter);
        // Use counter-move history for enhanced move ordering at sufficient depth
        // Get CMH weight from search limits - should always be available
        float cmhWeight = 1.5f;  // default fallback (shouldn't be hit)
        if (limits != nullptr) {
            cmhWeight = limits->counterMoveHistoryWeight;
        } else {
            // This shouldn't happen in normal search - log if in debug mode
            #ifdef DEBUG
            std::cerr << "Warning: SearchLimits not available in orderMoves, using default CMH weight" << std::endl;
            #endif
        }
        mvvLva.orderMovesWithHistory(board, moves, *searchData->killers, *searchData->history,
                                    *searchData->counterMoves, *searchData->counterMoveHistory,
                                    prevMove, ply, countermoveBonus, cmhWeight);
    } else if (searchData != nullptr && searchData->killers && searchData->history && searchData->counterMoves) {
        searchData->registerHistoryApplication(ply, SearchData::HistoryContext::Basic);
        // Fallback to basic countermoves without history
        mvvLva.orderMovesWithHistory(board, moves, *searchData->killers, *searchData->history,
                                    *searchData->counterMoves, prevMove, ply, countermoveBonus);
    } else if (searchData != nullptr) {
        searchData->clearHistoryContext(ply);
    }

    // Root-specific quiet move tweaks: prefer checks and central pawn pushes
    // Phase 2.2: Apply toggleable king penalty, excluding castling
    if (ply == 0 && !moves.empty() && limits != nullptr) {
        // Find where quiet moves start (after captures/promotions)
        auto quietStart = std::find_if(moves.begin(), moves.end(),
            [](const Move& m) { return !isPromotion(m) && !isCapture(m) && !isEnPassant(m); });
        if (quietStart != moves.end()) {
            auto scoreQuiet = [&board, limits](const Move& m) -> int {
                int score = 0;
                
                // Phase 2.2: Penalize non-capturing, non-castling king moves at root
                // Use UCI-controlled penalty value
                Piece fromPiece = board.pieceAt(moveFrom(m));
                if (fromPiece != NO_PIECE && typeOf(fromPiece) == KING) {
                    // Explicitly exclude castling from penalty
                    if (!isCastling(m)) {
                        score -= limits->rootKingPenalty;
                    }
                }
                
                // Central pawn push bonus (d4/e4/d5/e5)
                Square to = moveTo(m);
                if (to == 27 || to == 28 || to == 35 || to == 36) {
                    if (fromPiece != NO_PIECE && typeOf(fromPiece) == PAWN) {
                        score += 120;
                    }
                }
                return score;
            };
            std::stable_sort(quietStart, moves.end(),
                [&](const Move& a, const Move& b) { return scoreQuiet(a) > scoreQuiet(b); });
        }
    }

    // Sub-phase 4E: TT Move Ordering
    // Put TT move first AFTER all other ordering (only do this once!)
    if (ttMove != NO_MOVE) {
        auto it = std::find(moves.begin(), moves.end(), ttMove);
        if (it != moves.end() && it != moves.begin()) {
            // Move TT move to front
            Move temp = *it;
            std::move_backward(moves.begin(), it, it + 1);
            *moves.begin() = temp;
        }
    }
}

// Mate score constants for TT integration
constexpr int MATE_SCORE = 30000;
constexpr int MATE_BOUND = 29000;
constexpr int SINGULAR_DEPTH_MIN = 8;
constexpr int STACKED_RECAPTURE_MIN_DEPTH = 10;      // Guard: only stack when search is sufficiently deep
constexpr int STACKED_RECAPTURE_EVAL_MARGIN_CP = 96; // Require static eval within ~1 pawn of beta
constexpr int STACKED_RECAPTURE_TT_MARGIN = 1;       // TT entry must exceed current depth by this margin

#include "negamax_legacy.inc"

// Core negamax search implementation
eval::Score negamax(Board& board,
                   NodeContext context,
                   int depth,
                   int ply,
                   eval::Score alpha,
                   eval::Score beta,
                   SearchInfo& searchInfo,
                   SearchData& info,
                   const SearchLimits& limits,
                   TranspositionTable* tt,
                   TriangularPV* pv) {
    if (!limits.useSearchNodeAPIRefactor) {
        return negamax_legacy(board, depth, ply, alpha, beta, searchInfo, info, limits, tt, pv, context.isPv());
    }

    const bool isPvNode = context.isPv();

#ifdef DEBUG
    if (ply == 0) {
        assert(context.isRoot() && "Root context flag lost at ply 0");
        assert(isPvNode && "Root node must be treated as PV");
    } else {
        assert(!context.isRoot() && "Non-root ply flagged as root in context");
    }
#endif

#ifdef DEBUG
    if (!isPvNode) {
        assert(!context.hasExcludedMove() && "Excluded move should only appear on PV-like nodes in future stages");
    }
#endif

    // Debug output at root for deeper searches
    if (ply == 0 && depth >= 4 && !limits.suppressDebugOutput) {
        std::cerr << "Negamax: Starting search at depth " << depth << std::endl;
    }
    
    // Debug assertions
    assert(depth >= 0);
    assert(ply >= 0);
    assert(ply < 128);  // Prevent stack overflow
    
    // Debug assertion for valid search window
    assert(alpha < beta && "Alpha must be less than beta in negamax search");
    
    // In release mode, still handle invalid window gracefully
    if (alpha >= beta) {
        // This shouldn't happen in correct negamax, but handle it gracefully
        return alpha;  // Return alpha as a fail-soft bound
    }
    
    // Time check - only check periodically to reduce overhead
    if ((info.nodes & (SearchData::TIME_CHECK_INTERVAL - 1)) == 0) {
        // Check external stop flag first (UCI stop command or LazySMP global stop)
        if (limits.stopFlag && limits.stopFlag->load(std::memory_order_relaxed)) {
            info.stopped = true;
            return eval::Score::zero();
        }
        // Then check time limit
        if (info.checkTime()) {
            info.stopped = true;
            return eval::Score::zero();
        }
    }
    
    // Increment node counter
    info.nodes++;
    
    // Node explosion diagnostics: Track depth distribution
    if (limits.nodeExplosionDiagnostics) {
        g_nodeExplosionStats.recordNodeAtDepth(ply);
    }
    
    
    // Phase 1 & 2, enhanced in Phase 6: Check for periodic UCI info updates with smart throttling
    // Check periodically regardless of ply depth (since we spend most time at deeper plies)
    if (info.nodes > 0 && (info.nodes & 0xFFF) == 0) {
        // Use virtual method instead of expensive dynamic_cast
        // This eliminates RTTI overhead in the hot path
        if (info.isIterativeSearch()) {
            auto* iterativeInfo = static_cast<IterativeSearchData*>(&info);
            if (iterativeInfo->shouldSendInfo(true)) {  // Phase 6: Check with score change flag
                // UCI Score Conversion FIX: Use root side-to-move stored in SearchData
                if (iterativeInfo->isSingularTelemetryEnabled()) {
                    iterativeInfo->flushSingularTelemetry(false);
                }
                sendCurrentSearchInfo(*iterativeInfo, info.rootSideToMove, tt);
                iterativeInfo->recordInfoSent(iterativeInfo->bestScore);  // Phase 6: Pass current score
            }
        }
    }
    
    // Phase P2: Store PV status in search stack
    searchInfo.setPvNode(ply, isPvNode);
    // Stage 6c bridge: keep legacy stack exclusion in sync until singular extensions migrate fully.
    const Move legacyExcluded = limits.enableExcludedMoveParam ? context.excludedMove() : NO_MOVE;
    Move previousExcluded = searchInfo.getExcludedMove(ply);
#ifdef DEBUG
    if (legacyExcluded != NO_MOVE) {
        assert(limits.enableExcludedMoveParam && "Excluded move requires EnableExcludedMoveParam toggle");
        assert(limits.useSingularExtensions && "Excluded move requires UseSingularExtensions toggle");
    }
    assert(previousExcluded == NO_MOVE && "Unexpected stale excluded move at this ply");
#endif
    searchInfo.setExcludedMove(ply, legacyExcluded);
    const bool restoreExcluded = previousExcluded != legacyExcluded;
    ExcludedMoveGuard excludedGuard(searchInfo, ply, previousExcluded, restoreExcluded);
#ifdef DEBUG
    if (legacyExcluded != NO_MOVE) {
        assert(ply > 0 && "Root node should never mark an excluded move");
    }
#endif

    // Stage 0: Keep extension totals in sync for current node (parent sets applied count)
    searchInfo.setExtensionApplied(ply, searchInfo.extensionApplied(ply));
    
    // Update selective depth (maximum depth reached)
    if (ply > info.seldepth) {
        info.seldepth = ply;
    }

    // Phase 4.1: Default history context to none before ordering decisions
    info.clearHistoryContext(ply);

    // Terminal node - enter quiescence search or return static evaluation
    if (depth <= 0) {
        // Stage 14: Quiescence search - ALWAYS compiled in, controlled by UCI option
        // This is a CORE FEATURE and should never be behind compile flags!
        if (info.useQuiescence) {
            // Candidate 9: Detect time pressure for panic mode
            // Panic mode activates when remaining time < 100ms
            bool inPanicMode = false;
            if (info.timeLimit != std::chrono::milliseconds::max()) {
                auto remainingTime = info.timeLimit - info.elapsed();
                inPanicMode = (remainingTime < std::chrono::milliseconds(100));
            }
            // Node explosion diagnostics: Track quiescence entry
            if (limits.nodeExplosionDiagnostics) {
                g_nodeExplosionStats.recordQuiescenceEntry(ply);
            }
            // Use quiescence search to resolve tactical sequences
            if (limits.useSearchNodeAPIRefactor) {
                NodeContext qContext = context;
                qContext.clearExcluded();
                return quiescence(board, qContext, ply, 0, alpha, beta, searchInfo, info, limits, *tt, 0, inPanicMode);
            }
            return quiescence(board, ply, 0, alpha, beta, searchInfo, info, limits, *tt, 0, inPanicMode);
        }
        // Fallback: return static evaluation (only if quiescence disabled via UCI)
        return board.evaluate();
    }
    
    // Sub-phase 4B: Draw Detection Order
    // CRITICAL ORDER:
    // 1. Check for checkmate/stalemate FIRST (terminal conditions)
    // 2. Check for repetition SECOND
    // 3. Check for fifty-move rule THIRD
    // 4. Only THEN probe TT
    
    // Initialize TT move and info for singular extensions
    Move ttMove = NO_MOVE;
    Move singularCandidate = NO_MOVE;
    bool singularVerificationRan = false;
    bool singularExtensionPending = false;
    int singularExtensionAmountPending = 0;
    eval::Score singularVerificationScore = eval::Score::zero();
    eval::Score singularVerificationBeta = eval::Score::zero();
    eval::Score ttScore = eval::Score::zero();
    Bound ttBound = Bound::NONE;
    int ttDepth = -1;
    
    // Phase 4.2.c: Compute static eval early and preserve it for TT storage
    // This will be the true static evaluation, not the search score
    eval::Score staticEval = eval::Score::zero();
    bool staticEvalComputed = false;
    
    // Check if we're in check (needed for null move and other decisions)
    bool weAreInCheck = inCheck(board);
    
    // Check extension: Extend search by 1 ply when in check
    // This is a fundamental search extension present in all competitive engines
    // It helps the engine see through forcing sequences and avoid horizon effects
    if (weAreInCheck) {
        depth++;
    }
    
    // Phase 3.2: We'll generate pseudo-legal moves later, after TT/null pruning
    // Obtain per-ply scratch MoveList to avoid deep recursion stack growth
    MoveList& moves = info.acquireMoveList(ply);

    // Sub-phase 4B: Establish correct probe order
    // 1. Check repetition FIRST (fastest, most common draw in search)
    // 2. Check fifty-move rule SECOND (less common)
    // 3. Only THEN probe TT (after all draw conditions)
    
    // Never check draws below was guarding ply>0.
    // For robustness, do a minimal ROOT draw check too, before probing TT, to avoid
    // returning a TT score in a terminal draw while not having a best move.
    if (ply > 0) {
        // PERFORMANCE OPTIMIZATION: Strategic draw checking
        bool shouldCheckDraw = false;
        
        // Get info about last move from search stack (if available)
        bool lastMoveWasCapture = false;
        if (ply > 0 && ply - 1 < 128) {
            Move lastMove = searchInfo.getStackEntry(ply - 1).move;
            if (lastMove != NO_MOVE) {
                lastMoveWasCapture = isCapture(lastMove);
            }
        }
        
        shouldCheckDraw = 
            weAreInCheck ||                         // Always after checks (use cached value)
            lastMoveWasCapture ||                   // After captures (50-move reset)
            (ply >= 4 && (ply & 3) == 0);          // Every 4th ply for repetitions
        
        // Check for draws when necessary
        if (shouldCheckDraw && board.isDrawInSearch(searchInfo, ply)) {
            return eval::Score::draw();  // Draw score
        }
    }

    // Minimal root draw handling (Option B support): if root position is a draw,
    // return draw score but also ensure a legal move is available for UCI output.
    if (ply == 0) {
        if (board.isDrawInSearch(searchInfo, ply)) {
            MoveList legalRoot;
            MoveGenerator::generateLegalMoves(board, legalRoot);
            if (!legalRoot.empty()) {
                info.bestMove = legalRoot[0];
                info.bestScore = eval::Score::draw();
            }
            return eval::Score::draw();
        }
    }

    // Probe TT even at root (after draw detection if any)
    // IMPORTANT: At root (ply==0) we DO NOT ALLOW hard returns from TT.
    // We only use TT info to tighten bounds and seed ordering, then we still
    // generate moves and search at least one move to establish bestMove/PV.
    {
        TTEntry* ttEntry = nullptr;
        if (tt && tt->isEnabled()) {
            Hash zobristKey = board.zobristKey();
            tt->prefetch(zobristKey);
            ttEntry = tt->probe(zobristKey);
            info.ttProbes++;

            if (ttEntry && ttEntry->key32 == (zobristKey >> 32)) {
                info.ttHits++;

                if (ttEntry->depth >= depth) {
                    eval::Score tts(ttEntry->score);
                    ttBound = ttEntry->bound();

                    if (tts.value() >= MATE_BOUND)      tts = eval::Score(tts.value() - ply);
                    else if (tts.value() <= -MATE_BOUND) tts = eval::Score(tts.value() + ply);

                    if (ttBound == Bound::EXACT) {
                        if (ply > 0) {
                            info.ttCutoffs++;
                            return tts;  // non-root: safe to cutoff
                        } else {
                            // root: tighten alpha around exact score but do not return
                            if (tts > alpha) alpha = tts;
                            if (tts < beta)  beta = tts;
                            // keep window valid at root
                            if (alpha >= beta) alpha = eval::Score(beta.value() - 1);
                        }
                    } else if (ttBound == Bound::LOWER) {
                        if (ply > 0 && tts >= beta) {
                            info.ttCutoffs++;
                            return tts;  // non-root: beta cutoff
                        }
                        if (tts > alpha) alpha = tts;
                    } else if (ttBound == Bound::UPPER) {
                        if (ply > 0 && tts <= alpha) {
                            info.ttCutoffs++;
                            return tts;  // non-root: alpha cutoff
                        }
                        if (tts < beta) beta = tts;
                        if (alpha >= beta) {
                            if (ply > 0) {
                                info.ttCutoffs++;
                                return alpha;  // non-root: window closed
                            } else {
                                // root: keep window valid
                                alpha = eval::Score(beta.value() - 1);
                            }
                        }
                    }
                }

                // Extract TT move for ordering
                ttMove = static_cast<Move>(ttEntry->move);
                if (ttMove != NO_MOVE) {
                    Square from = moveFrom(ttMove);
                    Square to = moveTo(ttMove);
                    if (from < 64 && to < 64 && from != to) {
                        info.ttMoveHits++;
                        ttScore = eval::Score(ttEntry->score);
                        ttBound = ttEntry->bound();
                        ttDepth = ttEntry->depth;
                    } else {
                        ttMove = NO_MOVE;
                        info.ttCollisions++;
                        std::cerr << "WARNING: Corrupted TT move detected: "
                                  << std::hex << ttEntry->move << std::dec << std::endl;
                    }
                }

                if (!staticEvalComputed && ttEntry->evalScore != TT_EVAL_NONE) {
                    staticEval = eval::Score(ttEntry->evalScore);
                    staticEvalComputed = true;
                }

                if (limits.useSingularExtensions && limits.enableExcludedMoveParam &&
                    depth >= SINGULAR_DEPTH_MIN && ply > 0 && ttMove != NO_MOVE &&
                    ttBound == Bound::EXACT && ttDepth >= depth - 1) {
                    auto& singularStats = info.singularStats;
                    singularStats.candidatesExamined++;

                    const bool legalCandidate = MoveGenerator::isLegal(board, ttMove);
                    if (!legalCandidate) {
                        singularStats.candidatesRejectedIllegal++;
                    } else if (isCapture(ttMove) || isPromotion(ttMove)) {
                        singularStats.candidatesRejectedTactical++;
                    } else {
                        singularStats.candidatesQualified++;
                        singularCandidate = ttMove;
                    }
                }
            }
        }
    }

    const bool singularExclusionPrimed = singularCandidate != NO_MOVE;
    if (singularExclusionPrimed) {
#ifdef DEBUG
        assert(isPvNode && "Singular exclusion candidate should only appear on PV nodes");
#endif
        context.setExcluded(singularCandidate);
        if (limits.enableExcludedMoveParam) {
            searchInfo.setExcludedMove(ply, singularCandidate);
        }

        // Stage SE2.2a: Launch verification search with reduced window.
        info.singularStats.verificationsStarted++;
        const eval::Score margin = singular_margin(depth);
        const int singularBetaRaw = ttScore.value() - margin.value();
        singularVerificationBeta = clamp_singular_score(eval::Score(singularBetaRaw));

        singularVerificationScore = verify_exclusion(
            board,
            context,
            depth,
            ply,
            ttScore,
            alpha,
            beta,
            searchInfo,
            info,
            limits,
            tt,
            nullptr,
            nullptr);
        singularVerificationRan = true;
    }

    // Phase 4.2.c: Compute static eval if not in check and haven't gotten it from TT
    // We need this for pruning decisions and to store in TT later
    if (!staticEvalComputed && !weAreInCheck) {
        staticEval = board.evaluate();
        staticEvalComputed = true;
        // Cache it in search stack for potential reuse
        searchInfo.setStaticEval(ply, staticEval);
    }
    
    // Stage 21 Phase A4: Null Move Pruning with Static Null Move and Tuning
    // Constants for null move pruning
    constexpr eval::Score ZUGZWANG_THRESHOLD = eval::Score(500 + 330);  // Rook + Bishop value (original)
    
    // Phase 1.2: Enhanced reverse futility pruning (static null move)
    // Extended to depth 8 with more aggressive shallow margins
    if (!isPvNode && depth <= 8 && depth > 0 && !weAreInCheck && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
        // Phase 4.2.c: Use our pre-computed static eval
        if (staticEvalComputed) {
            // More aggressive margin at shallow depths, conservative at deeper
            // depth 1: 85, 2: 170, 3: 255, 4: 320, 5: 375, 6: 420, 7: 455, 8: 480
            int baseMargin = 85;
            eval::Score margin;
            if (depth <= 3) {
                margin = eval::Score(baseMargin * depth);
            } else {
                // Slower growth for deeper depths
                margin = eval::Score(baseMargin * 3 + (depth - 3) * (baseMargin / 2));
            }
            
            if (staticEval - margin >= beta) {
                info.nullMoveStats.staticCutoffs++;
                
                // TT remediation Phase 2.2: Add TT store for static-null pruning
                // TT pollution fix: Only store at depth >= 2 to reduce low-value writes
                if (tt && tt->isEnabled() && depth >= 2) {
                    uint64_t zobristKey = board.zobristKey();
                    // FIX: Use beta for fail-high, consistent with null-move
                    int16_t scoreToStore = beta.value();
                    
                    // Adjust mate scores before storing
                    if (beta.value() >= MATE_BOUND - MAX_PLY) {
                        scoreToStore = beta.value() + ply;
                    } else if (beta.value() <= -MATE_BOUND + MAX_PLY) {
                        scoreToStore = beta.value() - ply;
                    }
                    
                    // Store with NO_MOVE since this is a static pruning decision
                    // FIX: Use LOWER bound for fail-high (score >= beta)
                    // FIX: Use depth 0 since static-null is heuristic, not searched
                    int16_t evalToStore = staticEvalComputed ? staticEval.value() : TT_EVAL_NONE;
                    tt->store(zobristKey, NO_MOVE, scoreToStore, evalToStore,
                             0, Bound::LOWER);  // Depth 0 for heuristic bound
                    info.ttStores++;
                    // No longer track as missing store
                } else if (!tt || !tt->isEnabled()) {
                    // Only track as missing if TT is disabled
                    info.nullMoveStats.staticNullNoStore++;
                } else {
                    // Skipped due to depth gating (depth < 2)
                    info.nullMoveStats.staticNullNoStore++;
                }
                
                // Node explosion diagnostics: Track reverse futility pruning
                if (limits.nodeExplosionDiagnostics) {
                    g_nodeExplosionStats.recordStaticNullMovePrune(depth);
                }
                return staticEval - margin;  // Return reduced score for safety
            }
        }
    }
    
    // Regular null move pruning
    // Check if we can do null move
    bool canDoNull = !isPvNode                                  // Phase P2: No null in PV nodes!
                    && !weAreInCheck                              // Not in check
                    && depth >= limits.nullMoveMinDepth          // Minimum depth (UCI configurable)
                    && ply > 0                                   // Not at root
                    && !searchInfo.wasNullMove(ply - 1)         // No consecutive nulls
                    && board.nonPawnMaterial(board.sideToMove()) > ZUGZWANG_THRESHOLD  // Original detection
                    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY;  // Not near mate
    
    if (canDoNull && limits.useNullMove) {
        info.nullMoveStats.attempts++;
        
        // Use UCI-configurable reduction values
        int nullMoveReduction = limits.nullMoveReductionBase;
        if (depth >= 6) nullMoveReduction = limits.nullMoveReductionDepth6;
        if (depth >= 12) nullMoveReduction = limits.nullMoveReductionDepth12;
        
        // Dynamic adjustment based on eval margin (UCI configurable)
        if (staticEvalComputed && staticEval - beta > eval::Score(limits.nullMoveEvalMargin)) {
            nullMoveReduction++;
        }
        
        // Phase 4.2: Optional extra reduction when eval margin is very large
        bool aggressiveReductionActive = false;
        if (limits.useAggressiveNullMove && staticEvalComputed && depth >= 10) {
            const eval::Score evalGap = staticEval - beta;
            const int aggressiveThreshold = limits.nullMoveEvalMargin + AGGRESSIVE_NULL_MARGIN_OFFSET;
            if (evalGap.value() > aggressiveThreshold) {
                info.nullMoveStats.aggressiveCandidates++;

                bool allowAggressive = true;
                if (limits.aggressiveNullMinEval > 0 && staticEval.value() < limits.aggressiveNullMinEval) {
                    allowAggressive = false;
                }
                if (allowAggressive && limits.aggressiveNullRequirePositiveBeta && beta.value() <= 0) {
                    allowAggressive = false;
                }
                // Avoid aggressive reductions when we would immediately pay the
                // verification cost anyway (depth close to verify threshold).
                if (allowAggressive && depth >= limits.nullMoveVerifyDepth) {
                    allowAggressive = false;
                }
                if (allowAggressive && limits.aggressiveNullMaxApplications > 0 &&
                    info.nullMoveStats.aggressiveApplied >= static_cast<uint64_t>(limits.aggressiveNullMaxApplications)) {
                    allowAggressive = false;
                    info.nullMoveStats.aggressiveCapHits++;
                }

                bool ttBlocksExtra = false;
                if (allowAggressive && ttDepth >= depth - 1) {
                    if ((ttBound == Bound::UPPER || ttBound == Bound::EXACT) && ttScore < beta) {
                        ttBlocksExtra = true;
                    }
                }

                if (!allowAggressive) {
                    info.nullMoveStats.aggressiveSuppressed++;
                } else if (ttBlocksExtra) {
                    info.nullMoveStats.aggressiveBlockedByTT++;
                } else {
                    int proposedReduction = std::min(nullMoveReduction + 1, depth - 1);
                    if (proposedReduction > nullMoveReduction) {
                        nullMoveReduction = proposedReduction;
                        aggressiveReductionActive = true;
                        info.nullMoveStats.aggressiveApplied++;
                    } else {
                        info.nullMoveStats.aggressiveSuppressed++;
                    }
                }
            }
        }

        // Ensure we don't reduce too much
        nullMoveReduction = std::min(nullMoveReduction, depth - 1);
        
        // Make null move
        Board::UndoInfo nullUndo;
        board.makeNullMove(nullUndo);
        searchInfo.setNullMove(ply, true);
        
        // Search with adaptive reduction (null window around beta)
        const NodeContext nullContext = makeChildContext(context, false);
#ifdef DEBUG
        assert(!nullContext.isRoot() && "Null move context must not be marked as root");
        assert(!nullContext.isPv() && "Null move context must never be PV");
        assert(!nullContext.hasExcludedMove() && "Null move context should not carry excluded move");
#endif
        eval::Score nullScore = -negamax(
            board,
            nullContext,
            depth - 1 - nullMoveReduction,
            ply + 1,
            -beta,
            -beta + eval::Score(1),
            searchInfo,
            info,
            limits,
            tt,
            nullptr  // Phase PV1: Pass nullptr for now
        );
        
        // Unmake null move
        board.unmakeNullMove(nullUndo);
        searchInfo.setNullMove(ply, false);
        
        // Check for cutoff
        if (nullScore >= beta) {
            info.nullMoveStats.cutoffs++;
            if (aggressiveReductionActive) {
                info.nullMoveStats.aggressiveCutoffs++;
            }
            
            // Phase 1.5b: Deeper verification search at configurable depth
            if (depth >= limits.nullMoveVerifyDepth) {  // UCI configurable threshold
                // Verification search at depth - R (one ply deeper than null move)
                NodeContext verifyContext = context;
                verifyContext.setPv(false);
                verifyContext.clearExcluded();
#ifdef DEBUG
                assert(verifyContext.isRoot() == context.isRoot() &&
                       "Null-move verification should preserve root flag");
#endif
#ifdef DEBUG
                assert(!verifyContext.hasExcludedMove() && "Null-move verify should clear excluded move");
#endif
                eval::Score verifyScore = negamax(
                    board,
                    verifyContext,
                    depth - nullMoveReduction,  // depth - R (matches Stockfish/Laser)
                    ply,
                    beta - eval::Score(1),
                    beta,
                    searchInfo,
                    info,
                    limits,
                    tt,
                    nullptr  // Phase PV1: Pass nullptr for now
                );
                
                if (verifyScore < beta) {
                    // Verification failed, don't trust null move
                    info.nullMoveStats.verificationFails++;
                    if (aggressiveReductionActive) {
                        info.nullMoveStats.aggressiveVerifyFails++;
                    }
                    // Continue with normal search instead of returning
                } else {
                    // Verification passed, null move cutoff is valid
                    // TT remediation Phase 2.1: Add TT store for null-move cutoff
                    if (tt && tt->isEnabled()) {
                        uint64_t zobristKey = board.zobristKey();
                        int16_t scoreToStore = beta.value();  // Store beta for fail-high
                        
                        // Adjust mate scores before storing
                        if (beta.value() >= MATE_BOUND - MAX_PLY) {
                            scoreToStore = beta.value() + ply;
                        } else if (beta.value() <= -MATE_BOUND + MAX_PLY) {
                            scoreToStore = beta.value() - ply;
                        }
                        
                        // Store with NO_MOVE since this is a null-move cutoff
                        // Use LOWER bound since we're failing high (score >= beta)
                        tt->store(zobristKey, NO_MOVE, scoreToStore, TT_EVAL_NONE,
                                 static_cast<uint8_t>(depth), Bound::LOWER);
                        info.ttStores++;
                        // No longer track as missing store
                    } else {
                        // Only track as missing if TT is disabled
                        info.nullMoveStats.nullMoveNoStore++;
                    }
                    
                    if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                        if (aggressiveReductionActive) {
                            info.nullMoveStats.aggressiveVerifyPasses++;
                        }
                        return nullScore;
                    } else {
                        // Mate score, return beta instead
                        if (aggressiveReductionActive) {
                            info.nullMoveStats.aggressiveVerifyPasses++;
                        }
                        return beta;
                    }
                }
            } else {
                // Shallow depth, trust null move without verification
                // TT remediation Phase 2.1: Add TT store for null-move cutoff
                if (tt && tt->isEnabled()) {
                    uint64_t zobristKey = board.zobristKey();
                    int16_t scoreToStore = beta.value();  // Store beta for fail-high
                    
                    // Adjust mate scores before storing
                    if (beta.value() >= MATE_BOUND - MAX_PLY) {
                        scoreToStore = beta.value() + ply;
                    } else if (beta.value() <= -MATE_BOUND + MAX_PLY) {
                        scoreToStore = beta.value() - ply;
                    }
                    
                    // Store with NO_MOVE since this is a null-move cutoff
                    // Use LOWER bound since we're failing high (score >= beta)
                    tt->store(zobristKey, NO_MOVE, scoreToStore, TT_EVAL_NONE,
                             static_cast<uint8_t>(depth), Bound::LOWER);
                    info.ttStores++;
                    // No longer track as missing store
                } else {
                    // Only track as missing if TT is disabled
                    info.nullMoveStats.nullMoveNoStore++;
                }
                
                if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                    return nullScore;
                } else {
                    // Mate score, return beta instead
                    return beta;
                }
            }
        }
    } else if (!canDoNull && ply > 0 && board.nonPawnMaterial(board.sideToMove()) <= ZUGZWANG_THRESHOLD) {
        // Track when we avoid null move due to zugzwang
        info.nullMoveStats.zugzwangAvoids++;
    }
    
    // Phase R2.1: Razoring with Enhanced Safety Guards
    // Apply razoring at shallow depths (1-2) when static eval is far below alpha
    // Phase R2.1: Complete skip when tactical, endgame guard, TT context guard
    if (limits.useRazoring && 
        !isPvNode && 
        !weAreInCheck && 
        depth <= 2 && 
        depth >= 1 &&
        std::abs(alpha.value()) < MATE_BOUND - MAX_PLY) {
        
        // Increment attempt counter
        info.razoring.attempts++;
        
        // Phase R2.1: TT context guard - skip if TT suggests we won't fail low
        if (ttBound == Bound::LOWER && ttScore > alpha - eval::Score(50)) {
            info.razoring.ttContextSkips++;
            // Don't razor - TT suggests this position won't fail low
        }
        // Phase R2.1: Endgame guard - 1300cp NPM threshold (matches quiescence)
        else if (board.nonPawnMaterial(board.sideToMove()).value() < 1300 || 
                 board.nonPawnMaterial(~board.sideToMove()).value() < 1300) {
            info.razoring.endgameSkips++;
            // Don't razor in endgames - zugzwang risk
        }
        else {
            // Phase R2.1: Check for tactical moves (SEE >= 0 captures)
            bool hasTacticalMoves = false;
            MoveList captures;
            MoveGenerator::generateCaptures(board, captures);
            
            for (size_t i = 0; i < captures.size(); ++i) {
                Move move = captures[i];
                if (isCapture(move) && seeGE(board, move, 0)) {
                    hasTacticalMoves = true;
                    info.razoring.tacticalSkips++;  // Track when tactical guard triggers
                    break;
                }
            }
            
            // Phase R2.1: Option A - Skip razoring entirely if tactical moves exist
            if (hasTacticalMoves) {
                // Skip razoring at this node - too dangerous
            }
            else {
                // Select margin based on depth
                int razorMargin = (depth == 1) ? limits.razorMargin1 : limits.razorMargin2;
                
                // Check if static eval + margin is still below alpha
                if (staticEval + eval::Score(razorMargin) <= alpha) {
                    // Run quiescence search with scout window
                    eval::Score qScore;
                    if (limits.useSearchNodeAPIRefactor) {
                        NodeContext qContext = context;
                        qContext.clearExcluded();
                        qScore = quiescence(
                            board,
                            qContext,
                            ply,
                            0,                    // qsearch depth starts at 0
                            alpha,
                            alpha + eval::Score(1), // alpha+1 (scout window)
                            searchInfo,
                            info,
                            limits,
                            *tt,
                            0,
                            false);
                    } else {
                        qScore = quiescence(
                            board,
                            ply,
                            0,
                            alpha,
                            alpha + eval::Score(1),
                            searchInfo,
                            info,
                            limits,
                            *tt,
                            0,
                            false);
                    }
                    
                    // If quiescence still fails low, we can return
                    if (qScore <= alpha) {
                        // Update telemetry
                        info.razoring.cutoffs++;
                        info.razoring.depthBuckets[depth - 1]++;
                        info.razoringCutoffs++;  // Legacy counter
                        
                        // Phase R2: Improved TT storage with mate score adjustment
                        if (tt && tt->isEnabled()) {
                            uint64_t zobristKey = board.zobristKey();
                            int16_t scoreToStore = qScore.value();
                            
                            // Adjust mate scores before storing
                            if (qScore.value() >= MATE_BOUND - MAX_PLY) {
                                scoreToStore = qScore.value() + ply;  // Adjust mate score
                            } else if (qScore.value() <= -MATE_BOUND + MAX_PLY) {
                                scoreToStore = qScore.value() - ply;  // Adjust mated score
                            }
                            
                            tt->store(zobristKey, NO_MOVE, scoreToStore, TT_EVAL_NONE, 
                                     static_cast<uint8_t>(depth), Bound::UPPER);
                            info.ttStores++;
                        }
                        
                        return qScore;
                    }  // if (qScore <= alpha)
                }  // if (staticEval + razorMargin <= alpha)
            }  // else (not tactical)
        }  // else (not endgame, not TT skip)
    }  // if razoring enabled
    
    // Phase 4.1: Razoring - DISABLED (OLD VERSION)
    // Testing showed -39.45 Elo loss, likely due to:
    // 1. Interaction with our existing pruning techniques
    // 2. SeaJay's evaluation may not be accurate enough for razoring
    // 3. Our quiescence search implementation may differ from Laser's
    // Razoring requires very careful tuning and may not be suitable for SeaJay's current architecture
    
    // In quiescence search (depth <= 0), handle differently
    if (depth <= 0) {
        // Get info about last move
        bool lastMoveWasCapture = false;
        if (ply > 0 && ply - 1 < 128) {
            Move lastMove = searchInfo.getStackEntry(ply - 1).move;
            if (lastMove != NO_MOVE) {
                lastMoveWasCapture = isCapture(lastMove);
            }
        }
        
        // Only check insufficient material after captures in qsearch
        if (lastMoveWasCapture && board.isInsufficientMaterial()) {
            return eval::Score::draw();
        }
    }
    
    // Get previous move for countermove lookup (CM3.2)
    Move prevMove = NO_MOVE;
    if (ply > 0) {
        prevMove = searchInfo.getStackEntry(ply - 1).move;
    }
    
    // Phase 2a: Use RankedMovePicker if enabled (skip at root for safety)
    // Use optional to avoid dynamic allocation (stack allocation instead)
    std::optional<RankedMovePicker> rankedPicker;
    if (limits.useRankedMovePicker && ply > 0) {
        // Predict whether history/countermove history ordering will be active
        auto historyCtx = SearchData::HistoryContext::None;
        const bool hasHistoryInfra = info.killers && info.history && info.counterMoves;
        if (hasHistoryInfra) {
            if (info.counterMoveHistory && depth >= HISTORY_GATING_DEPTH) {
                historyCtx = SearchData::HistoryContext::Counter;
            } else {
                historyCtx = SearchData::HistoryContext::Basic;
            }
        }
        if (historyCtx != SearchData::HistoryContext::None) {
            info.registerHistoryApplication(ply, historyCtx);
        }
        rankedPicker.emplace(
            board, ttMove, info.killers, info.history, 
            info.counterMoves, info.counterMoveHistory,
            prevMove, ply, depth, info.countermoveBonus, &limits,
            &info
        );
    } else {
        // Generate pseudo-legal moves now that early exits are handled
        MoveGenerator::generateMovesForSearch(board, moves, false);
        
        // CM4.1: Track countermove availability (removed from here to avoid double-counting)
        // Hit tracking now happens only when countermove causes a cutoff or is tried first
        
        // Order moves for better alpha-beta pruning
        // TT move first, then promotions (especially queen), then captures (MVV-LVA), then killers, then quiet moves
        // CM3.3: Pass prevMove and bonus for countermove ordering
        // Phase 4.3.a-fix2: Pass depth for CMH gating
        orderMoves(board, moves, ttMove, &info, ply, prevMove, info.countermoveBonus, &limits, depth);
    }
    
    // Node explosion diagnostics: Track TT move ordering effectiveness
    if (limits.nodeExplosionDiagnostics && ttMove != NO_MOVE) {
        // Check if TT move is in the move list
        bool ttMoveValid = std::find(moves.begin(), moves.end(), ttMove) != moves.end();
        // Check if it's first
        bool ttMoveFirst = (!moves.empty() && moves[0] == ttMove);
        g_nodeExplosionStats.recordTTMoveFound(ttMoveValid, ttMoveFirst);
    }
    
    // Debug output at root for deeper searches
    if (ply == 0 && depth >= 4) {
        if (!limits.suppressDebugOutput) {
            std::cerr << "Root: generated " << moves.size() << " moves, depth=" << depth << "\n";
        }
    }
    
    
    // Debug: Validate board state before search
#ifdef DEBUG
    Hash hashBefore = board.zobristKey();
    int pieceCountBefore = __builtin_popcountll(board.occupied());
#endif
    
    // Sub-phase 5A-5E: Store preparation
    // Store original alpha for bound determination
    eval::Score alphaOrig = alpha;
    
    // Initialize best score
    eval::Score bestScore = eval::Score::minus_infinity();
    Move bestMove = NO_MOVE;  // Track best move for all plies (needed for TT storage later)
    
    // Search all moves
    int moveCount = 0;
    int legalMoveCount = 0;  // Phase 3.2: Track legal moves for checkmate/stalemate
    
    // Stage 20 Phase B3 Fix: Track quiet moves for butterfly history update
    std::vector<Move> quietMoves;
    quietMoves.reserve(moves.size());
    
    // Phase 2a: Iterate using RankedMovePicker or traditional move list
    Move move;
    size_t moveIndex = 0;
    if (singularExclusionPrimed) {
        context.clearExcluded();
        if (limits.enableExcludedMoveParam) {
            searchInfo.setExcludedMove(ply, previousExcluded);
        }
    }

    while (true) {
        bool pendingMoveCountPrune = false;
        int pendingMoveCountLimit = 0;
        int pendingMoveCountDepthBucket = 0;
#ifdef SEARCH_STATS
        int pendingMoveCountRankBucket = -1;
#endif
        int pendingMoveCountValue = moveCount;  // Current legal moves searched before this move
        int pendingRankValue = -1;
        if (rankedPicker) {
            move = rankedPicker->next();
            if (move == NO_MOVE) break;
        } else {
            if (moveIndex >= moves.size()) break;
            move = moves[moveIndex++];
        }

        std::string debugMoveUci;
        bool debugTrackedMove = false;
        auto logTrackedEvent = [&](const std::string& event, const std::string& extra) {
            if (!debugTrackedMove) {
                return;
            }
            std::ostringstream oss;
            oss << "info string DebugMove " << debugMoveUci
                << " depth=" << depth
                << " ply=" << ply
                << " event=" << event;
            if (!extra.empty()) {
                oss << ' ' << extra;
            }
            const int extHere = searchInfo.extensionApplied(ply);
            const int extTotal = searchInfo.totalExtensions(ply);
            oss << " extHere=" << extHere
                << " extTotal=" << extTotal;
            std::cout << oss.str() << std::endl;
        };
        if (!limits.debugTrackedMoves.empty()) {
            debugMoveUci = SafeMoveExecutor::moveToString(move);
            for (const auto& candidate : limits.debugTrackedMoves) {
                if (debugMoveUci == candidate) {
                    debugTrackedMove = true;
                    break;
                }
            }
            if (debugTrackedMove) {
                std::ostringstream extra;
                extra << "moveIndex=" << moveCount << " legal=" << legalMoveCount;
                logTrackedEvent("consider", extra.str());
            }
        }
        // Skip excluded move when threaded via NodeContext (singular extension scaffold)
        if (limits.enableExcludedMoveParam) {
            const Move excluded = context.excludedMove();
            if (excluded != NO_MOVE && move == excluded) {
                continue;
            }
        }

        if (singularVerificationRan && limits.useSingularExtensions) {
            if (singularVerificationScore < singularVerificationBeta) {
                info.singularStats.verificationFailLow++;
                const int extensionDepthConfig = std::max(limits.singularExtensionDepth, 0);
                singularExtensionPending = extensionDepthConfig > 0;
                singularExtensionAmountPending = extensionDepthConfig;
            } else {
                info.singularStats.verificationFailHigh++;
                singularExtensionPending = false;
                singularExtensionAmountPending = 0;
            }
            singularVerificationRan = false;
        }

        // Phase B1: Don't increment moveCount here - wait until we know move is legal
        info.totalMoves++;  // Track total moves examined
        
        // Phase 2: Track current root move for UCI currmove output
        if (ply == 0) {
            info.currentRootMove = move;
            info.currentRootMoveNumber = moveCount;
        }
        
        // Track quiet moves for history update
        if (!isCapture(move) && !isPromotion(move)) {
            quietMoves.push_back(move);
        }
        
        // NOTE: Futility pruning moved to AFTER legality check
        // See implementation after tryMakeMove() succeeds
        
        // Phase 3.1 CONSERVATIVE: Move Count Pruning (Late Move Pruning)
        // Only prune at depths 3+ to avoid tactical blindness at shallow depths
        // Much more conservative limits to avoid over-pruning
        bool parentGaveCheck = (ply > 0) ? searchInfo.moveGaveCheck(ply - 1) : false;

        if (limits.useMoveCountPruning && !isPvNode && !weAreInCheck && !parentGaveCheck
            && depth >= 3 && depth <= limits.moveCountMaxDepth && moveCount > 1
            && !isCapture(move) && !isPromotion(move) && !info.killers->isKiller(ply, move)) {
            
            // Phase 3.3: Countermove Consideration
            // Don't prune countermoves - they're often good responses
            if (prevMove != NO_MOVE && move == info.counterMoves->getCounterMove(prevMove)) {
                // This is a countermove, don't prune it
                // Skip the entire move count pruning block
            } else {
            
            // MODERATELY CONSERVATIVE depth-based move count limits
            // Starting at depth 3 to avoid shallow tactical issues
            // 25% less conservative than previous version for more pruning
            // Now configurable via UCI for SPSA tuning
            int moveCountLimit[9] = {
                999, // depth 0 (not used)
                999, // depth 1 (not used - too shallow)
                999, // depth 2 (not used - too shallow)  
                limits.moveCountLimit3,  // depth 3 - moderately conservative
                limits.moveCountLimit4,  // depth 4 - moderately conservative
                limits.moveCountLimit5,  // depth 5 - moderately conservative
                limits.moveCountLimit6,  // depth 6 - moderately conservative
                limits.moveCountLimit7,  // depth 7 - moderately conservative
                limits.moveCountLimit8   // depth 8 - moderately conservative
            };
            
            // Check if we're improving (compare to previous ply's eval)
            bool improving = false;
            if (ply >= 2) {
                int prevEval = searchInfo.getStackEntry(ply - 2).staticEval;
                int currEval = searchInfo.getStackEntry(ply).staticEval;
                if (prevEval != 0 && currEval != 0) {
                    improving = (currEval > prevEval);
                }
            }
            
            // Adjust limit based on improvement
            int limit = moveCountLimit[std::min(depth, 8)];
            if (!improving) {
                limit = (limit * limits.moveCountImprovingRatio) / 100;  // Configurable reduction ratio
            }
            
            // History-based adjustment (moderately conservative)
            int historyScore = info.history->getScore(board.sideToMove(), moveFrom(move), moveTo(move));
            if (historyScore > limits.moveCountHistoryThreshold) {  // Configurable threshold
                limit += limits.moveCountHistoryBonus;  // Configurable bonus
            }
            
            // Phase 2b.3: LMP rank gating - adjust limit based on move rank
            if (limits.useRankAwareGates && !isPvNode && ply > 0 && !weAreInCheck && depth >= 4 && depth <= 8) {
#ifdef DEBUG
                assert(!context.hasExcludedMove() &&
                       "Rank-aware LMP gating should not run when an excluded move is active");
#endif
                // Get rank from picker if available, otherwise use moveCount as fallback
                // Note: Using moveCount before legality check is an approximation
                const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : (moveCount + 1);
                const int K = 5;  // Protected window size
                
                // Phase 2b.7: Check if PVS re-search smoothing should disable rank-based LMP
                bool skipRankLMP = false;
                if (depth >= 4) {  // Smoothing only applies at depth >= 4
                    // Check if smoothing applies for this move
                    bool isTTMove = (move == ttMove);
                    bool isKillerMove = info.killers->isKiller(ply, move);
                    bool isCounterMove = (prevMove != NO_MOVE && 
                                          info.counterMoves->getCounterMove(prevMove) == move);
                    bool isRecapture = (prevMove != NO_MOVE && isCapture(prevMove) && 
                                        moveTo(prevMove) == moveTo(move));
                    bool isCheckOrPromo = isPromotion(move);
                    
                    if (!isTTMove && !isKillerMove && !isCounterMove && !isRecapture && !isCheckOrPromo) {
                        int depthBucket = SearchData::PVSReSearchSmoothing::depthBucket(depth);
                        int rankBucket = SearchData::PVSReSearchSmoothing::rankBucket(rank);
                        if (info.pvsReSearchSmoothing.shouldApplySmoothing(depthBucket, rankBucket)) {
                            // Smoothing: disable rank-based early pruning for this move
                            skipRankLMP = true;
                        }
                    }
                }
                
                if (!skipRankLMP) {
                    if (rank == 1) {
                        // Rank 1: disable LMP for this move (make limit very high)
                        limit = 999;
                    } else if (rank >= 2 && rank <= K) {
                        // Ranks 2-5: make prune less aggressive
                        limit += 2;
                    } else if (rank >= 6 && rank <= 10) {
                        // Ranks 6-10: leave limit unchanged
                        // (no adjustment needed)
                    } else if (rank >= 11) {
                        // Ranks 11+: make prune more aggressive
                        limit = std::max(1, limit - 4);
                    }
                }
                // If skipRankLMP is true, use baseline LMP for depth (limit stays unchanged)
            }
            
            if (moveCount > limit) {
                pendingMoveCountPrune = true;
                pendingMoveCountLimit = limit;
                pendingMoveCountDepthBucket = SearchData::PruneBreakdown::bucketForDepth(depth);
#ifdef SEARCH_STATS
                if (limits.useRankAwareGates && ply > 0) {
                    const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : (moveCount + 1);
                    pendingMoveCountRankBucket = SearchData::RankGateStats::bucketForRank(rank);
                    pendingRankValue = rank;
                }
#endif
            }
            } // End of else block for countermove check
        }
        
        // Singular Extension: DISABLED - Implementation needs redesign
        // Extension calculation moved earlier for effective-depth futility
        // int extension = 0;  // Now calculated before futility pruning
        
        // TODO: Reimplement singular extensions properly:
        // 1. Loop through all moves except TT move
        // 2. Search each at reduced depth with narrow window
        // 3. If all fail low, extend the TT move
        // See Laser engine for reference implementation
        
        #if 0  // DISABLED - needs proper implementation
        if (depth >= 7 && move == ttMove && ttBound == Bound::LOWER && 
            ttDepth >= depth - 3 && ply > 0 && !isPvNode &&
            std::abs(ttScore.value()) < MATE_BOUND - MAX_PLY) {
            // Implementation removed - was not working correctly
        }
        #endif
        
        // Track QxP and RxP attempts before making the move
        if (isCapture(move) && !isPromotion(move)) {
            Square fromSq = moveFrom(move);
            Square toSq = moveTo(move);
            Piece attacker = board.pieceAt(fromSq);
            Piece victim = board.pieceAt(toSq);
            PieceType attackerType = typeOf(attacker);
            PieceType victimType = typeOf(victim);
            
            if (victimType == PAWN) {
                if (attackerType == QUEEN) {
                    info.moveOrderingStats.qxpAttempts++;
                } else if (attackerType == ROOK) {
                    info.moveOrderingStats.rxpAttempts++;
                }
            }
        }
        
        // Track game phase
        int pieceCount = __builtin_popcountll(board.occupied());
        if (pieceCount > 28) {
            info.moveOrderingStats.openingNodes++;
        } else if (pieceCount >= 16) {
            info.moveOrderingStats.middlegameNodes++;
        } else {
            info.moveOrderingStats.endgameNodes++;
        }
        
        // Phase 2b.5: Capture SEE gating by rank
        if (limits.useRankAwareGates && !isPvNode && ply > 0 && !weAreInCheck && depth >= 4
            && isCapture(move) && !isPromotion(move)) {
#ifdef DEBUG
            assert(!context.hasExcludedMove() &&
                   "Rank-aware capture gating should not run when an excluded move is active");
#endif
            
            // Check exemptions: TT move, killers, countermoves, recaptures
            bool isTTMove = (move == ttMove);
            bool isKillerMove = info.killers->isKiller(ply, move);
            bool isCounterMove = (prevMove != NO_MOVE && 
                                  info.counterMoves->getCounterMove(prevMove) == move);
            bool isRecapture = (prevMove != NO_MOVE && isCapture(prevMove) && 
                                moveTo(prevMove) == moveTo(move));
            
            if (!isTTMove && !isKillerMove && !isCounterMove && !isRecapture) {
                // Get rank from picker if available, otherwise use moveCount+1 (pre-legality)
                const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : (moveCount + 1);
                const int K = 5;  // Protected window size
                
                // Only gate late-ranked captures (rank >= 11)
                if (rank >= 11) {
                    // Require non-losing SEE for late captures
                    if (!seeGE(board, move, 0)) {
                        // Track telemetry
#ifdef SEARCH_STATS
                        const int bucket = SearchData::RankGateStats::bucketForRank(rank);
                        info.rankGates.pruned[bucket]++;
#endif
                        if (debugTrackedMove) {
                            std::ostringstream extra;
                            extra << "rank=" << rank << " reason=see";
                            logTrackedEvent("prune_capture_rank", extra.str());
                        }
                        continue;  // Skip this capture
                    }
                }
                // Ranks 1-10: no SEE gate (conservative)
            }
        }
        
        // Phase 3.2: Try to make the move with lazy legality checking
        Board::UndoInfo undo;
        if (!board.tryMakeMove(move, undo)) {
            // B0: Count illegal pseudo-legal moves
            if (legalMoveCount == 0) {
                info.illegalPseudoBeforeFirst++;
            }
            info.illegalPseudoTotal++;
            if (debugTrackedMove) {
                logTrackedEvent("illegal", "reason=king_in_check");
            }
            // Move is illegal (leaves king in check) - skip it
            continue;
        }

        bool givesCheckMove = inCheck(board);
        bool wasCaptureMove = (undo.capturedPiece != NO_PIECE);

        if (pendingMoveCountPrune && !givesCheckMove && !wasCaptureMove) {
#ifdef DEBUG
            assert(!context.hasExcludedMove() &&
                   "Move count pruning should bypass nodes with excluded move context");
#endif
            info.moveCountPruned++;
            info.pruneBreakdown.moveCount[pendingMoveCountDepthBucket]++;

#ifdef SEARCH_STATS
            if (pendingMoveCountRankBucket >= 0) {
                info.rankGates.pruned[pendingMoveCountRankBucket]++;
            }
#endif
            if (limits.nodeExplosionDiagnostics) {
                g_nodeExplosionStats.recordMoveCountPrune(depth, pendingMoveCountValue);
            }
            if (debugTrackedMove) {
                std::ostringstream extra;
                extra << "limit=" << pendingMoveCountLimit << " moveCount=" << pendingMoveCountValue
                      << " rank=" << pendingRankValue
                      << " capture=" << (wasCaptureMove ? 1 : 0);
                logTrackedEvent("prune_move_count", extra.str());
            }
            board.unmakeMove(move, undo);
            continue;
        }

        // Move is legal and not pruned by move-count pruning
        legalMoveCount++;
        moveCount++;  // Phase B1: Increment moveCount only after confirming legality
        
        // Phase 2b.1: Rank capture and telemetry (no behavior change)
#ifdef SEARCH_STATS
        if (limits.useRankAwareGates && ply > 0) {  // Skip at root, same as RankedMovePicker
            // Get current move rank (1-based index from picker, or moveCount as fallback)
            const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
            const int bucket = SearchData::RankGateStats::bucketForRank(rank);
            info.rankGates.tried[bucket]++;
        }
#endif

        // Push position to search stack after we know move is legal
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);

        searchInfo.setGaveCheck(ply, givesCheckMove);

        enum class ExtensionType : uint8_t {
            None = 0,
            Singular = 1,
            SingularStack = 2,
            Recapture = 3,
            Check = 4,
        };

        auto extensionTypeToString = [](ExtensionType type) constexpr -> const char* {
            switch (type) {
                case ExtensionType::Singular: return "singular";
                case ExtensionType::SingularStack: return "singular+recapture";
                case ExtensionType::Recapture: return "recapture";
                case ExtensionType::Check: return "check";
                default: return "none";
            }
        };

        int extension = 0;
        ExtensionType extensionType = ExtensionType::None;
        int singularExtensionAmount = 0;
        int recaptureExtensionAmount = 0;
        bool stackedSingularExtension = false;

        // Singular extension: only for TT move confirmed singular by verification helper.
        const bool isSingularMove = (limits.useSingularExtensions && singularExtensionPending &&
                                     move == singularCandidate);
        if (isSingularMove) {
            singularExtensionAmount = singularExtensionAmountPending;
            singularExtensionPending = false;
            singularExtensionAmountPending = 0;
            if (singularExtensionAmount > 0) {
                extension = singularExtensionAmount;
                extensionType = ExtensionType::Singular;
            }
        }

        const bool isRecaptureMove = (prevMove != NO_MOVE && isCapture(prevMove) &&
                                      isCapture(move) && moveTo(prevMove) == moveTo(move));

        if (limits.allowStackedExtensions && singularExtensionAmount > 0 && isRecaptureMove) {
            auto& singularStats = info.singularStats;
            singularStats.stackingCandidates++;

            const bool depthGuardPassed = depth >= STACKED_RECAPTURE_MIN_DEPTH;
            const bool evalGuardPassed = staticEvalComputed &&
                                         (staticEval.value() >= beta.value() - STACKED_RECAPTURE_EVAL_MARGIN_CP);
            const bool ttGuardPassed = ttDepth >= depth + STACKED_RECAPTURE_TT_MARGIN;

            if (!depthGuardPassed) {
                singularStats.stackingRejectedDepth++;
            }
            if (!evalGuardPassed) {
                singularStats.stackingRejectedEval++;
            }
            if (!ttGuardPassed) {
                singularStats.stackingRejectedTT++;
            }

            if (depthGuardPassed && evalGuardPassed && ttGuardPassed) {
                recaptureExtensionAmount = 1;
                extension += recaptureExtensionAmount;
                extensionType = ExtensionType::SingularStack;
                stackedSingularExtension = true;
                singularStats.stackingApplied++;
            }
        }

        // Phase 1: Effective-Depth Futility Pruning (AFTER legality, BEFORE child search)
        // Following notes: Apply after confirming legality but before recursion
        const auto& config = seajay::getConfig();
        bool canPruneFutility = (depth <= 6) ? (legalMoveCount > 1) : (legalMoveCount >= 1);
        
        // Check if this is a special move that shouldn't be pruned
        bool isKillerMove = info.killers->isKiller(ply, move);
        bool isCounterMove = (prevMove != NO_MOVE &&
                              info.counterMoves->getCounterMove(prevMove) == move);
        // Determine whether the CHILD would be a PV node: at a PV parent, only the first legal move is PV
        bool childIsPV = (isPvNode && legalMoveCount == 1);
        NodeContext childContext = makeChildContext(context, childIsPV);
#ifdef DEBUG
        childContext.clearExcluded();
#endif
#ifdef DEBUG
        assert(!childContext.isRoot() && "Child context should never retain root flag");
        assert(childContext.isPv() == childIsPV && "Child context PV flag mismatch");
        if (!limits.enableExcludedMoveParam) {
            assert(!childContext.hasExcludedMove() && "Child context unexpectedly carries excluded move");
        }
#endif
        
        if (config.useFutilityPruning && !childIsPV && depth > 0 && !weAreInCheck
            && canPruneFutility && !isCapture(move) && !isPromotion(move)
            && staticEvalComputed && move != ttMove 
            && !isKillerMove && !isCounterMove) {
#ifdef DEBUG
            assert(!context.hasExcludedMove() &&
                   "Futility pruning should bypass nodes with excluded move context");
#endif
            
            // Calculate what reduction this move would get from LMR
            int reduction = 0;
            if (ply > 0 && info.lmrParams.enabled && depth >= info.lmrParams.minDepth && legalMoveCount > 1) {
                bool captureMove = false;  // Already checked it's not a capture
                bool givesCheck = false;  // Actual check handling below via clamp
                bool pvNode = isPvNode;

                bool allowReduction = limits.useSearchNodeAPIRefactor
                    ? shouldReduceMove(move, depth, moveCount, captureMove,
                                       weAreInCheck, givesCheck, context,
                                       *info.killers, *info.history,
                                       *info.counterMoves, prevMove,
                                       ply, board.sideToMove(),
                                       info.lmrParams)
                    : shouldReduceMove(move, depth, moveCount, captureMove,
                                       weAreInCheck, givesCheck, pvNode,
                                       *info.killers, *info.history,
                                       *info.counterMoves, prevMove,
                                       ply, board.sideToMove(),
                                       info.lmrParams);

                if (allowReduction) {
                    bool improving = false;
                    reduction = limits.useSearchNodeAPIRefactor
                        ? getLMRReduction(depth, moveCount, info.lmrParams, context, improving)
                        : getLMRReduction(depth, moveCount, info.lmrParams, pvNode, improving);

                    if (givesCheckMove && reduction > 0) {
                        const int rankForClamp = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                        const int clampLimit = (depth <= 6 && rankForClamp <= 10) ? 0 : 1;
                        reduction = std::min(reduction, clampLimit);
                    }
                }
            }
            
            // Calculate effective depth: eff = depth - 1 - reduction + extension
            int effectiveDepth = depth - 1 - reduction + extension;
            
            // Apply futility at effective depth
            if (effectiveDepth <= config.futilityMaxDepth) {
                // Handle leaf-like positions (eff <= 0)
                if (effectiveDepth <= 0) {
                    // Very shallow effective depth - use minimal margin
                    if (staticEval <= alpha - eval::Score(100)) {
                        board.unmakeMove(move, undo);
                        info.futilityPruned++;
                        continue;
                    }
                } else {
                    // Normal futility with progressive margins
                    int futilityMargin;
                    if (effectiveDepth <= 4) {
                        futilityMargin = config.futilityBase * effectiveDepth;
                    } else {
                        // Cap growth after 4 plies as requested
                        futilityMargin = config.futilityBase * 4 + (effectiveDepth - 4) * (config.futilityBase / 2);
                    }
                    
                    // Phase 2b.4: Futility margin scaling by rank
                    if (limits.useRankAwareGates && !isPvNode && ply > 0 && !weAreInCheck && depth >= 3) {
                        // Get rank from picker if available, otherwise use legalMoveCount
                        // Note: Using legalMoveCount after legality check is accurate
                        const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : legalMoveCount;
                        const int K = 5;  // Protected window size
                        
                        if (rank >= 1 && rank <= K) {
                            // Ranks 1-5: no change to margin (protect top moves)
                            // (no adjustment needed)
                        } else if (rank >= 6 && rank <= 10) {
                            // Ranks 6-10: modest bump to margin
                            futilityMargin += config.futilityBase / 2;
                        } else if (rank >= 11) {
                            // Ranks 11+: bigger (but still modest) bump
                            futilityMargin += config.futilityBase;
                        }
                        
                        // Optional cap to prevent excessive margins
                        futilityMargin = std::min(futilityMargin, config.futilityBase * (effectiveDepth + 1));
                    }
                    
                    if (staticEval <= alpha - eval::Score(futilityMargin)) {
                        board.unmakeMove(move, undo);
                        info.futilityPruned++;
                        // Track by effective depth for telemetry
                        int b = SearchData::PruneBreakdown::bucketForDepth(effectiveDepth);
                        info.pruneBreakdown.futilityEff[b]++;
                        
                        // Phase 2b.4: Track rank-aware futility pruning telemetry
#ifdef SEARCH_STATS
                        if (limits.useRankAwareGates && ply > 0) {
                            const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : legalMoveCount;
                            const int bucket = SearchData::RankGateStats::bucketForRank(rank);
                            info.rankGates.pruned[bucket]++;
                        }
#endif
                        
                        // Node explosion diagnostics: Track futility pruning
                        if (limits.nodeExplosionDiagnostics) {
                            g_nodeExplosionStats.recordFutilityPrune(effectiveDepth, futilityMargin);
                        }
                        continue;
                    }
                }
            }
        }
        
        // Phase PV3: Acquire child PV storage from arena when needed
        TriangularPV* childPVPtr = nullptr;
        if (pv != nullptr && isPvNode) {
            childPVPtr = info.acquireChildPV(ply);
        }
        
        // Phase P3: Principal Variation Search (PVS) with LMR integration
        eval::Score score;

        // Track beta cutoff position for move ordering analysis
        bool isCutoffMove = false;

        if (extension > 0) {
            const bool singularExtensionActive = (singularExtensionAmount > 0);
            const int clamped = searchInfo.clampExtensionAmount(ply, extension, singularExtensionActive);
            if (clamped != extension) {
                if (stackedSingularExtension && clamped < extension) {
                    info.singularStats.stackingBudgetClamped++;
                }
                extension = clamped;
                if (extension == 0) {
                    extensionType = ExtensionType::None;
                    singularExtensionAmount = 0;
                    recaptureExtensionAmount = 0;
                    stackedSingularExtension = false;
                } else {
                    if (singularExtensionAmount > extension) {
                        singularExtensionAmount = extension;
                    }
                    if (singularExtensionAmount < extension) {
                        recaptureExtensionAmount = extension - singularExtensionAmount;
                    } else {
                        recaptureExtensionAmount = 0;
                        if (extensionType == ExtensionType::SingularStack) {
                            extensionType = ExtensionType::Singular;
                        }
                        stackedSingularExtension = false;
                    }
                }
            }
        }

        // Record extension metadata for the child node before searching it
        searchInfo.setExtensionApplied(ply + 1, extension);
        searchInfo.setSingularExtensionApplied(ply + 1, singularExtensionAmount);
        if (singularExtensionAmount > 0) {
            info.singularStats.extensionsApplied += static_cast<uint64_t>(singularExtensionAmount);
            info.singularExtensions += static_cast<uint64_t>(singularExtensionAmount);
        }
        if (recaptureExtensionAmount > 0) {
            info.singularStats.stackingExtraDepth += static_cast<uint64_t>(recaptureExtensionAmount);
        }
        const int extensionDepth = searchInfo.totalExtensions(ply + 1);
        if (extensionDepth > 0) {
            const uint32_t depthValue = static_cast<uint32_t>(extensionDepth);
            if (depthValue > info.singularStats.maxExtensionDepth) {
                info.singularStats.maxExtensionDepth = depthValue;
            }
        }
        if (isPvNode && move == singularCandidate && singularExtensionAmount > 0 && !childContext.isPv()) {
            childContext.setPv(true);
        }

        if (debugTrackedMove && extension != 0) {
            std::ostringstream extra;
            extra << "value=" << extension
                  << " total=" << searchInfo.totalExtensions(ply + 1)
                  << " type=" << extensionTypeToString(extensionType);
            if (recaptureExtensionAmount > 0) {
                extra << " recapture=" << recaptureExtensionAmount;
            }
            logTrackedEvent("extension_apply", extra.str());
        }

        // Phase 2b.7: Variables for PVS re-search smoothing (declared outside if/else)
        int moveRank = 0;
        bool didReSearch = false;
        int appliedReduction = 0;
        
        // Phase B1: Use legalMoveCount to determine if this is the first LEGAL move
        if (legalMoveCount == 1) {
            // First move: search with full window as PV node (apply extension if any)
            // Pass childPV only if we're in a PV node and have a parent PV
            TriangularPV* firstMoveChildPV = (pv != nullptr && isPvNode) ? childPVPtr : nullptr;
            score = -negamax(board, childContext, depth - 1 + extension, ply + 1,
                            -beta, -alpha, searchInfo, info, limits, tt,
                            firstMoveChildPV);  // Phase PV3: Pass child PV for PV nodes
        } else {
            // Later moves: use PVS with LMR
            
            // Phase 3: Late Move Reductions (LMR)
            int reduction = 0;
            
            // Phase 2b.7: PVS re-search smoothing - compute early for both LMR and LMP
            bool applySmoothing = false;
            // moveRank already declared outside the if/else
            if (limits.useRankAwareGates && !isPvNode && depth >= 4) {
                // Get current move rank (1-based from picker, or moveCount as fallback)
                moveRank = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                
                // Check if smoothing should apply (only for non-exempt moves)
                bool isTTMove = (move == ttMove);
                bool isKillerMove = info.killers->isKiller(ply, move);
                bool isCounterMove = (prevMove != NO_MOVE && 
                                      info.counterMoves->getCounterMove(prevMove) == move);
                bool isRecapture = (prevMove != NO_MOVE && isCapture(prevMove) && 
                                    moveTo(prevMove) == moveTo(move));
                bool isCheckOrPromo = isPromotion(move);  // We already know !weAreInCheck
                
                if (!isTTMove && !isKillerMove && !isCounterMove && !isRecapture && !isCheckOrPromo) {
                    int depthBucket = SearchData::PVSReSearchSmoothing::depthBucket(depth);
                    int rankBucket = SearchData::PVSReSearchSmoothing::rankBucket(moveRank);
                    applySmoothing = info.pvsReSearchSmoothing.shouldApplySmoothing(depthBucket, rankBucket);
                    
#ifdef SEARCH_STATS
                    if (applySmoothing) {
                        info.pvsReSearchSmoothing.recordSmoothingApplied(depth, moveRank);
                    }
#endif
                }
            }
            
            // Calculate LMR reduction (don't reduce at root)
            if (ply > 0 && info.lmrParams.enabled && depth >= info.lmrParams.minDepth) {
                // Determine move properties for LMR
                bool captureMove = isCapture(move);
                bool givesCheck = false;  // Actual check handling below via clamp
                bool pvNode = isPvNode;   // Phase P3: Use actual PV status
                
                // Check if we should reduce this move with improved conditions
                bool allowReduction = limits.useSearchNodeAPIRefactor
                    ? shouldReduceMove(move, depth, moveCount, captureMove,
                                       weAreInCheck, givesCheck, context,
                                       *info.killers, *info.history,
                                       *info.counterMoves, prevMove,
                                       ply, board.sideToMove(),
                                       info.lmrParams)
                    : shouldReduceMove(move, depth, moveCount, captureMove,
                                       weAreInCheck, givesCheck, pvNode,
                                       *info.killers, *info.history,
                                       *info.counterMoves, prevMove,
                                       ply, board.sideToMove(),
                                       info.lmrParams);

                if (allowReduction) {
                    // Calculate reduction amount
                    // For now, assume not improving to be conservative
                    // (future enhancement: track eval history for proper improving detection)
                    bool improving = false; // Conservative: assume not improving

                    reduction = limits.useSearchNodeAPIRefactor
                        ? getLMRReduction(depth, moveCount, info.lmrParams, context, improving)
                        : getLMRReduction(depth, moveCount, info.lmrParams, pvNode, improving);

                    if (givesCheckMove && reduction > 0) {
                        const int rankForClamp = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                        const int clampLimit = (depth <= 6 && rankForClamp <= 10) ? 0 : 1;
                        const int originalReduction = reduction;
                        reduction = std::min(reduction, clampLimit);
                        if (debugTrackedMove && reduction != originalReduction) {
                            std::ostringstream extra;
                            extra << "rank=" << rankForClamp
                                  << " depth=" << depth
                                  << " clamp=" << clampLimit
                                  << " original=" << originalReduction;
                            logTrackedEvent("lmr_check_clamp", extra.str());
                        }
                    }

                    appliedReduction = reduction;
                    
                    // Phase 2b.2: LMR scaling by rank (conservative, non-PV, depth≥4)
                    // SPRT fix: Add !weAreInCheck guard to avoid reducing evasions
                    if (limits.useRankAwareGates && !isPvNode && !weAreInCheck && depth >= 4 
                        && !isCapture(move) && !isPromotion(move)
                        && move != ttMove
                        && !info.killers->isKiller(ply, move)
                        && !(prevMove != NO_MOVE && isCapture(prevMove) && moveTo(prevMove) == moveTo(move))  // Not a recapture
                        && !(prevMove != NO_MOVE && info.counterMoves && info.counterMoves->getCounterMove(prevMove) == move))  // Not a countermove
                    {
                        // Get current move rank (1-based index from picker, or moveCount as fallback)
                        // Phase 2b.2-fix: currentYieldIndex() now always available for accurate rank
                        const int rank = rankedPicker ? rankedPicker->currentYieldIndex() : moveCount;
                        const int K = 5;  // Protected rank threshold
                        
                        // Apply rank-based scaling (CONSERVATIVE after SPRT fail)
                        int originalReduction = reduction;
                        if (rank == 1) {
                            // Rank 1: clamp reduction to 0 (no reduction for best move)
                            reduction = 0;
                        } else if (rank <= K) {
                            // Ranks 2-K: clamp reduction to at most 1
                            reduction = std::min(reduction, 1);
                        }
                        // SPRT fix: Remove the +1 tier entirely for now (too aggressive at shallow depths)
                        // Later phases can re-enable with stricter depth/history guards
                        // else if (rank <= 10) {
                        //     // Ranks 6-10: leave base reduction unchanged
                        // } else {
                        //     // Ranks 11+: DISABLED - was causing over-reduction
                        //     // Only re-enable with depth >= 8 and low history checks
                        // }
                        
#ifdef SEARCH_STATS
                        // Track telemetry if we modified the reduction
                        if (reduction != originalReduction) {
                            const int bucket = SearchData::RankGateStats::bucketForRank(rank);
                            info.rankGates.reduced[bucket]++;
                        }
#endif
                        if (debugTrackedMove && reduction != originalReduction) {
                            std::ostringstream extra;
                            extra << "rank=" << rank
                                  << " original=" << originalReduction
                                  << " adjusted=" << reduction;
                            logTrackedEvent("lmr_scaled", extra.str());
                        }
                        appliedReduction = reduction;
                    }

                    // Phase 2b.7: Apply PVS re-search smoothing to LMR
                    if (applySmoothing && reduction > 0) {
                        // Subtract 1 from any extra reduction added by rank bucket
                        // Do not go below baseline reduction (i.e., the non-rank-aware reduction)
                        int baseReduction = limits.useSearchNodeAPIRefactor
                            ? getLMRReduction(depth, moveCount, info.lmrParams, context, false)
                            : getLMRReduction(depth, moveCount, info.lmrParams, isPvNode, false);
                        reduction = std::max(baseReduction - 1, reduction - 1);
                        appliedReduction = reduction;
                        if (debugTrackedMove) {
                            std::ostringstream extra;
                            extra << "smoothed=" << reduction;
                            logTrackedEvent("lmr_smoothed", extra.str());
                        }
                    }
                    
                    // Track LMR statistics
                    info.lmrStats.totalReductions++;
                    // Node explosion diagnostics: Track LMR application
                    if (limits.nodeExplosionDiagnostics) {
                        g_nodeExplosionStats.recordLMRReduction(depth, reduction);
                    }
                }
            }
            
            // Scout search (possibly reduced, with extension)
            info.pvsStats.scoutSearches++;
            score = -negamax(board, childContext, depth - 1 - reduction + extension, ply + 1,
                            -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                            nullptr);  // Phase PV3: Scout searches don't need PV
            
            // Phase 2b.7: Track re-search for smoothing (only for non-PV nodes)
            // didReSearch already declared outside the if/else
            
        // If reduced scout fails high, re-search without reduction
        if (score > alpha && reduction > 0) {
            info.lmrStats.reSearches++;
            // Node explosion diagnostics: Track LMR re-search
            if (limits.nodeExplosionDiagnostics) {
                g_nodeExplosionStats.recordLMRReSearch(depth);
            }
            score = -negamax(board, childContext, depth - 1 + extension, ply + 1,
                            -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                            nullptr);  // Phase PV3: Still a scout search
        }
            
            // If scout search fails high, do full window re-search
            if (score > alpha) {
                info.pvsStats.reSearches++;
                auto histCtx = info.historyContextAt(ply);
                if (histCtx == SearchData::HistoryContext::Basic) {
                    info.historyStats.basicReSearches++;
                } else if (histCtx == SearchData::HistoryContext::Counter) {
                    info.historyStats.counterReSearches++;
                }
                didReSearch = true;  // Phase 2b.7: Mark that we did a re-search
                // B1 Fix: Only the first legal move should be a PV node
                // Re-searches after scout failures are NOT PV nodes
                TriangularPV* reSearchChildPV = (pv != nullptr && isPvNode) ? childPVPtr : nullptr;
                score = -negamax(board, childContext, depth - 1 + extension, ply + 1,
                                -beta, -alpha, searchInfo, info, limits, tt,
                                reSearchChildPV);  // B1 Fix: Re-search is NOT a PV node!
            } else if (reduction > 0 && score <= alpha) {
                // Reduction was successful (move was bad as expected)
                info.lmrStats.successfulReductions++;
            }
            appliedReduction = reduction;
            if (debugTrackedMove && reduction > 0) {
                std::ostringstream extra;
                extra << "value=" << reduction
                      << " reSearch=" << (didReSearch ? 1 : 0);
                logTrackedEvent("lmr_applied", extra.str());
            }
        }

        int childStaticEval = 0;
        if (ply + 1 < seajay::MAX_PLY) {
            childStaticEval = searchInfo.getStackEntry(ply + 1).staticEval;
        }

        if (debugTrackedMove) {
            eval::EvalTrace trace;
            eval::Score evalScore = eval::evaluateWithTrace(board, trace);
            eval::Score pawnTotal = trace.passedPawns + trace.isolatedPawns + trace.doubledPawns +
                                   trace.backwardPawns + trace.pawnIslands;
            std::ostringstream extra;
            extra << "eval=" << evalScore.to_cp()
                  << " static=" << childStaticEval
                  << " mat=" << trace.material.to_cp()
                  << " pst=" << trace.pst.to_cp()
                  << " pawns=" << pawnTotal.to_cp()
                  << " king=" << trace.kingSafety.to_cp()
                  << " mob=" << trace.mobility.to_cp();
            logTrackedEvent("eval_trace", extra.str());
        }

        // Unmake the move
        board.unmakeMove(move, undo);

        // Phase 2b.7: Record PVS re-search statistics for smoothing
        // Only record for non-PV nodes that were searched (not pruned)
        if (limits.useRankAwareGates && !isPvNode && depth >= 4 && legalMoveCount > 1) {
            // Only record if we have moveRank computed (from earlier smoothing check)
            if (moveRank > 0) {
                info.pvsReSearchSmoothing.recordMove(depth, moveRank, didReSearch);
            }
        }
        
        // Debug: Validate board state is properly restored
#ifdef DEBUG
        assert(board.zobristKey() == hashBefore);
        assert(__builtin_popcountll(board.occupied()) == pieceCountBefore);
#endif
        
        // Check if search was interrupted
        if (info.stopped) {
            return bestScore;
        }

        if (debugTrackedMove) {
            std::ostringstream extra;
            extra << "score=" << score.to_cp()
                  << " alpha=" << alpha.to_cp()
                  << " beta=" << beta.to_cp()
                  << " reduction=" << appliedReduction
                  << " static=" << childStaticEval
                  << " legalIndex=" << legalMoveCount;
            logTrackedEvent("score", extra.str());
        }

        // Update best score and move
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // Phase PV3: Update PV at all depths
            if (pv != nullptr && isPvNode) {
                // Update PV with best move and child's PV
                // childPV should have been populated by the successful search
                if (childPVPtr != nullptr) {
                    pv->updatePV(ply, move, childPVPtr);
                }
            }
            
            // At root, store the best move in SearchInfo
            if (ply == 0) {
                info.bestMove = move;
                info.bestScore = score;
                
                // Debug output for root moves (removed in release builds)
                #ifndef NDEBUG
                if (depth >= 3) {
                    std::cerr << "Root: Move " << SafeMoveExecutor::moveToString(move) 
                              << " score=" << score.to_cp() << " cp" << std::endl;
                }
                #endif
            }
            
            // Update alpha (best score we can guarantee)
            if (score > alpha) {
                alpha = score;
                
                // Beta cutoff - prune remaining moves
                // This is a fail-high, meaning the opponent won't choose this position
                // Return bestScore for fail-soft alpha-beta
                if (score >= beta) [[unlikely]] {
                    info.betaCutoffs++;  // Track beta cutoffs
                    auto histCtx = info.historyContextAt(ply);
                    if (moveCount == 1) {
                        info.betaCutoffsFirst++;  // Track first-move cutoffs
                        if (histCtx == SearchData::HistoryContext::Basic) {
                            info.historyStats.basicFirstMoveHits++;
                        } else if (histCtx == SearchData::HistoryContext::Counter) {
                            info.historyStats.counterFirstMoveHits++;
                        }
                    }

                    const bool isCaptureMv = isCapture(move) || isPromotion(move) || isEnPassant(move);
                    const bool isTTMove = (move == ttMove && ttMove != NO_MOVE);
                    bool isKillerMove = false;
                    if (!isCaptureMv && info.killers) {
                        for (int slot = 0; slot < 2; ++slot) {
                            if (move == info.killers->getKiller(ply, slot)) {
                                isKillerMove = true;
                                break;
                            }
                        }
                    }
                    bool isCounterMove = false;
                    if (info.counterMoves && prevMove != NO_MOVE) {
                        if (move == info.counterMoves->getCounterMove(prevMove)) {
                            isCounterMove = true;
                        }
                    }
                    if (isCounterMove && info.countermoveBonus > 0) {
                        info.counterMoveStats.hits++;
                        info.counterMoveStats.cutoffs++;
                    }
                    if (histCtx == SearchData::HistoryContext::Counter && isCounterMove) {
                        info.historyStats.counterCutoffs++;
                    } else if (histCtx == SearchData::HistoryContext::Basic &&
                               !isCaptureMv && !isTTMove && !isKillerMove && !isCounterMove) {
                        info.historyStats.basicCutoffs++;
                    }
                    
#ifdef SEARCH_STATS
                    // Phase 2a.6c: Track cutoff move rank and shortlist coverage
                    // Gate by UCI toggle to avoid any overhead when disabled
                    if (rankedPicker && limits.useRankedMovePicker && limits.showMovePickerStats) {
                        int yieldIndex = rankedPicker->currentYieldIndex();
                        
                        // Bucket the yield rank: [1], [2-5], [6-10], [11+]
                        if (yieldIndex == 1) {
                            info.movePickerStats.bestMoveRank[0]++;
                        } else if (yieldIndex >= 2 && yieldIndex <= 5) {
                            info.movePickerStats.bestMoveRank[1]++;
                        } else if (yieldIndex >= 6 && yieldIndex <= 10) {
                            info.movePickerStats.bestMoveRank[2]++;
                        } else {
                            info.movePickerStats.bestMoveRank[3]++;
                        }
                        
                        // Check if the cutoff move was in the shortlist
                        if (rankedPicker->wasInShortlist(move)) {
                            info.movePickerStats.shortlistHits++;
                        }
                    }
#endif
                    if (debugTrackedMove) {
                        std::ostringstream extra;
                        extra << "score=" << score.to_cp() << " beta=" << beta.to_cp();
                        logTrackedEvent("cutoff", extra.str());
                    }
                    
                    // Node explosion diagnostics: Track beta cutoff position and move type
                    if (limits.nodeExplosionDiagnostics) {
                        g_nodeExplosionStats.recordBetaCutoff(ply, legalMoveCount - 1, isTTMove, isKillerMove, isCaptureMv);
                        if (legalMoveCount > 10) {
                            g_nodeExplosionStats.recordLateCutoff(ply, legalMoveCount - 1);
                        }
                    }
                    
                    // Diagnostic: Track cutoff position distribution
                    if (legalMoveCount <= 3) {
                        info.cutoffsByPosition[legalMoveCount - 1]++;
                    } else if (legalMoveCount <= 10) {
                        info.cutoffsByPosition[3]++;  // Moves 4-10
                    } else {
                        info.cutoffsByPosition[4]++;  // Moves 11+
                    }
                    
                    // Track detailed move ordering statistics
                    auto& stats = info.moveOrderingStats;
                    
                    // Track which type of move caused cutoff
                    if (isTTMove) {
                        stats.ttMoveCutoffs++;
                    } else if (isCaptureMv) {
                        if (moveCount == 1) {
                            stats.firstCaptureCutoffs++;
                        }
                        // Check if it's a bad capture (QxP or RxP)
                        Square fromSq = moveFrom(move);
                        Square toSq = moveTo(move);
                        Piece attacker = board.pieceAt(fromSq);
                        Piece victim = board.pieceAt(toSq);
                        PieceType attackerType = typeOf(attacker);
                        PieceType victimType = typeOf(victim);
                        
                        if (victimType == PAWN && (attackerType == QUEEN || attackerType == ROOK)) {
                            stats.badCaptureCutoffs++;
                            if (attackerType == QUEEN) {
                                stats.qxpCutoffs++;
                            } else {
                                stats.rxpCutoffs++;
                            }
                        }
                    } else if (isKillerMove) {
                        stats.killerCutoffs++;
                    } else if (isCounterMove && info.countermoveBonus > 0) {
                        stats.counterMoveCutoffs++;
                    } else {
                        stats.quietCutoffs++;
                    }
                    
                    // Track cutoff by move index
                    int moveIndex = moveCount - 1;
                    if (moveIndex < 10) {
                        stats.cutoffsAtMove[moveIndex]++;
                    } else {
                        stats.cutoffsAfter10++;
                    }
                    
                    // Stage 19, Phase A3: Update killer moves for quiet moves that cause cutoffs
                    // Stage 20, Phase B3: Update history for quiet moves that cause cutoffs
                    // Stage 23, Phase CM2: Update countermoves (shadow mode - no ordering yet)
                    if (!isCapture(move) && !isPromotion(move)) {
                        info.killers->update(ply, move);
                        
                        // Update history with depth-based bonus for cutoff move
                        Color side = board.sideToMove();
                        info.history->update(side, moveFrom(move), moveTo(move), depth);
                        
                        // Butterfly update: penalize quiet moves tried before the cutoff
                        for (const Move& quietMove : quietMoves) {
                            if (quietMove != move) {  // Don't penalize the cutoff move itself
                                info.history->updateFailed(side, moveFrom(quietMove), moveTo(quietMove), depth);
                            }
                        }
                        
                        // Stage 23, Phase CM2: Update countermove table
                        // Shadow mode: update the table but don't use for ordering yet
                        if (ply > 0) {
                            Move prevMove = searchInfo.getStackEntry(ply - 1).move;
                            if (prevMove != NO_MOVE) {
                                info.counterMoves->update(prevMove, move);
                                info.counterMoveStats.updates++;  // Track shadow mode updates
                                
                                // Phase 4.3.a: Update counter-move history (simplified interface)
                                if (info.counterMoveHistory) {
                                    info.counterMoveHistory->update(prevMove, move, depth);
                                    
                                    // Penalize quiet moves that were tried but didn't cause cutoff
                                    for (const Move& quietMove : quietMoves) {
                                        if (quietMove != move) {  // Don't penalize the cutoff move itself
                                            info.counterMoveHistory->updateFailed(prevMove, quietMove, depth);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    break;  // Beta cutoff - no need to search more moves
                }
            }

            if (debugTrackedMove) {
                std::ostringstream extra;
                extra << "bestScore=" << bestScore.to_cp();
                if (ply == 0) {
                    extra << " root=1";
                }
                logTrackedEvent("best_update", extra.str());
            }
        }
    }  // End of move iteration loop
    
    // Phase 3.2: Check for checkmate/stalemate after trying all moves
    if (legalMoveCount == 0) {
        if (weAreInCheck) {
            // Checkmate - return mate score
            return eval::Score(-32000 + ply);
        } else {
            // Stalemate - return draw score
            return eval::Score::draw();
        }
    }
    
    // Sub-phase 5A: Basic Store Implementation
    // Store position in TT after search completes
    // Only store if we have a valid TT and didn't terminate early
    if (tt && tt->isEnabled() && !info.stopped && bestMove != NO_MOVE) {
            Hash zobristKey = board.zobristKey();
            
            // Sub-phase 5B: Determine bound type based on score relative to original window
            Bound bound;
            if (bestScore <= alphaOrig) {
                // Fail-low: All moves were worse than alpha
                bound = Bound::UPPER;
            } else if (bestScore >= beta) {
                // Fail-high: We found a move better than beta (beta cutoff)
                bound = Bound::LOWER;
            } else {
                // Exact: Score is within the original window
                bound = Bound::EXACT;
            }
            
            // Sub-phase 5C: Mate Score Adjustment for storing
            // Adjust mate scores to be relative to root (inverse of adjustMateScoreFromTT)
            eval::Score scoreToStore = bestScore;
            if (bestScore.value() >= MATE_BOUND) {
                // Positive mate score - we're winning
                // Store distance from root, not from current position
                scoreToStore = eval::Score(bestScore.value() + ply);
            } else if (bestScore.value() <= -MATE_BOUND) {
                // Negative mate score - we're losing
                // Store distance from root, not from current position
                scoreToStore = eval::Score(bestScore.value() - ply);
            }
            
            // Store the entry
            // Phase 4.2.c: Store the TRUE static eval, not the search score
            // This allows better eval reuse and improving position detection
            int16_t evalToStore = staticEvalComputed ? staticEval.value() : TT_EVAL_NONE;
            tt->store(zobristKey, bestMove, scoreToStore.value(), evalToStore, 
                     static_cast<uint8_t>(depth), bound);
            info.ttStores++;
    }

    // Root safety net: ensure we always have a move to play.
    // In rare edge cases (e.g., tightened TT window + pruning), bestMove may remain NO_MOVE.
    if (ply == 0 && bestMove == NO_MOVE) {
        MoveList legalRoot;
        MoveGenerator::generateLegalMoves(board, legalRoot);
        if (!legalRoot.empty()) {
            bestMove = legalRoot[0];
            info.bestMove = bestMove;
            // bestScore may be uninformative; keep as-is (fail-soft). UCI will still have a move.
        }
    }

    return bestScore;
}

// Stage 13: Test wrapper for iterative deepening (Deliverable 1.2a)
// This function calls the existing search without modifications
// Used to verify that IterativeSearchData doesn't break anything
Move searchIterativeTest(Board& board, const SearchLimits& limits, TranspositionTable* tt) {
    // Stage 13, Deliverable 1.2c: Full iteration recording (all depths)
    // Stage 13, Deliverable 2.2a: Time management integration prep
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    auto infoPtr = std::make_unique<IterativeSearchData>();
    IterativeSearchData& info = *infoPtr;  // Stored on heap to keep thread stack lean

    // Allocate move ordering helpers on the heap to avoid per-thread stack pressure
    auto killerMoves = std::make_unique<KillerMoves>();
    auto historyHeuristic = std::make_unique<HistoryHeuristic>();
    auto counterMovesTable = std::make_unique<CounterMoves>();

    // Phase 4.3.a: Allocate counter-move history on heap (32MB per thread)
    std::unique_ptr<CounterMoveHistory> counterMoveHistoryPtr = std::make_unique<CounterMoveHistory>();

    // Connect helper storage to search data object
    info.killers = killerMoves.get();
    info.history = historyHeuristic.get();
    info.counterMoves = counterMovesTable.get();
    info.counterMoveHistory = counterMoveHistoryPtr.get();

    // Ensure scratch arenas and statistics start clean for this search
    info.reset();
    info.setSingularTelemetryEnabled(limits.useSingularExtensions);

    // UCI Score Conversion FIX: Store root side-to-move for all UCI output
    // This MUST be used for all UCI output, not the changing board.sideToMove() during search
    info.rootSideToMove = board.sideToMove();
    
    // Stage 14, Deliverable 1.8: Pass quiescence option to search
    info.useQuiescence = limits.useQuiescence;
    
    // Stage 14 Remediation: Parse SEE modes once at search start
    info.seePruningModeEnum = parseSEEPruningMode(limits.seePruningMode);
    info.seePruningModeEnumQ = parseSEEPruningMode(limits.seePruningModeQ);
    
    // Stage 18: Initialize LMR parameters from limits
    info.lmrParams.enabled = limits.lmrEnabled;
    info.lmrParams.minDepth = limits.lmrMinDepth;
    info.lmrParams.minMoveNumber = limits.lmrMinMoveNumber;
    info.lmrParams.baseReduction = limits.lmrBaseReduction;
    info.lmrParams.depthFactor = limits.lmrDepthFactor;
    info.lmrParams.historyThreshold = limits.lmrHistoryThreshold;
    info.lmrParams.pvReduction = limits.lmrPvReduction;
    info.lmrParams.nonImprovingBonus = limits.lmrNonImprovingBonus;
    info.lmrStats.reset();
    
    // Stage 22 Phase P3.5: Reset PVS statistics at search start
    info.pvsStats.reset();
    
    // Stage 23, Phase CM2: Reset countermove statistics at search start
    info.counterMoveStats.reset();
    
    // Stage 23, Phase CM3.3: Initialize countermove bonus from limits
    info.countermoveBonus = limits.countermoveBonus;
    info.counterMoves->clear();  // Clear countermove table for fresh search
    
    // Stage 13 Remediation: Set configurable stability threshold
    // Phase 4: Adjust for game phase if enabled
    if (limits.usePhaseStability) {
        GamePhase phase = detectGamePhase(board);
        int adjustedThreshold = getPhaseStabilityThreshold(phase, 
                                                          limits.stabilityThreshold,
                                                          limits.openingStability,
                                                          limits.middlegameStability,
                                                          limits.endgameStability,
                                                          true);  // Use specific values
        info.setRequiredStability(adjustedThreshold);
        
        // Debug output for phase detection
        #ifndef NDEBUG
        const char* phaseName = (phase == GamePhase::OPENING) ? "Opening" :
                               (phase == GamePhase::MIDDLEGAME) ? "Middlegame" : "Endgame";
        std::cerr << "[Phase Stability] Game phase: " << phaseName 
                  << ", stability threshold: " << adjustedThreshold << std::endl;
        #endif
    } else {
        info.setRequiredStability(limits.stabilityThreshold);
    }
    
    // Stage 13, Deliverable 2.2b: Switch to new time management
    // Calculate initial time limits with neutral stability (1.0)
    TimeLimits timeLimits = calculateTimeLimits(limits, board, 1.0);
    info.m_softLimit = timeLimits.soft.count();
    info.m_hardLimit = timeLimits.hard.count();
    info.m_optimumTime = timeLimits.optimum.count();
    
    
    // Use NEW calculation for timeLimit (replacing old)
    info.timeLimit = timeLimits.optimum;
    // Detect depth-only (fixed-depth) searches: treat as infinite time
    // More robust: infer from limits rather than computed optimum alone
    const bool depthOnlySearch = (
        limits.movetime == std::chrono::milliseconds(0) &&
        limits.time[WHITE] == std::chrono::milliseconds(0) &&
        limits.time[BLACK] == std::chrono::milliseconds(0)
    );
    
    // Debug logging for time management (Deliverable 2.2b)
    if (limits.movetime == std::chrono::milliseconds(0)) {  // Only log for non-fixed time
        std::cerr << "[Time Management] Initial limits: ";
        std::cerr << "soft=" << info.m_softLimit << "ms, ";
        std::cerr << "hard=" << info.m_hardLimit << "ms, ";
        std::cerr << "optimum=" << info.m_optimumTime << "ms\n";
    }
    
    Move bestMove;
    Move previousBestMove = NO_MOVE;  // Track best move from previous iteration
    eval::Score previousScore = eval::Score::zero();  // Track score for aspiration windows
    
    // Advance TT generation for new search to enable proper replacement strategy
    if (tt) {
        tt->newSearch();
    }
    
    // Same iterative deepening loop as original search
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        // Check external stop flag before starting new iteration
        if (limits.stopFlag && limits.stopFlag->load(std::memory_order_relaxed)) {
            info.stopped = true;
            break;
        }
        
        info.depth = depth;
        board.setSearchMode(true);
        
        // Track start time for this iteration
        auto iterationStart = std::chrono::steady_clock::now();
        uint64_t nodesBeforeIteration = info.nodes;  // Save node count before iteration
        
        // Stage 13, Deliverable 3.2b: Use aspiration window for depth >= 4
        eval::Score alpha, beta;
        AspirationWindow window;
        
        // Stage 13 Remediation: Use configurable aspiration windows
        if (limits.useAspirationWindows && 
            depth >= AspirationConstants::MIN_DEPTH && 
            previousScore != eval::Score::zero()) {
            // Calculate aspiration window based on previous score with configurable delta
            window = calculateInitialWindow(previousScore, depth, limits.aspirationWindow);
            alpha = window.alpha;
            beta = window.beta;
        } else {
            // Use infinite window for shallow depths or when disabled
            alpha = eval::Score::minus_infinity();
            beta = eval::Score::infinity();
        }
        
        // Phase PV2: Pass root PV arena to collect principal variation at root
        eval::Score score = negamax(board, depth, 0, alpha, beta, searchInfo, info, limits, tt,
                                   &info.rootPV());  // Phase PV2: Collect PV at root
        
        // Stage 13, Deliverable 3.2d: Progressive widening re-search
        if (depth >= AspirationConstants::MIN_DEPTH && (score <= alpha || score >= beta)) {
            // Progressive widening instead of full window
            bool failedHigh = (score >= beta);
            window.failedLow = !failedHigh;
            window.failedHigh = failedHigh;
            
            // Progressive widening loop
            while (score <= alpha || score >= beta) {
                // Widen the window progressively with configurable growth mode
                WindowGrowthMode growthMode = parseWindowGrowthMode(limits.aspirationGrowth);
                window = widenWindow(window, score, failedHigh, limits.aspirationMaxAttempts, growthMode);
                alpha = window.alpha;
                beta = window.beta;
                
                // Re-search with widened window
                // Phase PV2: Pass root PV arena for re-search at root
                score = negamax(board, depth, 0, alpha, beta, searchInfo, info, limits, tt,
                              &info.rootPV());  // Phase PV2: Collect PV at root re-search
                
                // Check if we're now using an infinite window
                if (window.isInfinite()) {
                    break;  // No point in further widening
                }
                
                // Update fail direction if needed
                failedHigh = (score >= beta);
            }
            // B0: Aggregate aspiration telemetry after re-search loop
            info.aspiration.attempts += static_cast<uint64_t>(window.attempts);
            if (window.failedLow) info.aspiration.failLow++;
            if (window.failedHigh) info.aspiration.failHigh++;
        }
        
        board.setSearchMode(false);
        
        if (!info.stopped) {
            bestMove = info.bestMove;
            
            // Stage 13, Deliverable 5.1a: Use enhanced UCI info output
            // Phase 6: Always send info at iteration completion and record it
            // Phase PV4: Pass root PV arena for full principal variation display
            // UCI Score Conversion FIX: Use root side-to-move, not current position's side
            if (info.isSingularTelemetryEnabled()) {
                info.flushSingularTelemetry(false);
            }
            sendIterationInfo(info, info.rootSideToMove, tt, &info.rootPV());
            info.recordInfoSent(info.bestScore);  // Phase 6: Record to prevent immediate duplicate
            
            // Record iteration data for ALL depths (full recording)
            auto iterationEnd = std::chrono::steady_clock::now();
            auto iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                iterationEnd - iterationStart).count();
            
            // Ensure minimum iteration time of 1ms for very fast searches
            if (iterationTime == 0) {
                iterationTime = 1;
            }
            
            IterationInfo iter;
            iter.depth = depth;
            iter.score = score;
            iter.bestMove = info.bestMove;
            iter.nodes = info.nodes - nodesBeforeIteration;  // Nodes for this iteration only
            iter.elapsed = iterationTime;
            // Record actual window used
            iter.alpha = alpha;
            iter.beta = beta;
            iter.windowAttempts = window.attempts;
            iter.failedHigh = window.failedHigh;
            iter.failedLow = window.failedLow;
            
            // Track move changes and stability
            if (depth == 1) {
                iter.moveChanged = false;  // No previous iteration
                iter.moveStability = 1;    // First occurrence
            } else {
                iter.moveChanged = (info.bestMove != previousBestMove);
                if (iter.moveChanged) {
                    iter.moveStability = 1;  // Reset stability counter
                } else {
                    // Same move as previous iteration - increment stability
                    const IterationInfo& prevIter = info.getLastIteration();
                    iter.moveStability = prevIter.moveStability + 1;
                }
            }
            
            iter.firstMoveFailHigh = false;
            iter.failHighMoveIndex = -1;
            iter.secondBestScore = eval::Score::minus_infinity();
            
            // Calculate branching factor if not first iteration
            if (depth > 1 && info.hasIterations()) {
                const IterationInfo& prevIter = info.getLastIteration();
                if (prevIter.nodes > 0) {
                    iter.branchingFactor = static_cast<double>(iter.nodes) / prevIter.nodes;
                } else {
                    iter.branchingFactor = 0.0;
                }
            } else {
                iter.branchingFactor = 0.0;
            }
            
            info.recordIteration(iter);
            info.updateStability(iter);  // Update stability tracking (Deliverable 2.1e)
            previousBestMove = info.bestMove;  // Update for next iteration
            previousScore = score;  // Save score for next iteration's aspiration window
            
            // Stage 22 Phase P3.5: Output PVS statistics if requested
            if (limits.showPVSStats && info.pvsStats.scoutSearches > 0) {
                std::cout << "info string PVS scout searches: " << info.pvsStats.scoutSearches << std::endl;
                std::cout << "info string PVS re-searches: " << info.pvsStats.reSearches << std::endl;
                std::cout << "info string PVS re-search rate: " 
                          << std::fixed << std::setprecision(1)
                          << info.pvsStats.reSearchRate() << "%" << std::endl;
            }
            
            // Stage 23, Phase CM4.1: Enhanced countermove statistics
            if (info.counterMoveStats.updates > 0 || info.counterMoveStats.hits > 0) {
                double hitRate = info.counterMoveStats.updates > 0 
                    ? (100.0 * info.counterMoveStats.hits / info.counterMoveStats.updates) 
                    : 0.0;
                std::cout << "info string Countermoves: updates=" << info.counterMoveStats.updates 
                          << " hits=" << info.counterMoveStats.hits
                          << " hitRate=" << std::fixed << std::setprecision(1) << hitRate << "%"
                          << " bonus=" << info.countermoveBonus << std::endl;
            }
            
            // Output detailed move ordering statistics at depth 5 and 10
            if ((depth == 5 || depth == 10) && info.nodes > 1000) {
                auto& stats = info.moveOrderingStats;
                uint64_t totalCutoffs = stats.ttMoveCutoffs + stats.firstCaptureCutoffs + 
                                       stats.killerCutoffs + stats.counterMoveCutoffs +
                                       stats.quietCutoffs + stats.badCaptureCutoffs;
                
                // Always output stats for debugging
                std::cout << "info string MoveOrdering: totalCutoffs=" << totalCutoffs 
                          << " TT=" << stats.ttMoveCutoffs 
                          << " Cap=" << stats.firstCaptureCutoffs
                          << " Kill=" << stats.killerCutoffs 
                          << " nodes=" << info.nodes << std::endl;
                
                if (totalCutoffs > 0) {
                    std::cout << "info string MoveOrdering cutoffs: "
                              << std::fixed << std::setprecision(1)
                              << "TT=" << (100.0 * stats.ttMoveCutoffs / totalCutoffs) << "% "
                              << "1stCap=" << (100.0 * stats.firstCaptureCutoffs / totalCutoffs) << "% "
                              << "Killer=" << (100.0 * stats.killerCutoffs / totalCutoffs) << "% "
                              << "Counter=" << (100.0 * stats.counterMoveCutoffs / totalCutoffs) << "% "
                              << "Quiet=" << (100.0 * stats.quietCutoffs / totalCutoffs) << "% "
                              << "BadCap=" << (100.0 * stats.badCaptureCutoffs / totalCutoffs) << "%" << std::endl;
                    
                    // Output cutoff distribution by move index
                    std::cout << "info string Cutoff by index: ";
                    for (int i = 0; i < 5; i++) {
                        std::cout << "[" << i << "]=" << stats.cutoffDistribution(i) << "% ";
                    }
                    std::cout << "[5+]=" << (stats.cutoffDistribution(5) + stats.cutoffDistribution(6) + 
                                             stats.cutoffDistribution(7) + stats.cutoffDistribution(8) +
                                             stats.cutoffDistribution(9)) << "% "
                              << "[10+]=" << stats.cutoffDistribution(10) << "%" << std::endl;
                    
                    // Output QxP and RxP statistics
                    if (stats.qxpAttempts > 0 || stats.rxpAttempts > 0) {
                        std::cout << "info string Bad captures: "
                                  << "QxP=" << stats.qxpAttempts << " attempts (" 
                                  << (stats.qxpAttempts > 0 ? (100.0 * stats.qxpCutoffs / stats.qxpAttempts) : 0.0) << "% cutoff) "
                                  << "RxP=" << stats.rxpAttempts << " attempts ("
                                  << (stats.rxpAttempts > 0 ? (100.0 * stats.rxpCutoffs / stats.rxpAttempts) : 0.0) << "% cutoff)" << std::endl;
                    }
                    
                    // Output game phase distribution
                    uint64_t totalPhaseNodes = stats.openingNodes + stats.middlegameNodes + stats.endgameNodes;
                    if (totalPhaseNodes > 0) {
                        std::cout << "info string Phase distribution: "
                                  << "Opening=" << (100.0 * stats.openingNodes / totalPhaseNodes) << "% "
                                  << "Middle=" << (100.0 * stats.middlegameNodes / totalPhaseNodes) << "% "
                                  << "Endgame=" << (100.0 * stats.endgameNodes / totalPhaseNodes) << "%" << std::endl;
                    }
                    
                    // MO2a: Output killer validation statistics
                    if (stats.killerValidationAttempts > 0) {
                        std::cout << "info string Killer validation: "
                                  << stats.killerValidationAttempts << " attempts, "
                                  << stats.killerValidationFailures << " illegal ("
                                  << (100.0 * stats.killerValidationFailures / stats.killerValidationAttempts) << "% pollution rate)" << std::endl;
                    }
                }
            }
            
            // Stage 13, Deliverable 2.2b: Dynamic time management
            // Recalculate time limits based on current stability
            double stabilityFactor = info.getStabilityFactor();
            if (depth > 2 && stabilityFactor != 1.0) {  // Only adjust after depth 2
                TimeLimits adjustedLimits = calculateTimeLimits(limits, board, stabilityFactor);
                info.m_softLimit = adjustedLimits.soft.count();
                // Don't change hard limit - it's a safety bound
                
                // Log stability-based adjustments
                if (depth == 3 || depth == 5) {  // Log at specific depths to avoid spam
                    std::cerr << "[Time Management] Depth " << depth 
                              << ": stability=" << std::fixed << std::setprecision(1) 
                              << stabilityFactor << ", adjusted soft=" 
                              << info.m_softLimit << "ms\n";
                }
            }
            
            if (score.is_mate_score()) {
                break;
            }
            
            // Stage 13, Deliverable 4.2b: Enhanced early termination logic
            // Skip time management for infinite searches
            if (!limits.infinite && info.m_hardLimit > 0) {
                // Use actual elapsed time, not cached (for accurate time management)
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - info.startTime);
                bool stable = info.isPositionStable();
                
                // Check if we should stop based on new time management
                // Guard against max value issues
                TimeLimits checkLimits;
                checkLimits.soft = std::chrono::milliseconds(info.m_softLimit);
                checkLimits.optimum = std::chrono::milliseconds(info.m_optimumTime);
                // Protect against overflow when hardLimit is near max
                if (info.m_hardLimit >= std::chrono::milliseconds::max().count() - 1000) {
                    checkLimits.hard = std::chrono::milliseconds::max();
                } else {
                    checkLimits.hard = std::chrono::milliseconds(info.m_hardLimit);
                }
                
                if (shouldStopOnTime(checkLimits, elapsed, depth, stable)) {
                    std::cerr << "[Time Management] Stopping at depth " << depth 
                              << " (elapsed=" << elapsed.count() << "ms, "
                              << (stable ? "stable" : "unstable") << ")\n";
                    break;
                }
                
                // For depth-only searches, skip all time-based early termination decisions
                if (!depthOnlySearch) {
                    // Enhanced time prediction using sophisticated EBF
                    double sophisticatedEBF = info.getSophisticatedEBF();
                    if (sophisticatedEBF <= 0) {
                        // Fall back to simple EBF or default
                        sophisticatedEBF = iter.branchingFactor > 0 ? iter.branchingFactor : 5.0;
                    }
                    
                    // Predict time for next iteration
                    auto predictedTime = predictNextIterationTime(
                        std::chrono::milliseconds(iterationTime),
                        sophisticatedEBF,
                        depth + 1
                    );
                    
                    // Early termination decision factors:
                    // 1. Would we exceed soft limit?
                    // 2. Is position stable (less need for deeper search)?
                    // 3. Have we reached reasonable depth?
                    // 4. Is predicted time reasonable?
                    
                    bool exceedsSoftLimit = (elapsed + predictedTime) > std::chrono::milliseconds(info.m_softLimit);
                    bool exceedsHardLimit = (elapsed + predictedTime) > std::chrono::milliseconds(info.m_hardLimit);
                    bool reasonableDepth = depth >= 6;  // Minimum reasonable depth
                    bool veryStable = stable && info.m_stabilityCount >= 3;
                    
                    // Decision logic:
                    if (exceedsHardLimit) {
                        // Never exceed hard limit
                        std::cerr << "[Time Management] No time for depth " << (depth + 1)
                                  << " (would exceed hard limit)\n";
                        break;
                    } else if (exceedsSoftLimit) {
                        // Decide whether to exceed soft limit based on position characteristics
                        if (veryStable || (stable && reasonableDepth)) {
                            // Stop if position is very stable or stable at reasonable depth
                            std::cerr << "[Time Management] No time for depth " << (depth + 1)
                                      << " (would exceed soft limit, position stable/deep)\n";
                            break;
                        } else if (!stable && depth < 6) {
                            // Continue if unstable and not too deep
                            // This allows searching deeper in tactical positions
                            std::cerr << "[Time Management] Continuing despite soft limit "
                                      << "(depth=" << depth << ", unstable)\n";
                        } else {
                            // Default: stop at soft limit
                            std::cerr << "[Time Management] No time for depth " << (depth + 1)
                                      << " (would exceed soft limit)\n";
                            break;
                        }
                    }
                    
                    // Additional early termination for very stable positions
                    if (veryStable && depth >= 8 && predictedTime > std::chrono::milliseconds(2000)) {
                        std::cerr << "[Time Management] Early termination at depth " << depth
                                  << " (very stable, deep enough, next iteration expensive)\n";
                        break;
                    }
                }
            }
        } else {
            break;
        }
    }
    
    // Stage 15: Report final SEE pruning statistics after search completes
    // Only report once at the end, not per iteration
    if (info.seeStats.totalCaptures > 0) {
        std::cout << "info string SEE pruning final: "
                  << info.seeStats.seePruned << "/" << info.seeStats.totalCaptures
                  << " captures pruned (" << std::fixed << std::setprecision(1) 
                  << info.seeStats.pruneRate() << "%)";
        
        // Determine mode from statistics patterns
        if (info.seeStats.conservativePrunes > 0 && info.seeStats.aggressivePrunes == 0) {
            std::cout << " [Conservative mode]";
        } else if (info.seeStats.aggressivePrunes > 0) {
            std::cout << " [Aggressive mode]";
            if (info.seeStats.equalExchangePrunes > 0) {
                std::cout << ", equal exchanges: " << info.seeStats.equalExchangePrunes;
            }
        }
        std::cout << std::endl;
    }
    
    // Phase 2a.6c: Output move picker statistics if requested
#ifdef SEARCH_STATS
    if (limits.showMovePickerStats && limits.useRankedMovePicker) {
        std::cout << "info string MovePickerStats:";
        std::cout << " bestMoveRank: [1]=" << info.movePickerStats.bestMoveRank[0];
        std::cout << " [2-5]=" << info.movePickerStats.bestMoveRank[1];
        std::cout << " [6-10]=" << info.movePickerStats.bestMoveRank[2];
        std::cout << " [11+]=" << info.movePickerStats.bestMoveRank[3];
        std::cout << " shortlistHits=" << info.movePickerStats.shortlistHits;
        std::cout << " SEE(lazy): calls=" << info.movePickerStats.seeCallsLazy;
        std::cout << " captures=" << info.movePickerStats.capturesTotal;
        std::cout << " ttFirstYield=" << info.movePickerStats.ttFirstYield;
        std::cout << " remainderYields=" << info.movePickerStats.remainderYields;
        std::cout << std::endl;
    }
#endif
    
    // B0: One-shot search summary (low overhead)
    if (limits.showSearchStats or limits.showPVSStats) {
        double ttHitRate = info.ttProbes > 0 ? (100.0 * info.ttHits / (double)info.ttProbes) : 0.0;
        double pvsReRate = info.pvsStats.scoutSearches > 0 ? (100.0 * info.pvsStats.reSearches / (double)info.pvsStats.scoutSearches) : 0.0;
        double nullRate = info.nullMoveStats.attempts > 0 ? (100.0 * info.nullMoveStats.cutoffs / (double)info.nullMoveStats.attempts) : 0.0;
        std::cout << "info string SearchStats: depth=" << info.depth
                  << " seldepth=" << info.seldepth
                  << " nodes=" << info.nodes
                  << " nps=" << info.nps()
                  << " tt: probes=" << info.ttProbes
                  << " hits=" << info.ttHits
                  << " hit%=" << std::fixed << std::setprecision(1) << ttHitRate
                  << " cutoffs=" << info.ttCutoffs
                  << " stores=" << info.ttStores
                  << " coll=" << info.ttCollisions
                  << " pvs: scout=" << info.pvsStats.scoutSearches
                  << " re%=" << std::fixed << std::setprecision(1) << pvsReRate
                  << " null: att=" << info.nullMoveStats.attempts
                  << " cut=" << info.nullMoveStats.cutoffs
                  << " cut%=" << std::fixed << std::setprecision(1) << nullRate
                  << " extra(cand=" << info.nullMoveStats.aggressiveCandidates
                  << ",app=" << info.nullMoveStats.aggressiveApplied
                  << ",blk=" << info.nullMoveStats.aggressiveBlockedByTT
                  << ",sup=" << info.nullMoveStats.aggressiveSuppressed
                  << ",cut=" << info.nullMoveStats.aggressiveCutoffs
                  << ",vpass=" << info.nullMoveStats.aggressiveVerifyPasses
                  << ",vfail=" << info.nullMoveStats.aggressiveVerifyFails
                  << ",cap=" << info.nullMoveStats.aggressiveCapHits << ")"
                  << " no-store=" << info.nullMoveStats.nullMoveNoStore
                  << " static-cut=" << info.nullMoveStats.staticCutoffs
                  << " static-no-store=" << info.nullMoveStats.staticNullNoStore
                  << " prune: fut=" << info.futilityPruned
                  << " mcp=" << info.moveCountPruned
                  << " razor: att=" << info.razoring.attempts
                  << " cut=" << info.razoring.cutoffs
                  << " cut%=" << std::fixed << std::setprecision(1) 
                  << (info.razoring.attempts > 0 ? 100.0 * info.razoring.cutoffs / info.razoring.attempts : 0.0)
                  << " skips(tact=" << info.razoring.tacticalSkips
                  << ",tt=" << info.razoring.ttContextSkips
                  << ",eg=" << info.razoring.endgameSkips << ")"
                  << " razor_b=[" << info.razoring.depthBuckets[0] << "," << info.razoring.depthBuckets[1] << "]"
                  << " asp: att=" << info.aspiration.attempts
                  << " low=" << info.aspiration.failLow
                  << " high=" << info.aspiration.failHigh
                  << " fut_b=[" << info.pruneBreakdown.futility[0] << "," << info.pruneBreakdown.futility[1]
                  << "," << info.pruneBreakdown.futility[2] << "," << info.pruneBreakdown.futility[3] << "]"
                  << " fut_eff_b=[" << info.pruneBreakdown.futilityEff[0] << "," << info.pruneBreakdown.futilityEff[1]
                  << "," << info.pruneBreakdown.futilityEff[2] << "," << info.pruneBreakdown.futilityEff[3] << "]"
                  << " mcp_b=[" << info.pruneBreakdown.moveCount[0] << "," << info.pruneBreakdown.moveCount[1]
                  << "," << info.pruneBreakdown.moveCount[2] << "," << info.pruneBreakdown.moveCount[3] << "]"
                  << " illegal: first=" << info.illegalPseudoBeforeFirst
                  << " total=" << info.illegalPseudoTotal
                  << " hist(apps=" << info.historyStats.totalApplications()
                  << ",basic=" << info.historyStats.basicApplications
                  << ",cmh=" << info.historyStats.counterApplications
                  << ",first=" << info.historyStats.basicFirstMoveHits << "+" << info.historyStats.counterFirstMoveHits
                  << ",cuts=" << info.historyStats.basicCutoffs << "+" << info.historyStats.counterCutoffs
                  << ",re=" << info.historyStats.totalReSearches() << ")"
                  << std::endl;

        if (info.isSingularTelemetryEnabled()) {
            const auto singularTotals = info.singularTotals().snapshot();
            if (!singularTotals.empty()) {
            std::cout << "info string SingularStats: examined=" << singularTotals.candidatesExamined
                      << " qualified=" << singularTotals.candidatesQualified
                      << " rej_illegal=" << singularTotals.candidatesRejectedIllegal
                      << " rej_tactical=" << singularTotals.candidatesRejectedTactical
                      << " fail_low=" << singularTotals.verificationFailLow
                      << " fail_high=" << singularTotals.verificationFailHigh
                      << " verified=" << singularTotals.verificationsStarted
                      << " extended=" << singularTotals.extensionsApplied
                      << " cacheHits=" << singularTotals.verificationCacheHits
                          << " maxDepth=" << singularTotals.maxExtensionDepth
                          << std::endl;
                if (singularTotals.stackingCandidates > 0 || singularTotals.stackingApplied > 0 ||
                    singularTotals.stackingRejectedDepth > 0 || singularTotals.stackingRejectedEval > 0 ||
                    singularTotals.stackingRejectedTT > 0 || singularTotals.stackingBudgetClamped > 0 ||
                    singularTotals.stackingExtraDepth > 0) {
                    std::cout << "info string SingularStack: cand=" << singularTotals.stackingCandidates
                              << " app=" << singularTotals.stackingApplied
                              << " rej_depth=" << singularTotals.stackingRejectedDepth
                              << " rej_eval=" << singularTotals.stackingRejectedEval
                              << " rej_tt=" << singularTotals.stackingRejectedTT
                              << " clamp=" << singularTotals.stackingBudgetClamped
                              << " extra=" << singularTotals.stackingExtraDepth
                              << std::endl;
                }
            }
        }
    }
    
    // Node explosion diagnostics: Display collected statistics
    if (limits.nodeExplosionDiagnostics) {
        g_nodeExplosionStats.displayStats(info.nodes, info.qsearchNodes);
        g_nodeExplosionStats.reset();  // Reset for next search
    }

    if (info.isSingularTelemetryEnabled()) {
        info.flushSingularTelemetry(false);
    }
    return bestMove;
}

// Iterative deepening search controller (original)
Move search(Board& board, const SearchLimits& limits, TranspositionTable* tt) {
    // Debug output (removed in release builds)
    #ifndef NDEBUG
    std::cerr << "Search: Starting with maxDepth=" << limits.maxDepth << std::endl;
    std::cerr << "Search: movetime=" << limits.movetime.count() << "ms" << std::endl;
    #endif
    
    // Initialize search tracking
    SearchInfo searchInfo;  // For repetition detection
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());  // Capture current game history size
    
    SearchData info;  // For search statistics
    
    // PERFORMANCE FIX: Allocate move ordering tables on stack
    // Same fix as in searchIterativeTest()
    KillerMoves killerMoves;
    HistoryHeuristic historyHeuristic;
    CounterMoves counterMovesTable;
    
    // Phase 4.3.a: Allocate counter-move history on heap
    std::unique_ptr<CounterMoveHistory> counterMoveHistoryPtr = std::make_unique<CounterMoveHistory>();
    
    // Connect pointers to stack-allocated objects
    info.killers = &killerMoves;
    info.history = &historyHeuristic;
    info.counterMoves = &counterMovesTable;
    info.counterMoveHistory = counterMoveHistoryPtr.get();

    // UCI Score Conversion FIX: Store root side-to-move for correct UCI output
    info.rootSideToMove = board.sideToMove();
    info.setSingularTelemetryEnabled(limits.useSingularExtensions);
    
    // Stage 20, Phase B3 Fix: Clear history only at search start,
    // not between iterative deepening iterations
    // History accumulates across ID iterations for better move ordering
    
    // Stage 14, Deliverable 1.8: Pass quiescence option to search
    info.useQuiescence = limits.useQuiescence;
    
    // Stage 14 Remediation: Parse SEE modes once at search start to avoid hot path parsing
    info.seePruningModeEnum = parseSEEPruningMode(limits.seePruningMode);
    info.seePruningModeEnumQ = parseSEEPruningMode(limits.seePruningModeQ);
    
    // Stage 18: Initialize LMR parameters from limits
    info.lmrParams.enabled = limits.lmrEnabled;
    info.lmrParams.minDepth = limits.lmrMinDepth;
    info.lmrParams.minMoveNumber = limits.lmrMinMoveNumber;
    info.lmrParams.baseReduction = limits.lmrBaseReduction;
    info.lmrParams.depthFactor = limits.lmrDepthFactor;
    info.lmrParams.historyThreshold = limits.lmrHistoryThreshold;
    info.lmrParams.pvReduction = limits.lmrPvReduction;
    info.lmrParams.nonImprovingBonus = limits.lmrNonImprovingBonus;
    info.lmrStats.reset();
    
    // Stage 13, Deliverable 2.2b: Use new time management in regular search too
    TimeLimits timeLimits = calculateTimeLimits(limits, board, 1.0);
    info.timeLimit = timeLimits.optimum;
    // Phase 1a: Store soft and hard limits for better time management
    auto softLimit = timeLimits.soft;
    auto hardLimit = timeLimits.hard;
    std::cerr << "Search: using NEW time management - optimum=" << info.timeLimit.count() << "ms" 
              << ", soft=" << softLimit.count() << "ms"
              << ", hard=" << hardLimit.count() << "ms" << std::endl;
    
    Move bestMove;
    // Detect depth-only (fixed-depth) searches where no time controls are provided
    const bool depthOnlySearch = (
        limits.movetime == std::chrono::milliseconds(0) &&
        limits.time[WHITE] == std::chrono::milliseconds(0) &&
        limits.time[BLACK] == std::chrono::milliseconds(0)
    );
    
    // Debug: Show search parameters (removed in release)
    #ifndef NDEBUG
    std::cerr << "Search: maxDepth=" << limits.maxDepth 
              << " movetime=" << limits.movetime.count() 
              << "ms" << std::endl;
    #endif
    
    // Phase 1c: Track iteration times for EBF prediction
    std::chrono::milliseconds lastIterationTime(0);
    double lastEBF = 3.0;  // Conservative default EBF
    
    // Iterative deepening loop
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        // Check external stop flag before starting new iteration
        if (limits.stopFlag && limits.stopFlag->load(std::memory_order_relaxed)) {
            info.stopped = true;
            break;
        }
        
        info.depth = depth;
        
        // Track iteration start time
        auto iterationStart = std::chrono::steady_clock::now();
        
        // Set search mode at depth 1
        board.setSearchMode(true);
#ifdef DEBUG
        if (depth == 1) {
            Board::resetCounters();
        }
#endif
        
        // Search with infinite window initially
        eval::Score score = negamax(board, depth, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info, limits, tt,
                                   nullptr);  // Phase PV1: Pass nullptr for now
        
        // Clear search mode after search
        board.setSearchMode(false);
        
        // Only update best move if search completed
        if (!info.stopped) {
            bestMove = info.bestMove;
            
            // Send UCI info about this iteration
            sendSearchInfo(info);
            
            // Phase 1c: Update iteration time and EBF for next iteration prediction
            auto iterationEnd = std::chrono::steady_clock::now();
            lastIterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                iterationEnd - iterationStart);
            
            // Get EBF from this iteration (if available)
            double currentEBF = info.effectiveBranchingFactor();
            if (currentEBF > 0) {
                lastEBF = currentEBF;
            }
            
            // Stop early if we found a mate
            if (score.is_mate_score()) {
                // If we found a mate, no need to search deeper
                break;
            }
            
            // Phase 1c: Use EBF prediction for time management
            // Replace Phase 1b's simple soft limit check with intelligent prediction
            // Skip time-based early termination for depth-only searches
            if (!depthOnlySearch && hardLimit.count() > 0 && info.timeLimit != std::chrono::milliseconds::max()) {
                auto elapsed = info.elapsed();
                
                // Never exceed hard limit
                if (elapsed >= hardLimit) {
                    std::cerr << "[Time Management] Stopping at depth " << depth 
                              << " - reached hard limit (" << elapsed.count() << "ms >= " 
                              << hardLimit.count() << "ms)\n";
                    break;
                }
                
                // Use EBF prediction to decide if we have time for next iteration
                if (depth >= 2 && lastIterationTime.count() > 0) {
                    // Predict time for next iteration using our sophisticated prediction function
                    auto predictedNext = search::predictNextIterationTime(
                        lastIterationTime, lastEBF, depth);
                    
                    // Don't start an iteration we can't finish
                    if (elapsed + predictedNext > hardLimit) {
                        std::cerr << "[Time Management] Stopping at depth " << depth 
                                  << " - predicted next iteration (" << predictedNext.count() 
                                  << "ms) would exceed hard limit (elapsed: " << elapsed.count() 
                                  << "ms, hard: " << hardLimit.count() << "ms)\n";
                        break;
                    }
                    
                    // Use optimum time as a guideline, not a hard stop
                    // Allow going over optimum if we predict we can complete another iteration
                    if (elapsed > timeLimits.optimum && depth >= 8) {
                        // At reasonable depth and past optimum time
                        // Check if next iteration would take us well past optimum
                        if (elapsed + predictedNext > timeLimits.optimum * 2) {
                            std::cerr << "[Time Management] Stopping at depth " << depth 
                                      << " - sufficient depth reached and next iteration would exceed 2x optimum\n";
                            break;
                        }
                    }
                }
            }
        } else {
            // Time ran out during this iteration
            break;
        }
    }
    
#ifdef DEBUG
    // Print instrumentation counters at end of search - only in debug builds
    Board::printCounters();
#endif
    
    // Node explosion diagnostics: Display collected statistics
    if (limits.nodeExplosionDiagnostics) {
        g_nodeExplosionStats.displayStats(info.nodes, info.qsearchNodes);
        g_nodeExplosionStats.reset();  // Reset for next search
    }

    // Return the best move found
    // If no iterations completed, bestMove will be invalid
    return bestMove;
}

// Calculate time allocation for this move
std::chrono::milliseconds calculateTimeLimit(const SearchLimits& limits, 
                                            const Board& board) {
    // Fixed move time takes priority
    if (limits.movetime > std::chrono::milliseconds(0)) {
        return limits.movetime;
    }
    
    // Infinite analysis mode
    if (limits.infinite) {
        return std::chrono::milliseconds::max();
    }
    
    // Game time management
    Color stm = board.sideToMove();
    auto remaining = limits.time[stm];
    
    // If no time specified, use a default
    if (remaining == std::chrono::milliseconds(0)) {
        return std::chrono::milliseconds(5000);  // 5 seconds default
    }
    
    // Simple time allocation:
    // Use 5% of remaining time + 75% of increment
    auto base = remaining / 20;  // 5% of remaining
    auto increment = limits.inc[stm] * 3 / 4;  // 75% of increment
    auto allocated = base + increment;
    
    // Apply safety bounds
    // Minimum 5ms to do something
    allocated = std::max(allocated, std::chrono::milliseconds(5));
    
    // Never use more than 25% of remaining time in one move
    auto maxTime = remaining / 4;
    allocated = std::min(allocated, maxTime);
    
    // Keep at least 50ms buffer
    if (remaining > std::chrono::milliseconds(100)) {
        allocated = std::min(allocated, remaining - std::chrono::milliseconds(50));
    }
    
    return allocated;
}

// Send UCI info output
void sendSearchInfo(const SearchData& info) {
    std::cout << "info"
              << " depth " << info.depth
              << " seldepth " << info.seldepth;
    
    // Output score
    if (info.bestScore.is_mate_score()) {
        // Mate score - calculate moves to mate
        int mateIn = 0;
        if (info.bestScore > eval::Score::zero()) {
            // We have a mate
            mateIn = (eval::Score::mate().value() - info.bestScore.value() + 1) / 2;
        } else {
            // We are being mated
            mateIn = -(eval::Score::mate().value() + info.bestScore.value()) / 2;
        }
        std::cout << " score mate " << mateIn;
    } else {
        std::cout << " score cp " << info.bestScore.to_cp();
    }
    
    std::cout << " nodes " << info.nodes
              << " nps " << info.nps()
              << " time " << info.elapsed().count();
    
    // Add search efficiency statistics (only if we have beta cutoffs)
    if (info.betaCutoffs > 0) {
        std::cout << " ebf " << std::fixed << std::setprecision(2) 
                  << info.effectiveBranchingFactor()
                  << " moveeff " << std::fixed << std::setprecision(1)
                  << info.moveOrderingEfficiency() << "%";
    }
    
    // Add TT statistics if we have probes
    if (info.ttProbes > 0) {
        double hitRate = (100.0 * info.ttHits) / info.ttProbes;
        std::cout << " tthits " << std::fixed << std::setprecision(1) << hitRate << "%";
        if (info.ttCutoffs > 0) {
            std::cout << " ttcuts " << info.ttCutoffs;
        }
        if (info.ttStores > 0) {
            std::cout << " ttstores " << info.ttStores;
        }
    }
    
    // Output principal variation (just the best move for now)
    // Bug #013 fix: Validate move is legal before outputting to prevent illegal PV moves
    if (info.bestMove != Move()) {
        // Quick validation - check that from and to squares are valid
        Square from = moveFrom(info.bestMove);
        Square to = moveTo(info.bestMove);
        if (from < 64 && to < 64 && from != to) {
            std::cout << " pv " << SafeMoveExecutor::moveToString(info.bestMove);
        } else {
            // Log warning about corrupted move but don't output it
            std::cerr << "WARNING: Corrupted bestMove detected in sendUCIInfo: " 
                      << std::hex << info.bestMove << std::dec << std::endl;
        }
    }
    
    std::cout << std::endl;
}

// Phase 1: Send current search info during a search (periodic updates)
// Phase 5: Refactored to use InfoBuilder for cleaner construction
void sendCurrentSearchInfo(const IterativeSearchData& info, Color sideToMove, TranspositionTable* tt) {
    uci::InfoBuilder builder;
    
    // Basic depth info
    builder.appendDepth(info.depth, info.seldepth);
    
    // Score - UCI-compliant side-to-move perspective
    builder.appendScore(info.bestScore, sideToMove);
    
    // Node statistics
    builder.appendNodes(info.nodes)
           .appendNps(info.nps())
           .appendTime(info.elapsed().count());
    
    // Phase 4: Hashfull
    if (tt) {
        builder.appendHashfull(tt->hashfull());
    }
    
    // Phase 2: Currmove info for long searches
    if (info.currentRootMove != NO_MOVE && info.currentRootMoveNumber > 0) {
        // Force accurate time calculation for periodic updates
        auto now = std::chrono::steady_clock::now();
        auto actualElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.startTime);
        
        // Show currmove after 3 seconds
        if (actualElapsed.count() > 3000) {
            builder.appendCurrmove(info.currentRootMove, info.currentRootMoveNumber);
        }
    }
    
    // Principal variation
    if (info.bestMove != NO_MOVE) {
        builder.appendPv(info.bestMove);
    }

    if (info.isSingularTelemetryEnabled()) {
        const auto singularTotals = info.singularTotals().snapshot();
            if (!singularTotals.empty()) {
                builder.appendCustom("se_exam", std::to_string(singularTotals.candidatesExamined));
                if (singularTotals.candidatesQualified > 0) {
                    builder.appendCustom("se_qual", std::to_string(singularTotals.candidatesQualified));
                }
                if (singularTotals.candidatesRejectedIllegal > 0) {
                    builder.appendCustom("se_illegal", std::to_string(singularTotals.candidatesRejectedIllegal));
                }
                if (singularTotals.candidatesRejectedTactical > 0) {
                    builder.appendCustom("se_tact", std::to_string(singularTotals.candidatesRejectedTactical));
                }
                if (singularTotals.verificationFailLow > 0) {
                    builder.appendCustom("se_low", std::to_string(singularTotals.verificationFailLow));
                }
                if (singularTotals.verificationFailHigh > 0) {
                    builder.appendCustom("se_high", std::to_string(singularTotals.verificationFailHigh));
                }
                builder.appendCustom("se_ver", std::to_string(singularTotals.verificationsStarted));
                builder.appendCustom("se_ext", std::to_string(singularTotals.extensionsApplied));
            if (singularTotals.verificationCacheHits > 0) {
                builder.appendCustom("se_hits", std::to_string(singularTotals.verificationCacheHits));
            }
            if (singularTotals.maxExtensionDepth > 0) {
                builder.appendCustom("se_max", static_cast<int>(singularTotals.maxExtensionDepth));
            }
            if (singularTotals.stackingCandidates > 0) {
                builder.appendCustom("se_stack_c", std::to_string(singularTotals.stackingCandidates));
            }
            if (singularTotals.stackingApplied > 0) {
                builder.appendCustom("se_stack_a", std::to_string(singularTotals.stackingApplied));
            }
            if (singularTotals.stackingRejectedDepth > 0) {
                builder.appendCustom("se_stack_rd", std::to_string(singularTotals.stackingRejectedDepth));
            }
            if (singularTotals.stackingRejectedEval > 0) {
                builder.appendCustom("se_stack_re", std::to_string(singularTotals.stackingRejectedEval));
            }
            if (singularTotals.stackingRejectedTT > 0) {
                builder.appendCustom("se_stack_rt", std::to_string(singularTotals.stackingRejectedTT));
            }
            if (singularTotals.stackingBudgetClamped > 0) {
                builder.appendCustom("se_stack_cl", std::to_string(singularTotals.stackingBudgetClamped));
            }
            if (singularTotals.stackingExtraDepth > 0) {
                builder.appendCustom("se_stack_x", std::to_string(singularTotals.stackingExtraDepth));
            }
        }
    }

    std::cout << builder.build();
}

// Stage 13, Deliverable 5.1a: Enhanced UCI info output with iteration details
// Phase 5: Refactored to use InfoBuilder for cleaner construction
void sendIterationInfo(const IterativeSearchData& info, Color sideToMove, TranspositionTable* tt, const TriangularPV* pv) {
    uci::InfoBuilder builder;
    
    // Basic depth info
    builder.appendDepth(info.depth, info.seldepth);
    
    // Score - UCI-compliant side-to-move perspective
    builder.appendScore(info.bestScore, sideToMove);
    
    // Iteration-specific information
    if (info.hasIterations()) {
        const auto& lastIter = info.getLastIteration();
        
        // Stage 13, Deliverable 5.1b: Aspiration window reporting
        if (lastIter.depth >= AspirationConstants::MIN_DEPTH) {
            // Report window information
            if (lastIter.windowAttempts > 0) {
                // Window was used and there were re-searches
                if (lastIter.failedHigh) {
                    builder.appendString("fail-high(" + std::to_string(lastIter.windowAttempts) + ")");
                } else if (lastIter.failedLow) {
                    builder.appendString("fail-low(" + std::to_string(lastIter.windowAttempts) + ")");
                }
            }
            
            // Show window bounds for debugging (optional)
            if (lastIter.alpha != eval::Score::minus_infinity() || 
                lastIter.beta != eval::Score::infinity()) {
                std::string boundStr = "[";
                boundStr += (lastIter.alpha == eval::Score::minus_infinity() ? "-inf" : 
                            std::to_string(lastIter.alpha.to_cp()));
                boundStr += ",";
                boundStr += (lastIter.beta == eval::Score::infinity() ? "inf" : 
                            std::to_string(lastIter.beta.to_cp()));
                boundStr += "]";
                builder.appendCustom("bound", boundStr);
            }
        }
        
        // Stability indicator
        if (info.getIterationCount() >= 3) {
            if (info.isPositionStable()) {
                builder.appendString("stable");
            } else if (info.shouldExtendDueToInstability()) {
                builder.appendString("unstable");
            }
        }
        
        // Effective branching factor from sophisticated calculation
        double ebf = info.getSophisticatedEBF();
        if (ebf > 0) {
            builder.appendCustom("ebf", ebf);
        }
    }
    
    // Standard statistics
    builder.appendNodes(info.nodes)
           .appendNps(info.nps())
           .appendTime(info.elapsed().count());

    if (info.isSingularTelemetryEnabled()) {
        const auto singularTotals = info.singularTotals().snapshot();
        if (!singularTotals.empty()) {
            builder.appendCustom("se_exam", std::to_string(singularTotals.candidatesExamined));
            if (singularTotals.candidatesQualified > 0) {
                builder.appendCustom("se_qual", std::to_string(singularTotals.candidatesQualified));
            }
            if (singularTotals.candidatesRejectedIllegal > 0) {
                builder.appendCustom("se_illegal", std::to_string(singularTotals.candidatesRejectedIllegal));
            }
            if (singularTotals.candidatesRejectedTactical > 0) {
                builder.appendCustom("se_tact", std::to_string(singularTotals.candidatesRejectedTactical));
            }
            if (singularTotals.verificationFailLow > 0) {
                builder.appendCustom("se_low", std::to_string(singularTotals.verificationFailLow));
            }
            if (singularTotals.verificationFailHigh > 0) {
                builder.appendCustom("se_high", std::to_string(singularTotals.verificationFailHigh));
            }
            builder.appendCustom("se_ver", std::to_string(singularTotals.verificationsStarted));
            builder.appendCustom("se_ext", std::to_string(singularTotals.extensionsApplied));
            if (singularTotals.verificationCacheHits > 0) {
                builder.appendCustom("se_hits", std::to_string(singularTotals.verificationCacheHits));
            }
            if (singularTotals.maxExtensionDepth > 0) {
                builder.appendCustom("se_max", static_cast<int>(singularTotals.maxExtensionDepth));
            }
            if (singularTotals.stackingCandidates > 0) {
                builder.appendCustom("se_stack_c", std::to_string(singularTotals.stackingCandidates));
            }
            if (singularTotals.stackingApplied > 0) {
                builder.appendCustom("se_stack_a", std::to_string(singularTotals.stackingApplied));
            }
            if (singularTotals.stackingRejectedDepth > 0) {
                builder.appendCustom("se_stack_rd", std::to_string(singularTotals.stackingRejectedDepth));
            }
            if (singularTotals.stackingRejectedEval > 0) {
                builder.appendCustom("se_stack_re", std::to_string(singularTotals.stackingRejectedEval));
            }
            if (singularTotals.stackingRejectedTT > 0) {
                builder.appendCustom("se_stack_rt", std::to_string(singularTotals.stackingRejectedTT));
            }
            if (singularTotals.stackingBudgetClamped > 0) {
                builder.appendCustom("se_stack_cl", std::to_string(singularTotals.stackingBudgetClamped));
            }
            if (singularTotals.stackingExtraDepth > 0) {
                builder.appendCustom("se_stack_x", std::to_string(singularTotals.stackingExtraDepth));
            }
        }
    }

    // Move ordering efficiency
    if (info.betaCutoffs > 0) {
        builder.appendCustom("moveeff", info.moveOrderingEfficiency());
    }
    
    // TT statistics
    if (info.ttProbes > 0) {
        double hitRate = (100.0 * info.ttHits) / info.ttProbes;
        builder.appendCustom("tthits", hitRate);
    }
    
    // Phase 4: Hashfull
    if (tt) {
        builder.appendHashfull(tt->hashfull());
    }
    
    // Phase PV4: Output full principal variation
    if (pv != nullptr && !pv->isEmpty(0)) {
        // Extract full PV from root
        std::vector<Move> pvMoves = pv->extractPV(0);
        if (!pvMoves.empty()) {
            // Validate moves and build clean PV
            std::vector<Move> validPvMoves;
            for (Move move : pvMoves) {
                // Validate each move before adding
                Square from = moveFrom(move);
                Square to = moveTo(move);
                if (from < 64 && to < 64 && from != to) {
                    validPvMoves.push_back(move);
                } else {
                    // Stop if we hit a corrupted move
                    break;
                }
            }
            // Pass entire PV vector to builder (it will add "pv" once)
            if (!validPvMoves.empty()) {
                builder.appendPv(validPvMoves);
            }
        }
    } else if (info.bestMove != Move()) {
        // Fallback: Output just the best move if no PV available
        // Bug #013 fix: Validate move is legal before outputting
        Square from = moveFrom(info.bestMove);
        Square to = moveTo(info.bestMove);
        if (from < 64 && to < 64 && from != to) {
            builder.appendPv(info.bestMove);
        } else {
            // Log warning about corrupted move but don't output it
            std::cerr << "WARNING: Corrupted bestMove detected in sendIterationInfo: " 
                      << std::hex << info.bestMove << std::dec << std::endl;
        }
    }
    
    std::cout << builder.build();
}

} // namespace seajay::search
