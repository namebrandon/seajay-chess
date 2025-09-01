#include "evaluate.h"
#include "material.h"
#include "pst.h"  // Stage 9: Include PST header
#include "pawn_structure.h"  // Phase PP2: Passed pawn evaluation
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

Score evaluate(const Board& board) {
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
        
        // Fixed-point blend with rounding (shift by 8 to divide by 256)
        int blendedPst = (pstScore.mg.value() * phase + pstScore.eg.value() * invPhase + 128) >> 8;
        pstValue = Score(blendedPst);
    } else {
        // Original behavior - use only middlegame PST values (no tapering)
        pstValue = pstScore.mg;
    }
    
    // Phase PP2: Passed pawn evaluation
    // Rank-based bonuses (indexed by relative rank)
    static constexpr int PASSED_PAWN_BONUS[8] = {
        0,    // Rank 1 (no bonus for pawns on first rank)
        10,   // Rank 2
        17,   // Rank 3
        30,   // Rank 4
        60,   // Rank 5
        120,  // Rank 6
        180,  // Rank 7
        0     // Rank 8 (promoted, handled elsewhere)
    };
    
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
    
    // PPH2: Get cached pawn structure evaluation early
    uint64_t pawnKey = board.pawnZobristKey();
    PawnEntry* pawnEntry = g_pawnStructure.probe(pawnKey);
    
    Bitboard whiteIsolated, blackIsolated, whiteDoubled, blackDoubled;
    Bitboard whitePassedPawns, blackPassedPawns;
    Bitboard whiteBackward, blackBackward;  // BP2: Track backward pawns
    uint8_t whiteIslands, blackIslands;  // PI2: Track island counts
    
    if (!pawnEntry) {
        // Cache miss - compute pawn structure
        PawnEntry newEntry;
        newEntry.key = pawnKey;
        newEntry.valid = true;
        
        // Compute all pawn structure features
        newEntry.isolatedPawns[WHITE] = g_pawnStructure.getIsolatedPawns(WHITE, whitePawns);
        newEntry.isolatedPawns[BLACK] = g_pawnStructure.getIsolatedPawns(BLACK, blackPawns);
        newEntry.doubledPawns[WHITE] = g_pawnStructure.getDoubledPawns(WHITE, whitePawns);
        newEntry.doubledPawns[BLACK] = g_pawnStructure.getDoubledPawns(BLACK, blackPawns);
        newEntry.passedPawns[WHITE] = g_pawnStructure.getPassedPawns(WHITE, whitePawns, blackPawns);
        newEntry.passedPawns[BLACK] = g_pawnStructure.getPassedPawns(BLACK, blackPawns, whitePawns);
        // BP2: Compute and cache backward pawns
        newEntry.backwardPawns[WHITE] = g_pawnStructure.getBackwardPawns(WHITE, whitePawns, blackPawns);
        newEntry.backwardPawns[BLACK] = g_pawnStructure.getBackwardPawns(BLACK, blackPawns, whitePawns);
        // PI2: Compute and cache pawn island counts
        newEntry.pawnIslands[WHITE] = PawnStructure::countPawnIslands(whitePawns);
        newEntry.pawnIslands[BLACK] = PawnStructure::countPawnIslands(blackPawns);
        
        // Store in cache
        g_pawnStructure.store(pawnKey, newEntry);
        
        // Use computed values
        whiteIsolated = newEntry.isolatedPawns[WHITE];
        blackIsolated = newEntry.isolatedPawns[BLACK];
        whiteDoubled = newEntry.doubledPawns[WHITE];
        blackDoubled = newEntry.doubledPawns[BLACK];
        whitePassedPawns = newEntry.passedPawns[WHITE];
        blackPassedPawns = newEntry.passedPawns[BLACK];
        whiteBackward = newEntry.backwardPawns[WHITE];  // BP2
        blackBackward = newEntry.backwardPawns[BLACK];  // BP2
        whiteIslands = newEntry.pawnIslands[WHITE];  // PI2
        blackIslands = newEntry.pawnIslands[BLACK];  // PI2
    } else {
        // Cache hit - use stored values
        whiteIsolated = pawnEntry->isolatedPawns[WHITE];
        blackIsolated = pawnEntry->isolatedPawns[BLACK];
        whiteDoubled = pawnEntry->doubledPawns[WHITE];
        blackDoubled = pawnEntry->doubledPawns[BLACK];
        whitePassedPawns = pawnEntry->passedPawns[WHITE];
        blackPassedPawns = pawnEntry->passedPawns[BLACK];
        whiteBackward = pawnEntry->backwardPawns[WHITE];  // BP2
        blackBackward = pawnEntry->backwardPawns[BLACK];  // BP2
        whiteIslands = pawnEntry->pawnIslands[WHITE];  // PI2
        blackIslands = pawnEntry->pawnIslands[BLACK];  // PI2
    }
    
    // Calculate passed pawn score
    int passedPawnValue = 0;
    
    // PPH2: Use cached white passed pawns instead of per-pawn detection
    Bitboard whitePassers = whitePassedPawns;  // Use cached bitboard
    while (whitePassers) {
        Square sq = popLsb(whitePassers);
        // No need to check isPassed - we know it's passed from cache
        {
            int relRank = PawnStructure::relativeRank(WHITE, sq);
            int bonus = PASSED_PAWN_BONUS[relRank];
            
            // PP3a: Protected passer bonus ONLY
            // Check if friendly pawns can protect this square (pawns diagonally behind)
            Bitboard protectingSquares = 0ULL;
            if (rankOf(sq) > 0) {  // Can't be protected if on rank 1
                if (fileOf(sq) > 0) protectingSquares |= (1ULL << (sq - 9));  // Southwest
                if (fileOf(sq) < 7) protectingSquares |= (1ULL << (sq - 7));  // Southeast
            }
            Bitboard pawnSupport = protectingSquares & whitePawns;
            if (pawnSupport) {
                bonus = (bonus * 12) / 10;  // +20% for protected (conservative)
            }
            
            // PP3c: Blockader evaluation - penalty if enemy piece blocks the pawn
            Square blockSquare = Square(sq + 8);  // Square in front of pawn
            if (blockSquare <= SQ_H8) {
                Piece blocker = board.pieceAt(blockSquare);
                if (blocker != NO_PIECE && colorOf(blocker) == BLACK) {
                    // Apply penalty based on piece type blocking
                    // Knights are best blockers, bishops worst
                    int blockPenalty = 0;
                    switch (typeOf(blocker)) {
                        case KNIGHT: blockPenalty = bonus / 8; break;   // -12.5% (knights are good blockers)
                        case BISHOP: blockPenalty = bonus / 4; break;   // -25% (bishops are poor blockers)
                        case ROOK:   blockPenalty = bonus / 6; break;   // -16.7%
                        case QUEEN:  blockPenalty = bonus / 5; break;   // -20%
                        case KING:   blockPenalty = bonus / 6; break;   // -16.7%
                        default: break;
                    }
                    bonus -= blockPenalty;
                }
            }
            
            // PP3e: King proximity in endgame only (WHITE)
            // In endgame, passed pawns are stronger if friendly king is close
            // and weaker if enemy king is close  
            if (gamePhase == search::GamePhase::ENDGAME) {
                // Calculate king distances (Manhattan distance)
                // Using cached king squares from Phase 2.5.b
                Square whiteKing = whiteKingSquare;
                Square blackKing = blackKingSquare;
                
                int friendlyKingDist = std::abs(rankOf(sq) - rankOf(whiteKing)) + 
                                       std::abs(fileOf(sq) - fileOf(whiteKing));
                int enemyKingDist = std::abs(rankOf(sq) - rankOf(blackKing)) + 
                                    std::abs(fileOf(sq) - fileOf(blackKing));
                
                // Bonus for friendly king close (max 8 distance on board)
                // Closer king = bigger bonus
                int kingProximityBonus = (8 - friendlyKingDist) * 2;  // 0-16 cp
                
                // Penalty for enemy king close
                int kingProximityPenalty = (8 - enemyKingDist) * 3;  // 0-24 cp (enemy king more important)
                
                bonus += kingProximityBonus;
                bonus -= kingProximityPenalty;
                
                // PP3f: Unstoppable passer detection (WHITE)
                // Check if the pawn is unstoppable in pure king and pawn endgame
                // Rule: The pawn can promote if the enemy king cannot catch it
                // This uses the "square rule" - if enemy king is outside the square, pawn promotes
                if (relRank >= 4) {  // Only check for advanced pawns (rank 5+)
                    // Using cached isPureEndgame from Phase 2.5.b
                    if (isPureEndgame) {
                        // Calculate if enemy king can catch the pawn using square rule
                        // Distance to promotion square
                        int pawnDistToPromotion = 7 - rankOf(sq);
                        
                        // Enemy king distance to promotion square
                        Square promotionSquare = Square(fileOf(sq) + 56);  // Rank 8, same file
                        int kingDistToPromotion = std::max(std::abs(rankOf(blackKing) - rankOf(promotionSquare)),
                                                          std::abs(fileOf(blackKing) - fileOf(promotionSquare)));
                        
                        // Account for who moves first (if it's white's turn, pawn gets head start)
                        int moveAdvantage = (sideToMove == WHITE) ? 1 : 0;
                        
                        // If king can't catch the pawn, it's unstoppable
                        if (kingDistToPromotion > pawnDistToPromotion + moveAdvantage) {
                            // Huge bonus for unstoppable pawn (equivalent to a minor piece)
                            bonus += 300;
                        }
                    }
                }
            }
            
            // PP3b: Connected passer bonus - passed pawns on adjacent files
            // More conservative approach: small fixed bonus, stricter rank requirement
            int file = fileOf(sq);
            Bitboard adjacentFiles = 0ULL;
            if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
            if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
            
            // PPH2: Check if there's another white passed pawn on adjacent files using cache
            // IMPORTANT: Exclude current pawn from the check
            Bitboard otherPassedPawns = whitePassedPawns & ~(1ULL << sq);
            Bitboard adjacentPassedPawns = otherPassedPawns & adjacentFiles;  // Use cached passed pawns
            bool hasConnectedPasser = false;
            while (adjacentPassedPawns && !hasConnectedPasser) {
                Square adjSq = popLsb(adjacentPassedPawns);
                // We know it's passed since it's from the cached passed pawns bitboard
                {
                    // Only count as connected if on same or adjacent ranks (strict)
                    int rankDiff = std::abs(rankOf(sq) - rankOf(adjSq));
                    if (rankDiff <= 1) {
                        hasConnectedPasser = true;
                        // Upper range bonus: +20% (max per expert analysis)
                        // And only if this pawn is more advanced (to avoid double counting)
                        if (rankOf(sq) > rankOf(adjSq)) {  // Strict > to avoid double counting on same rank
                            bonus = (bonus * 12) / 10;  // +20% for connected passers
                        }
                    }
                }
            }
            
            passedPawnValue += bonus;
        }
    }
    
    // PPH2: Use cached black passed pawns instead of per-pawn detection
    Bitboard blackPassers = blackPassedPawns;  // Use cached bitboard
    while (blackPassers) {
        Square sq = popLsb(blackPassers);
        // No need to check isPassed - we know it's passed from cache
        {
            int relRank = PawnStructure::relativeRank(BLACK, sq);
            int bonus = PASSED_PAWN_BONUS[relRank];
            
            // PP3a: Protected passer bonus ONLY
            // Check if friendly pawns can protect this square (pawns diagonally behind)
            Bitboard protectingSquares = 0ULL;
            if (rankOf(sq) < 7) {  // Can't be protected if on rank 8
                if (fileOf(sq) > 0) protectingSquares |= (1ULL << (sq + 7));  // Northwest
                if (fileOf(sq) < 7) protectingSquares |= (1ULL << (sq + 9));  // Northeast
            }
            Bitboard pawnSupport = protectingSquares & blackPawns;
            if (pawnSupport) {
                bonus = (bonus * 12) / 10;  // +20% for protected (conservative)
            }
            
            // PP3c: Blockader evaluation - penalty if enemy piece blocks the pawn
            Square blockSquare = Square(sq - 8);  // Square in front of pawn (for black)
            if (blockSquare >= SQ_A1) {
                Piece blocker = board.pieceAt(blockSquare);
                if (blocker != NO_PIECE && colorOf(blocker) == WHITE) {
                    // Apply penalty based on piece type blocking
                    // Knights are best blockers, bishops worst
                    int blockPenalty = 0;
                    switch (typeOf(blocker)) {
                        case KNIGHT: blockPenalty = bonus / 8; break;   // -12.5% (knights are good blockers)
                        case BISHOP: blockPenalty = bonus / 4; break;   // -25% (bishops are poor blockers)
                        case ROOK:   blockPenalty = bonus / 6; break;   // -16.7%
                        case QUEEN:  blockPenalty = bonus / 5; break;   // -20%
                        case KING:   blockPenalty = bonus / 6; break;   // -16.7%
                        default: break;
                    }
                    bonus -= blockPenalty;
                }
            }
            
            // PP3e: King proximity in endgame only (BLACK)
            // In endgame, passed pawns are stronger if friendly king is close
            // and weaker if enemy king is close
            if (gamePhase == search::GamePhase::ENDGAME) {
                // Calculate king distances (Manhattan distance)
                // Using cached king squares from Phase 2.5.b
                Square whiteKing = whiteKingSquare;
                Square blackKing = blackKingSquare;
                
                int friendlyKingDist = std::abs(rankOf(sq) - rankOf(blackKing)) + 
                                       std::abs(fileOf(sq) - fileOf(blackKing));
                int enemyKingDist = std::abs(rankOf(sq) - rankOf(whiteKing)) + 
                                    std::abs(fileOf(sq) - fileOf(whiteKing));
                
                // Bonus for friendly king close (max 8 distance on board)
                // Closer king = bigger bonus
                int kingProximityBonus = (8 - friendlyKingDist) * 2;  // 0-16 cp
                
                // Penalty for enemy king close
                int kingProximityPenalty = (8 - enemyKingDist) * 3;  // 0-24 cp (enemy king more important)
                
                bonus += kingProximityBonus;
                bonus -= kingProximityPenalty;
                
                // PP3f: Unstoppable passer detection (BLACK)
                // Check if the pawn is unstoppable in pure king and pawn endgame
                // Rule: The pawn can promote if the enemy king cannot catch it
                // This uses the "square rule" - if enemy king is outside the square, pawn promotes
                if (relRank >= 4) {  // Only check for advanced pawns (rank 5+ relative)
                    // Using cached isPureEndgame from Phase 2.5.b
                    if (isPureEndgame) {
                        // Calculate if enemy king can catch the pawn using square rule
                        // Distance to promotion square (for black, promotion is rank 1)
                        int pawnDistToPromotion = rankOf(sq);  // Black promotes at rank 0
                        
                        // Enemy king distance to promotion square
                        Square promotionSquare = Square(fileOf(sq));  // Rank 1, same file
                        int kingDistToPromotion = std::max(std::abs(rankOf(whiteKing) - rankOf(promotionSquare)),
                                                          std::abs(fileOf(whiteKing) - fileOf(promotionSquare)));
                        
                        // Account for who moves first (if it's black's turn, pawn gets head start)
                        int moveAdvantage = (sideToMove == BLACK) ? 1 : 0;
                        
                        // If king can't catch the pawn, it's unstoppable
                        if (kingDistToPromotion > pawnDistToPromotion + moveAdvantage) {
                            // Huge bonus for unstoppable pawn (equivalent to a minor piece)
                            bonus += 300;
                        }
                    }
                }
            }
            
            // PP3b: Connected passer bonus - passed pawns on adjacent files
            // More conservative approach: small fixed bonus, stricter rank requirement
            int file = fileOf(sq);
            Bitboard adjacentFiles = 0ULL;
            if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
            if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
            
            // PPH2: Check if there's another black passed pawn on adjacent files using cache
            // IMPORTANT: Exclude current pawn from the check  
            Bitboard otherPassedPawns = blackPassedPawns & ~(1ULL << sq);
            Bitboard adjacentPassedPawns = otherPassedPawns & adjacentFiles;  // Use cached passed pawns
            bool hasConnectedPasser = false;
            while (adjacentPassedPawns && !hasConnectedPasser) {
                Square adjSq = popLsb(adjacentPassedPawns);
                // We know it's passed since it's from the cached passed pawns bitboard
                {
                    // Only count as connected if on same or adjacent ranks (strict)
                    int rankDiff = std::abs(rankOf(sq) - rankOf(adjSq));
                    if (rankDiff <= 1) {
                        hasConnectedPasser = true;
                        // Upper range bonus: +20% (max per expert analysis)
                        // And only if this pawn is more advanced (to avoid double counting)
                        // For black, "more advanced" means lower rank number
                        if (rankOf(sq) < rankOf(adjSq)) {  // Strict < to avoid double counting on same rank
                            bonus = (bonus * 12) / 10;  // +20% for connected passers
                        }
                    }
                }
            }
            
            passedPawnValue -= bonus;
        }
    }
    
    // Phase scaling: passed pawns are more valuable in endgame
    // Using cached gamePhase from Phase 2.5.b
    
    // Scale passed pawn bonus based on phase
    // Opening: 50% value, Middlegame: 75% value, Endgame: 150% value
    switch (gamePhase) {
        case search::GamePhase::OPENING:
            passedPawnValue = passedPawnValue / 2;  // 50%
            break;
        case search::GamePhase::MIDDLEGAME:
            passedPawnValue = (passedPawnValue * 3) / 4;  // 75%
            break;
        case search::GamePhase::ENDGAME:
            passedPawnValue = (passedPawnValue * 3) / 2;  // 150%
            break;
    }
    
    // Convert passed pawn value to Score
    Score passedPawnScore(passedPawnValue);
    
    // Phase IP2: Isolated pawn evaluation
    // Base penalties by rank (white perspective) - conservative values
    static constexpr int ISOLATED_PAWN_PENALTY[8] = {
        0,   // Rank 1 - no pawns here
        15,  // Rank 2 - most vulnerable
        14,  // Rank 3
        12,  // Rank 4 - standard
        12,  // Rank 5
        10,  // Rank 6
        8,   // Rank 7 - less weak when advanced
        0    // Rank 8 - promoted
    };
    
    // Phase IP3a: File-based adjustments (percentage multipliers)
    static constexpr int FILE_ADJUSTMENT[8] = {
        120,  // a-file (edge pawn penalty)
        105,  // b-file
        100,  // c-file (standard)
        80,   // d-file (central bonus - controls center)
        80,   // e-file (central bonus - controls center)
        100,  // f-file (standard)
        105,  // g-file
        120   // h-file (edge pawn penalty)
    };
    
    // Calculate isolated pawn penalties (using cached values computed earlier)
    int isolatedPawnPenalty = 0;
    
    // Evaluate white isolated pawns (penalty)
    Bitboard whiteIsolani = whiteIsolated;
    while (whiteIsolani) {
        Square sq = popLsb(whiteIsolani);
        int rank = rankOf(sq);
        int file = fileOf(sq);
        int penalty = ISOLATED_PAWN_PENALTY[rank];
        
        // IP3a: Apply file adjustment
        penalty = (penalty * FILE_ADJUSTMENT[file]) / 100;
        
        isolatedPawnPenalty -= penalty;
    }
    
    // Evaluate black isolated pawns (bonus for white)
    Bitboard blackIsolani = blackIsolated;
    while (blackIsolani) {
        Square sq = popLsb(blackIsolani);
        int rank = 7 - rankOf(sq);  // Convert to black's perspective
        int file = fileOf(sq);
        int penalty = ISOLATED_PAWN_PENALTY[rank];
        
        // IP3a: Apply file adjustment
        penalty = (penalty * FILE_ADJUSTMENT[file]) / 100;
        
        isolatedPawnPenalty += penalty;
    }
    
    // Phase scaling for isolated pawns - less penalty in endgame
    switch (gamePhase) {
        case search::GamePhase::OPENING:
        case search::GamePhase::MIDDLEGAME:
            // Full penalty in opening and middlegame
            break;
        case search::GamePhase::ENDGAME:
            isolatedPawnPenalty = isolatedPawnPenalty / 2;  // 50% penalty in endgame
            break;
    }
    
    // Convert isolated pawn penalty to Score
    Score isolatedPawnScore(isolatedPawnPenalty);
    
    // DP3: Doubled pawn penalties enabled
    // Reduced penalties after testing showed -15/-6 was too harsh
    static constexpr int DOUBLED_PAWN_PENALTY_MG = -8;   // Middlegame penalty (reduced from -15)
    static constexpr int DOUBLED_PAWN_PENALTY_EG = -3;   // Endgame penalty (reduced from -6)
    
    // Calculate doubled pawn penalties
    int doubledPawnPenalty = 0;
    
    // Count doubled pawns (now using cached values from above)
    int whiteDoubledCount = popCount(whiteDoubled);
    int blackDoubledCount = popCount(blackDoubled);
    
    // Apply penalties - positive values since we subtract black's penalty
    int penalty = (gamePhase == search::GamePhase::ENDGAME) ? 
                  std::abs(DOUBLED_PAWN_PENALTY_EG) : std::abs(DOUBLED_PAWN_PENALTY_MG);
    
    // White is penalized for its doubled pawns, black's doubled pawns help white
    doubledPawnPenalty = (blackDoubledCount - whiteDoubledCount) * penalty;
    
    // Convert doubled pawn penalty to Score
    Score doubledPawnScore(doubledPawnPenalty);
    
    // PI2: Pawn islands evaluation - fewer islands is better
    // Conservative penalties to start (will tune in PI3)
    static constexpr int PAWN_ISLAND_PENALTY = 5;  // Per island beyond the first
    
    // Calculate penalty (having 1 island is ideal, penalize additional islands)
    int whiteIslandPenalty = (whiteIslands > 1) ? (whiteIslands - 1) * PAWN_ISLAND_PENALTY : 0;
    int blackIslandPenalty = (blackIslands > 1) ? (blackIslands - 1) * PAWN_ISLAND_PENALTY : 0;
    
    // From white's perspective: penalize white's islands, reward black's islands
    int pawnIslandValue = blackIslandPenalty - whiteIslandPenalty;
    Score pawnIslandScore(pawnIslandValue);
    
    // BP3: Backward pawn evaluation enabled
    // Reduced penalty after initial test showed 18cp was too high
    static constexpr int BACKWARD_PAWN_PENALTY = 8;  // Centipawns penalty per backward pawn
    int whiteBackwardCount = popCount(whiteBackward);
    int blackBackwardCount = popCount(blackBackward);
    
    // From white's perspective: penalize white's backward pawns, reward black's backward pawns
    int backwardPawnValue = (blackBackwardCount - whiteBackwardCount) * BACKWARD_PAWN_PENALTY;
    Score backwardPawnScore(backwardPawnValue);
    
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
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
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
    
    // Phase KS2: King safety evaluation (integrated but returns 0 score)
    // Evaluate for both sides and combine
    Score whiteKingSafety = KingSafety::evaluate(board, WHITE);
    Score blackKingSafety = KingSafety::evaluate(board, BLACK);
    
    // King safety is from each side's perspective, so we subtract black's from white's
    // Note: In Phase KS2, both will return 0 since enableScoring = 0
    Score kingSafetyScore = whiteKingSafety - blackKingSafety;
    
    // Phase ROF2: Calculate rook file bonus score
    Score rookFileScore = Score(whiteRookFileBonus - blackRookFileBonus);

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
    
    // Calculate total evaluation from white's perspective
    // Material difference + PST score + passed pawn score + isolated pawn score + doubled pawn score + island score + backward score + bishop pair + mobility + king safety + rook files + knight outposts
    Score materialDiff = material.value(WHITE) - material.value(BLACK);
    Score totalWhite = materialDiff + pstValue + passedPawnScore + isolatedPawnScore + doubledPawnScore + pawnIslandScore + backwardPawnScore + bishopPairScore + mobilityScore + kingSafetyScore + rookFileScore + rookKingProximityScore + knightOutpostScore;
    
    // Return from side-to-move perspective
    if (sideToMove == WHITE) {
        return totalWhite;
    } else {
        return -totalWhite;
    }
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

