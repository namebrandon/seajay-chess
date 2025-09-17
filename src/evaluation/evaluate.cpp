#include "evaluate.h"
#include "eval_trace.h"  // For evaluation tracing
#include "material.h"
#include "pst.h"  // Stage 9: Include PST header
#include "pawn_structure.h"  // Phase PP2: Passed pawn evaluation
#include "pawn_eval.h"
#include "king_safety.h"  // Phase KS2: King safety evaluation
#include "../core/board.h"
#include "../core/bitboard.h"
#include "../core/move_generation.h"  // MOB2: For mobility calculation
#include "../core/simd_utils.h"  // Phase 2.5.e-3: SIMD optimizations
#include "../core/engine_config.h"  // PST Phase Interpolation: For UCI options
#include "../search/game_phase.h"  // Phase PP2: For phase scaling
#include <cstdlib>  // PP3b: For std::abs
#include <algorithm>  // For std::clamp

namespace seajay::eval {

// PST Phase Interpolation - Phase calculation (continuous, fast)
// Returns phase value from 0 (pure endgame) to 256 (pure middlegame)
inline int phase0to256(const Board& board) noexcept {
    // Material weights for phase calculation
    // P=0 (pawns don't affect phase), N=1, B=1, R=2, Q=4, K=0
    constexpr int PHASE_WEIGHT[6] = { 0, 1, 1, 2, 4, 0 };
    
    // Maximum phase = 2*(N+N+B+B+R+R+Q) = 2*(1+1+1+1+2+2+4) = 24
    constexpr int TOTAL_PHASE = 24;
    
    // Count non-pawn material
    int phase = 0;
    phase += popCount(board.pieces(KNIGHT)) * PHASE_WEIGHT[KNIGHT];
    phase += popCount(board.pieces(BISHOP)) * PHASE_WEIGHT[BISHOP];
    phase += popCount(board.pieces(ROOK))   * PHASE_WEIGHT[ROOK];
    phase += popCount(board.pieces(QUEEN))  * PHASE_WEIGHT[QUEEN];
    
    // Scale to [0,256] with rounding
    // 256 = full middlegame, 0 = pure endgame
    return std::clamp((phase * 256 + TOTAL_PHASE/2) / TOTAL_PHASE, 0, 256);
}
// Template implementation for evaluation with optional tracing
template<bool Traced>
Score evaluateImpl(const Board& board, EvalTrace* trace = nullptr) {
    // Reset trace if provided
    if constexpr (Traced) {
        if (trace) trace->reset();
    }
    
    // Get material from board
    const Material& material = board.material();
    
    // Check for insufficient material draws
    if (material.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Check for same-colored bishops (KB vs KB)
    // This is a special case where we need board position info
    if (material.count(WHITE, BISHOP) == 1 && 
        material.count(BLACK, BISHOP) == 1 &&
        material.count(WHITE, PAWN) == 0 && 
        material.count(BLACK, PAWN) == 0 &&
        material.count(WHITE, KNIGHT) == 0 && 
        material.count(BLACK, KNIGHT) == 0 &&
        material.count(WHITE, ROOK) == 0 && 
        material.count(BLACK, ROOK) == 0 &&
        material.count(WHITE, QUEEN) == 0 && 
        material.count(BLACK, QUEEN) == 0) {
        
        // Find the bishops and check if they're on same color squares
        Bitboard whiteBishops = board.pieces(WHITE, BISHOP);
        Bitboard blackBishops = board.pieces(BLACK, BISHOP);
        
        if (whiteBishops && blackBishops) {
            Square wbSq = lsb(whiteBishops);
            Square bbSq = lsb(blackBishops);
            
            // Check if both bishops are on same color squares
            // (rank + file) % 2 == 0 means dark square
            bool wbDark = ((wbSq / 8) + (wbSq % 8)) % 2 == 0;
            bool bbDark = ((bbSq / 8) + (bbSq % 8)) % 2 == 0;
            
            if (wbDark == bbDark) {
                return Score::draw();  // Same colored bishops = draw
            }
        }
    }
    
    // Get PST score (Stage 9)
    // PST score is stored from white's perspective in the board
    const MgEgScore& pstScore = board.pstScore();
    
    Score pstValue;
    if (seajay::getConfig().usePSTInterpolation) {
        // PST Phase Interpolation - calculate phase and interpolate mg/eg values
        int phase = phase0to256(board);  // 256 = full MG, 0 = full EG
        int invPhase = 256 - phase;      // Inverse phase for endgame weight
        
        // Trace phase and PST components
        if constexpr (Traced) {
            if (trace) {
                trace->phase256 = phase;
                trace->pstMg = pstScore.mg;
                trace->pstEg = pstScore.eg;
            }
        }
        
        // Fixed-point blend with rounding (shift by 8 to divide by 256)
        int blendedPst = (pstScore.mg.value() * phase + pstScore.eg.value() * invPhase + 128) >> 8;
        pstValue = Score(blendedPst);
    } else {
        // Original behavior - use only middlegame PST values (no tapering)
        pstValue = pstScore.mg;
    }
    
    // Trace PST value
    if constexpr (Traced) {
        if (trace) trace->pst = pstValue;
    }
    
    // Phase PP2: Passed pawn evaluation
    // Get pawn bitboards
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPawns = board.pieces(BLACK, PAWN);
    
    // Phase 2.5.b: Cache frequently used values at the start
    // Cache game phase once (was being called 3 times)
    search::GamePhase gamePhase = search::detectGamePhase(board);
    
    // Cache king squares once (was being fetched multiple times)
    Square whiteKingSquare = board.kingSquare(WHITE);
    Square blackKingSquare = board.kingSquare(BLACK);
    
    // Cache side to move (accessed 4 times)
    Color sideToMove = board.sideToMove();
    
    // Cache pure endgame check once (was calculated twice with identical logic)
    bool isPureEndgame = board.pieces(WHITE, KNIGHT) == 0 &&
                        board.pieces(WHITE, BISHOP) == 0 &&
                        board.pieces(WHITE, ROOK) == 0 &&
                        board.pieces(WHITE, QUEEN) == 0 &&
                        board.pieces(BLACK, KNIGHT) == 0 &&
                        board.pieces(BLACK, BISHOP) == 0 &&
                        board.pieces(BLACK, ROOK) == 0 &&
                        board.pieces(BLACK, QUEEN) == 0;
    
    PawnEntry scratchEntry;
    const PawnEntry& pawnEntry = getOrBuildPawnEntry(board, scratchEntry);
    const PawnEvalSummary pawnSummary = computePawnEval(
        board,
        pawnEntry,
        material,
        gamePhase,
        isPureEndgame,
        sideToMove,
        whiteKingSquare,
        blackKingSquare,
        whitePawns,
        blackPawns);

    Score passedPawnScore = pawnSummary.passed;
    Score isolatedPawnScore = pawnSummary.isolated;
    Score doubledPawnScore = pawnSummary.doubled;
    Score pawnIslandScore = pawnSummary.islands;
    Score backwardPawnScore = pawnSummary.backward;

    if constexpr (Traced) {
        if (trace) {
            trace->passedPawns = pawnSummary.passed;
            trace->passedDetail.whiteCount = pawnSummary.whitePassedCount;
            trace->passedDetail.blackCount = pawnSummary.blackPassedCount;
            trace->isolatedPawns = pawnSummary.isolated;
            trace->doubledPawns = pawnSummary.doubled;
            trace->pawnIslands = pawnSummary.islands;
            trace->backwardPawns = pawnSummary.backward;
        }
    }
    
    // BPB2: Bishop pair bonus integration (Phase 2 - conservative values)
    // Detect if each side has the bishop pair
    bool whiteBishopPair = (material.count(WHITE, BISHOP) >= 2);
    bool blackBishopPair = (material.count(BLACK, BISHOP) >= 2);
    
    // Phase 2: Apply conservative bishop pair bonus
    // Starting with 20cp midgame, 50cp endgame (will tune in Phase 3)
    static constexpr int BISHOP_PAIR_BONUS_MG = 20;
    static constexpr int BISHOP_PAIR_BONUS_EG = 50;
    
    int bishopPairValue = 0;
    
    // Apply phase-scaled bonus
    int bonus = 0;
    switch (gamePhase) {
        case search::GamePhase::OPENING:
        case search::GamePhase::MIDDLEGAME:
            bonus = BISHOP_PAIR_BONUS_MG;
            break;
        case search::GamePhase::ENDGAME:
            bonus = BISHOP_PAIR_BONUS_EG;
            break;
    }
    
    // Apply bonus for white, penalty for black (from white's perspective)
    if (whiteBishopPair) bishopPairValue += bonus;
    if (blackBishopPair) bishopPairValue -= bonus;
    
    Score bishopPairScore(bishopPairValue);
    
    // Trace bishop pair score
    if constexpr (Traced) {
        if (trace) trace->bishopPair = bishopPairScore;
    }
    
    // MOB2: Piece mobility evaluation (Phase 2 - basic integration)
    // Count pseudo-legal moves for pieces, excluding squares attacked by enemy pawns
    
    // Get all occupied squares
    Bitboard occupied = board.occupied();
    
    // Calculate squares attacked by pawns (for safer mobility calculation)
    Bitboard whitePawnAttacks = 0;
    Bitboard blackPawnAttacks = 0;
    
    // Calculate pawn attacks for each side
    Bitboard wp = whitePawns;
    while (wp) {
        Square sq = popLsb(wp);
        whitePawnAttacks |= pawnAttacks(WHITE, sq);
    }
    
    Bitboard bp = blackPawns;
    while (bp) {
        Square sq = popLsb(bp);
        blackPawnAttacks |= pawnAttacks(BLACK, sq);
    }
    
    // Calculate pawn attack spans - all squares pawns could ever attack
    // For white pawns: fill forward and expand diagonally
    // For black pawns: fill backward and expand diagonally
    auto calculatePawnAttackSpan = [](Bitboard pawns, Color color) -> Bitboard {
        Bitboard span = 0;
        
        if (color == WHITE) {
            // Fill all squares forward of white pawns
            Bitboard filled = pawns;
            for (int i = 0; i < 6; i++) {  // Max 6 ranks to advance
                filled |= (filled << 8) & ~RANK_8_BB;  // Move forward one rank
            }
            // Expand to diagonals (squares that could be attacked)
            span = ((filled & ~FILE_A_BB) << 7) | ((filled & ~FILE_H_BB) << 9);
            // Remove rank 1 and 2 (pawns can't attack backwards)
            span &= ~(RANK_1_BB | RANK_2_BB);
        } else {
            // Fill all squares backward of black pawns  
            Bitboard filled = pawns;
            for (int i = 0; i < 6; i++) {  // Max 6 ranks to advance
                filled |= (filled >> 8) & ~RANK_1_BB;  // Move backward one rank
            }
            // Expand to diagonals (squares that could be attacked)
            span = ((filled & ~FILE_H_BB) >> 7) | ((filled & ~FILE_A_BB) >> 9);
            // Remove rank 7 and 8 (pawns can't attack backwards)
            span &= ~(RANK_7_BB | RANK_8_BB);
        }
        
        return span;
    };
    
    Bitboard whitePawnAttackSpan = calculatePawnAttackSpan(whitePawns, WHITE);
    Bitboard blackPawnAttackSpan = calculatePawnAttackSpan(blackPawns, BLACK);
    
    // Get knight bitboards for outpost evaluation
    Bitboard whiteKnights = board.pieces(WHITE, KNIGHT);
    Bitboard blackKnights = board.pieces(BLACK, KNIGHT);
    
    // Knight outpost evaluation (simplified to match Stash engine)
    // An outpost is a square where a knight:
    // 1. Cannot be attacked by enemy pawns (ever - using attack spans)
    // 2. Is protected by friendly pawns
    // 3. Is in enemy territory (ranks 4-6 for white, ranks 3-5 for black)
    
    // Define outpost ranks for each side
    constexpr Bitboard WHITE_OUTPOST_RANKS = RANK_4_BB | RANK_5_BB | RANK_6_BB;
    constexpr Bitboard BLACK_OUTPOST_RANKS = RANK_3_BB | RANK_4_BB | RANK_5_BB;
    
    // Simple outpost bonus (similar to Stash's 31cp, we use 35cp)
    constexpr int KNIGHT_OUTPOST_BONUS = 35;
    
    // Find potential outpost squares (safe from enemy pawn attack spans and protected by friendly pawns)
    // Now using attack spans for proper detection of squares that can never be attacked by pawns
    Bitboard whiteOutpostSquares = WHITE_OUTPOST_RANKS & ~blackPawnAttackSpan & whitePawnAttacks;
    Bitboard blackOutpostSquares = BLACK_OUTPOST_RANKS & ~whitePawnAttackSpan & blackPawnAttacks;
    
    // Find knights on outpost squares
    Bitboard whiteKnightOutposts = whiteKnights & whiteOutpostSquares;
    Bitboard blackKnightOutposts = blackKnights & blackOutpostSquares;
    
    // Count outposts and apply bonus
    int knightOutpostValue = 0;
    knightOutpostValue += popCount(whiteKnightOutposts) * KNIGHT_OUTPOST_BONUS;
    knightOutpostValue -= popCount(blackKnightOutposts) * KNIGHT_OUTPOST_BONUS;
    
    // Create score from total outpost value
    Score knightOutpostScore(knightOutpostValue);
    
    // Trace knight outpost score
    if constexpr (Traced) {
        if (trace) trace->knightOutposts = knightOutpostScore;
    }
    
    // Phase 2: Conservative mobility bonuses per move count
    // Phase 2.5.e-3: Using SIMD-optimized batched mobility calculation
    static constexpr int MOBILITY_BONUS_PER_MOVE = 2;  // 2 centipawns per available move
    
    int whiteMobilityScore = 0;
    int blackMobilityScore = 0;
    
    // Count white piece mobility with batched processing for better ILP
    // Knights - process in batches for better cache usage
    Bitboard wn = whiteKnights;
    {
        // Extract up to 4 knight positions for parallel processing
        Square knightSqs[4];
        int knightCount = 0;
        Bitboard tempKnights = wn;
        while (tempKnights && knightCount < 4) {
            knightSqs[knightCount++] = popLsb(tempKnights);
        }
        
        // Process all knights in parallel (compiler may vectorize)
        for (int i = 0; i < knightCount; ++i) {
            Bitboard attacks = MoveGenerator::getKnightAttacks(knightSqs[i]);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            int moveCount = popCount(attacks);
            whiteMobilityScore += moveCount * MOBILITY_BONUS_PER_MOVE;
            if constexpr (Traced) {
                if (trace) trace->mobilityDetail.whiteKnightMoves += moveCount;
            }
        }
        
        // Process remaining knights if any
        while (tempKnights) {
            Square sq = popLsb(tempKnights);
            Bitboard attacks = MoveGenerator::getKnightAttacks(sq);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Bishops - batch process for better ILP
    Bitboard whiteBishops = board.pieces(WHITE, BISHOP);
    Bitboard wb = whiteBishops;
    {
        // Process bishops in groups of 3 for register pressure balance
        Square bishopSqs[3];
        int bishopCount = 0;
        Bitboard tempBishops = wb;
        
        while (tempBishops && bishopCount < 3) {
            bishopSqs[bishopCount++] = popLsb(tempBishops);
        }
        
        // Calculate all bishop attacks in parallel
        for (int i = 0; i < bishopCount; ++i) {
            Bitboard attacks = MoveGenerator::getBishopAttacks(bishopSqs[i], occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
        
        // Process remaining bishops
        while (tempBishops) {
            Square sq = popLsb(tempBishops);
            Bitboard attacks = MoveGenerator::getBishopAttacks(sq, occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Rooks - batch process with file bonus tracking
    Bitboard whiteRooks = board.pieces(WHITE, ROOK);
    Bitboard wr = whiteRooks;
    // Phase ROF2: Track rook open file bonus separately
    int whiteRookFileBonus = 0;
    {
        // Batch process rooks for better ILP
        Square rookSqs[3];
        int rookCount = 0;
        Bitboard tempRooks = wr;
        
        while (tempRooks && rookCount < 3) {
            rookSqs[rookCount++] = popLsb(tempRooks);
        }
        
        // Process rooks in parallel
        for (int i = 0; i < rookCount; ++i) {
            Bitboard attacks = MoveGenerator::getRookAttacks(rookSqs[i], occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            // Phase ROF2: Add open/semi-open file bonuses
            int file = fileOf(rookSqs[i]);
            if (board.isOpenFile(file)) {
                whiteRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, WHITE)) {
                whiteRookFileBonus += 15;
            }
        }
        
        // Process remaining rooks
        while (tempRooks) {
            Square sq = popLsb(tempRooks);
            Bitboard attacks = MoveGenerator::getRookAttacks(sq, occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            int file = fileOf(sq);
            if (board.isOpenFile(file)) {
                whiteRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, WHITE)) {
                whiteRookFileBonus += 15;
            }
        }
    }
    
    // Queens - batch process (usually fewer queens)
    Bitboard whiteQueens = board.pieces(WHITE, QUEEN);
    Bitboard wq = whiteQueens;
    while (wq) {
        Square sq = popLsb(wq);
        // Queens use combined bishop and rook attacks
        Bitboard attacks = MoveGenerator::getQueenAttacks(sq, occupied);
        attacks &= ~board.pieces(WHITE);
        attacks &= ~blackPawnAttacks;
        whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
    }
    
    // Count black piece mobility with batched processing
    // Knights - batch process
    Bitboard bn = blackKnights;
    {
        Square knightSqs[4];
        int knightCount = 0;
        Bitboard tempKnights = bn;
        
        while (tempKnights && knightCount < 4) {
            knightSqs[knightCount++] = popLsb(tempKnights);
        }
        
        for (int i = 0; i < knightCount; ++i) {
            Bitboard attacks = MoveGenerator::getKnightAttacks(knightSqs[i]);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
        
        while (tempKnights) {
            Square sq = popLsb(tempKnights);
            Bitboard attacks = MoveGenerator::getKnightAttacks(sq);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Bishops - batch process
    Bitboard blackBishops = board.pieces(BLACK, BISHOP);
    Bitboard bb = blackBishops;
    {
        Square bishopSqs[3];
        int bishopCount = 0;
        Bitboard tempBishops = bb;
        
        while (tempBishops && bishopCount < 3) {
            bishopSqs[bishopCount++] = popLsb(tempBishops);
        }
        
        for (int i = 0; i < bishopCount; ++i) {
            Bitboard attacks = MoveGenerator::getBishopAttacks(bishopSqs[i], occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
        
        while (tempBishops) {
            Square sq = popLsb(tempBishops);
            Bitboard attacks = MoveGenerator::getBishopAttacks(sq, occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Rooks - batch process
    Bitboard blackRooks = board.pieces(BLACK, ROOK);
    Bitboard br = blackRooks;
    int blackRookFileBonus = 0;
    {
        Square rookSqs[3];
        int rookCount = 0;
        Bitboard tempRooks = br;
        
        while (tempRooks && rookCount < 3) {
            rookSqs[rookCount++] = popLsb(tempRooks);
        }
        
        for (int i = 0; i < rookCount; ++i) {
            Bitboard attacks = MoveGenerator::getRookAttacks(rookSqs[i], occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            int file = fileOf(rookSqs[i]);
            if (board.isOpenFile(file)) {
                blackRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, BLACK)) {
                blackRookFileBonus += 15;
            }
        }
        
        while (tempRooks) {
            Square sq = popLsb(tempRooks);
            Bitboard attacks = MoveGenerator::getRookAttacks(sq, occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            int file = fileOf(sq);
            if (board.isOpenFile(file)) {
                blackRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, BLACK)) {
                blackRookFileBonus += 15;
            }
        }
    }
    
    // Queens - usually fewer, process directly
    Bitboard blackQueens = board.pieces(BLACK, QUEEN);
    Bitboard bq = blackQueens;
    while (bq) {
        Square sq = popLsb(bq);
        Bitboard attacks = MoveGenerator::getQueenAttacks(sq, occupied);
        attacks &= ~board.pieces(BLACK);
        attacks &= ~whitePawnAttacks;
        blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
    }
    
    // Phase 2: Apply mobility difference to evaluation
    int mobilityValue = whiteMobilityScore - blackMobilityScore;
    Score mobilityScore(mobilityValue);
    
    // Trace mobility score
    if constexpr (Traced) {
        if (trace) trace->mobility = mobilityScore;
    }
    
    // Phase KS2: King safety evaluation (integrated but returns 0 score)
    // Evaluate for both sides and combine
    Score whiteKingSafety = KingSafety::evaluate(board, WHITE);
    Score blackKingSafety = KingSafety::evaluate(board, BLACK);
    
    // King safety is from each side's perspective, so we subtract black's from white's
    // Note: In Phase KS2, both will return 0 since enableScoring = 0
    Score kingSafetyScore = whiteKingSafety - blackKingSafety;
    
    // Trace king safety score
    if constexpr (Traced) {
        if (trace) trace->kingSafety = kingSafetyScore;
    }
    
    // Phase ROF2: Calculate rook file bonus score
    Score rookFileScore = Score(whiteRookFileBonus - blackRookFileBonus);
    
    // Trace rook file score
    if constexpr (Traced) {
        if (trace) trace->rookFiles = rookFileScore;
    }

    // Phase A3: Tiny bonus for king proximity to own rook in endgames only
    Score rookKingProximityScore = Score(0);
    if (gamePhase == search::GamePhase::ENDGAME) {
        auto kingDist = [](Square k, Square r) {
            int dr = std::abs(rankOf(k) - rankOf(r));
            int df = std::abs(fileOf(k) - fileOf(r));
            return dr + df;  // Manhattan distance
        };
        // White
        int wBonus = 0;
        Bitboard wrBB = board.pieces(WHITE, ROOK);
        if (wrBB) {
            int best = 99;
            Bitboard t = wrBB;
            while (t) {
                Square rsq = popLsb(t);
                best = std::min(best, kingDist(whiteKingSquare, rsq));
            }
            // Tiny bonus: closer king-rook gets up to ~6 cp
            wBonus = std::max(0, 6 - best);
        }
        // Black
        int bBonus = 0;
        Bitboard brBB = board.pieces(BLACK, ROOK);
        if (brBB) {
            int best = 99;
            Bitboard t = brBB;
            while (t) {
                Square rsq = popLsb(t);
                best = std::min(best, kingDist(blackKingSquare, rsq));
            }
            bBonus = std::max(0, 6 - best);
        }
        rookKingProximityScore = Score(wBonus - bBonus);
    }
    
    // Trace rook-king proximity score
    if constexpr (Traced) {
        if (trace) trace->rookKingProximity = rookKingProximityScore;
    }
    
    // Calculate total evaluation from white's perspective
    // Material difference + PST score + passed pawn score + isolated pawn score + doubled pawn score + island score + backward score + bishop pair + mobility + king safety + rook files + knight outposts
    
    // Phase-interpolated material evaluation
    Score materialDiff;
    if (seajay::getConfig().usePSTInterpolation) {
        // Use phase interpolation for material values (same phase as PST)
        int phase = phase0to256(board);  // 256 = full MG, 0 = full EG
        int invPhase = 256 - phase;      // Inverse phase for endgame weight
        
        // Get MG and EG material differences
        Score materialMg = material.valueMg(WHITE) - material.valueMg(BLACK);
        Score materialEg = material.valueEg(WHITE) - material.valueEg(BLACK);
        
        // Interpolate between mg and eg values
        int interpolated = (materialMg.value() * phase + materialEg.value() * invPhase) / 256;
        materialDiff = Score(interpolated);
    } else {
        // Use pure middlegame values (backward compatibility)
        materialDiff = material.value(WHITE) - material.value(BLACK);
    }
    
    // Trace material
    if constexpr (Traced) {
        if (trace) trace->material = materialDiff;
    }
    
    Score totalWhite = materialDiff + pstValue + passedPawnScore + isolatedPawnScore + doubledPawnScore + pawnIslandScore + backwardPawnScore + bishopPairScore + mobilityScore + kingSafetyScore + rookFileScore + rookKingProximityScore + knightOutpostScore;
    
    // Return from side-to-move perspective
    if (sideToMove == WHITE) {
        return totalWhite;
    } else {
        return -totalWhite;
    }
}

// Public interfaces for evaluation
Score evaluate(const Board& board) {
    // Normal evaluation with no tracing - zero overhead
    return evaluateImpl<false>(board, nullptr);
}

Score evaluateWithTrace(const Board& board, EvalTrace& trace) {
    // Evaluation with detailed tracing
    return evaluateImpl<true>(board, &trace);
}

#ifdef DEBUG
bool verifyMaterialIncremental(const Board& board) {
    // Recount material from scratch
    Material scratch;
    
    for (int i = 0; i < 64; ++i) {
        Square s = static_cast<Square>(i);
        Piece p = board.pieceAt(s);
        if (p != NO_PIECE) {
            scratch.add(p);
        }
    }
    
    // Compare with board's incremental material
    return scratch == board.material();
}
#endif

#ifndef NDEBUG
// Phase 3C.0: Reference function for parity checking
// Computes material + PST using the exact same code paths as evaluate()
// This is the "ground truth" for shadow parity checking
Score refMaterialPST(const Board& board) {
    // Get material from board
    const Material& material = board.material();
    
    // Check for insufficient material draws (use board check to match fastEvaluate)
    // board.isInsufficientMaterial() includes KB-vs-KB same-color check
    if (board.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Get PST score (same as evaluate())
    const MgEgScore& pstScore = board.pstScore();
    
    Score materialScore;
    Score pstValue;
    
    if (seajay::getConfig().usePSTInterpolation) {
        // Use exact same phase calculation as evaluate()
        int phase = phase0to256(board);  // 256 = full MG, 0 = full EG
        int invPhase = 256 - phase;      // Inverse phase for endgame weight
        
        // Material with phase interpolation (exact same math as evaluate() - no rounding)
        Score whiteMat = Score((material.valueMg(WHITE).value() * phase + 
                                material.valueEg(WHITE).value() * invPhase) / 256);
        Score blackMat = Score((material.valueMg(BLACK).value() * phase + 
                                material.valueEg(BLACK).value() * invPhase) / 256);
        
        // PST blend (exact same math as evaluate())
        int blendedPst = (pstScore.mg.value() * phase + pstScore.eg.value() * invPhase + 128) >> 8;
        pstValue = Score(blendedPst);
        
        // Material balance from side-to-move perspective
        Color stm = board.sideToMove();
        materialScore = stm == WHITE ? whiteMat - blackMat : blackMat - whiteMat;
        
        // PST from side-to-move perspective
        pstValue = stm == WHITE ? pstValue : -pstValue;
    } else {
        // Original behavior - use only middlegame values
        materialScore = material.balanceMg(board.sideToMove());
        pstValue = board.sideToMove() == WHITE ? pstScore.mg : -pstScore.mg;
    }
    
    // Return combined score from side-to-move perspective
    return materialScore + pstValue;
}
#endif

} // namespace seajay::eval