// Phase 3: Detailed evaluation breakdown for UCI eval command
EvalBreakdown evaluateDetailed(const Board& board) {
    EvalBreakdown breakdown;
    
    // Cache side to move for Phase 2.5.b optimization
    Color sideToMove = board.sideToMove();
    
    // Get material from board
    const Material& material = board.material();
    
    // Check for insufficient material draws
    if (material.isInsufficientMaterial()) {
        breakdown.material = Score::draw();
        breakdown.total = Score::draw();
        return breakdown;
    }
    
    // Check for same-colored bishops (KB vs KB)
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
        
        Bitboard whiteBishops = board.pieces(WHITE, BISHOP);
        Bitboard blackBishops = board.pieces(BLACK, BISHOP);
        
        if (whiteBishops && blackBishops) {
            Square wbSq = lsb(whiteBishops);
            Square bbSq = lsb(blackBishops);
            
            bool wbDark = ((wbSq / 8) + (wbSq % 8)) % 2 == 0;
            bool bbDark = ((bbSq / 8) + (bbSq % 8)) % 2 == 0;
            
            if (wbDark == bbDark) {
                breakdown.material = Score::draw();
                breakdown.total = Score::draw();
                return breakdown;
            }
        }
    }
    
    // Get PST score
    const MgEgScore& pstScore = board.pstScore();
    if (seajay::getConfig().usePSTInterpolation) {
        int phase = phase0to256(board);
        int invPhase = 256 - phase;
        int blendedPst = (pstScore.mg.value() * phase + pstScore.eg.value() * invPhase + 128) >> 8;
        breakdown.pst = Score(blendedPst);
    } else {
        breakdown.pst = pstScore.mg;
    }
    
    // Passed pawn evaluation
    static constexpr int PASSED_PAWN_BONUS[8] = {
        0, 10, 17, 30, 60, 120, 180, 0
    };
    
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPawns = board.pieces(BLACK, PAWN);
    
    uint64_t pawnKey = board.pawnZobristKey();
    PawnEntry* pawnEntry = g_pawnStructure.probe(pawnKey);
    
    Bitboard whiteIsolated, blackIsolated, whiteDoubled, blackDoubled;
    Bitboard whitePassedPawns, blackPassedPawns;
    Bitboard whiteBackward, blackBackward;
    uint8_t whiteIslands, blackIslands;
    
    if (!pawnEntry) {
        PawnEntry newEntry;
        newEntry.key = pawnKey;
        newEntry.valid = true;
        
        newEntry.isolatedPawns[WHITE] = g_pawnStructure.getIsolatedPawns(WHITE, whitePawns);
        newEntry.isolatedPawns[BLACK] = g_pawnStructure.getIsolatedPawns(BLACK, blackPawns);
        newEntry.doubledPawns[WHITE] = g_pawnStructure.getDoubledPawns(WHITE, whitePawns);
        newEntry.doubledPawns[BLACK] = g_pawnStructure.getDoubledPawns(BLACK, blackPawns);
        newEntry.passedPawns[WHITE] = g_pawnStructure.getPassedPawns(WHITE, whitePawns, blackPawns);
        newEntry.passedPawns[BLACK] = g_pawnStructure.getPassedPawns(BLACK, blackPawns, whitePawns);
        newEntry.backwardPawns[WHITE] = g_pawnStructure.getBackwardPawns(WHITE, whitePawns, blackPawns);
        newEntry.backwardPawns[BLACK] = g_pawnStructure.getBackwardPawns(BLACK, blackPawns, whitePawns);
        newEntry.pawnIslands[WHITE] = PawnStructure::countPawnIslands(whitePawns);
        newEntry.pawnIslands[BLACK] = PawnStructure::countPawnIslands(blackPawns);
        
        g_pawnStructure.store(pawnKey, newEntry);
        
        whiteIsolated = newEntry.isolatedPawns[WHITE];
        blackIsolated = newEntry.isolatedPawns[BLACK];
        whiteDoubled = newEntry.doubledPawns[WHITE];
        blackDoubled = newEntry.doubledPawns[BLACK];
        whitePassedPawns = newEntry.passedPawns[WHITE];
        blackPassedPawns = newEntry.passedPawns[BLACK];
        whiteBackward = newEntry.backwardPawns[WHITE];
        blackBackward = newEntry.backwardPawns[BLACK];
        whiteIslands = newEntry.pawnIslands[WHITE];
        blackIslands = newEntry.pawnIslands[BLACK];
    } else {
        whiteIsolated = pawnEntry->isolatedPawns[WHITE];
        blackIsolated = pawnEntry->isolatedPawns[BLACK];
        whiteDoubled = pawnEntry->doubledPawns[WHITE];
        blackDoubled = pawnEntry->doubledPawns[BLACK];
        whitePassedPawns = pawnEntry->passedPawns[WHITE];
        blackPassedPawns = pawnEntry->passedPawns[BLACK];
        whiteBackward = pawnEntry->backwardPawns[WHITE];
        blackBackward = pawnEntry->backwardPawns[BLACK];
        whiteIslands = pawnEntry->pawnIslands[WHITE];
        blackIslands = pawnEntry->pawnIslands[BLACK];
    }
    
    // For simplicity, we'll compute the main evaluation components
    // This is a simplified version - the actual evaluate() has much more detail
    
    // Material difference
    breakdown.material = material.value(WHITE) - material.value(BLACK);
    
    // Passed pawns (simplified - just count them for now)
    int whitePassedCount = popCount(whitePassedPawns);
    int blackPassedCount = popCount(blackPassedPawns);
    breakdown.passedPawns = Score((whitePassedCount - blackPassedCount) * 50);
    
    // Isolated pawns
    int whiteIsolatedCount = popCount(whiteIsolated);
    int blackIsolatedCount = popCount(blackIsolated);
    breakdown.isolatedPawns = Score((blackIsolatedCount - whiteIsolatedCount) * 15);
    
    // Doubled pawns
    int whiteDoubledCount = popCount(whiteDoubled);
    int blackDoubledCount = popCount(blackDoubled);
    breakdown.doubledPawns = Score((blackDoubledCount - whiteDoubledCount) * 10);
    
    // Backward pawns
    int whiteBackwardCount = popCount(whiteBackward);
    int blackBackwardCount = popCount(blackBackward);
    breakdown.backwardPawns = Score((blackBackwardCount - whiteBackwardCount) * 8);
    
    // Pawn islands
    breakdown.pawnIslands = Score((blackIslands - whiteIslands) * 5);
    
    // Bishop pair
    bool whiteBishopPair = (material.count(WHITE, BISHOP) >= 2);
    bool blackBishopPair = (material.count(BLACK, BISHOP) >= 2);
    breakdown.bishopPair = Score((whiteBishopPair ? 50 : 0) - (blackBishopPair ? 50 : 0));
    
    // Simplified mobility placeholder (not calculated here)
    breakdown.mobility = Score(0);
    // King safety: compute using same function as main evaluate()
    {
        Score whiteKingSafety = KingSafety::evaluate(board, WHITE);
        Score blackKingSafety = KingSafety::evaluate(board, BLACK);
        breakdown.kingSafety = whiteKingSafety - blackKingSafety;
    }
    breakdown.rookFiles = Score(0);
    breakdown.knightOutposts = Score(0);
    
    // Calculate total
    breakdown.total = breakdown.material + breakdown.pst + breakdown.passedPawns + 
                     breakdown.isolatedPawns + breakdown.doubledPawns + breakdown.backwardPawns +
                     breakdown.pawnIslands + breakdown.bishopPair + breakdown.mobility + 
                     breakdown.kingSafety + breakdown.rookFiles + breakdown.knightOutposts;
    
    // Return from side-to-move perspective
    if (sideToMove == BLACK) {
        breakdown.material = -breakdown.material;
        breakdown.pst = -breakdown.pst;
        breakdown.passedPawns = -breakdown.passedPawns;
        breakdown.isolatedPawns = -breakdown.isolatedPawns;
        breakdown.doubledPawns = -breakdown.doubledPawns;
        breakdown.backwardPawns = -breakdown.backwardPawns;
        breakdown.pawnIslands = -breakdown.pawnIslands;
        breakdown.bishopPair = -breakdown.bishopPair;
        breakdown.mobility = -breakdown.mobility;
        breakdown.kingSafety = -breakdown.kingSafety;
        breakdown.rookFiles = -breakdown.rookFiles;
        breakdown.knightOutposts = -breakdown.knightOutposts;
        breakdown.total = -breakdown.total;
    }
    
    return breakdown;
}

} // namespace seajay::eval
