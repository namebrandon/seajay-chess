#include "evaluate.h"
#include "material.h"
#include "pst.h"  // Stage 9: Include PST header
#include "pawn_structure.h"  // Phase PP2: Passed pawn evaluation
#include "../core/board.h"
#include "../core/bitboard.h"
#include "../search/game_phase.h"  // Phase PP2: For phase scaling
#include <cstdlib>  // PP3b: For std::abs

namespace seajay::eval {

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
    
    // For Stage 9, we only use middlegame PST values (no tapering yet)
    Score pstValue = pstScore.mg;
    
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
    
    // PPH2: Get cached pawn structure evaluation early
    uint64_t pawnKey = board.pawnZobristKey();
    PawnEntry* pawnEntry = g_pawnStructure.probe(pawnKey);
    
    Bitboard whiteIsolated, blackIsolated, whiteDoubled, blackDoubled;
    Bitboard whitePassedPawns, blackPassedPawns;
    
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
        
        // Store in cache
        g_pawnStructure.store(pawnKey, newEntry);
        
        // Use computed values
        whiteIsolated = newEntry.isolatedPawns[WHITE];
        blackIsolated = newEntry.isolatedPawns[BLACK];
        whiteDoubled = newEntry.doubledPawns[WHITE];
        blackDoubled = newEntry.doubledPawns[BLACK];
        whitePassedPawns = newEntry.passedPawns[WHITE];
        blackPassedPawns = newEntry.passedPawns[BLACK];
    } else {
        // Cache hit - use stored values
        whiteIsolated = pawnEntry->isolatedPawns[WHITE];
        blackIsolated = pawnEntry->isolatedPawns[BLACK];
        whiteDoubled = pawnEntry->doubledPawns[WHITE];
        blackDoubled = pawnEntry->doubledPawns[BLACK];
        whitePassedPawns = pawnEntry->passedPawns[WHITE];
        blackPassedPawns = pawnEntry->passedPawns[BLACK];
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
            search::GamePhase currentPhase = search::detectGamePhase(board);
            if (currentPhase == search::GamePhase::ENDGAME) {
                // Calculate king distances (Manhattan distance)
                Square whiteKing = board.kingSquare(WHITE);
                Square blackKing = board.kingSquare(BLACK);
                
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
                    // Check if it's a pure pawn endgame (no pieces other than kings and pawns)
                    bool isPureEndgame = board.pieces(WHITE, KNIGHT) == 0 &&
                                        board.pieces(WHITE, BISHOP) == 0 &&
                                        board.pieces(WHITE, ROOK) == 0 &&
                                        board.pieces(WHITE, QUEEN) == 0 &&
                                        board.pieces(BLACK, KNIGHT) == 0 &&
                                        board.pieces(BLACK, BISHOP) == 0 &&
                                        board.pieces(BLACK, ROOK) == 0 &&
                                        board.pieces(BLACK, QUEEN) == 0;
                    
                    if (isPureEndgame) {
                        // Calculate if enemy king can catch the pawn using square rule
                        // Distance to promotion square
                        int pawnDistToPromotion = 7 - rankOf(sq);
                        
                        // Enemy king distance to promotion square
                        Square promotionSquare = Square(fileOf(sq) + 56);  // Rank 8, same file
                        int kingDistToPromotion = std::max(std::abs(rankOf(blackKing) - rankOf(promotionSquare)),
                                                          std::abs(fileOf(blackKing) - fileOf(promotionSquare)));
                        
                        // Account for who moves first (if it's white's turn, pawn gets head start)
                        int moveAdvantage = (board.sideToMove() == WHITE) ? 1 : 0;
                        
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
            Bitboard adjacentPassedPawns = whitePassedPawns & adjacentFiles;  // Use cached passed pawns
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
            search::GamePhase currentPhaseBlack = search::detectGamePhase(board);
            if (currentPhaseBlack == search::GamePhase::ENDGAME) {
                // Calculate king distances (Manhattan distance)
                Square whiteKing = board.kingSquare(WHITE);
                Square blackKing = board.kingSquare(BLACK);
                
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
                    // Check if it's a pure pawn endgame (no pieces other than kings and pawns)
                    bool isPureEndgame = board.pieces(WHITE, KNIGHT) == 0 &&
                                        board.pieces(WHITE, BISHOP) == 0 &&
                                        board.pieces(WHITE, ROOK) == 0 &&
                                        board.pieces(WHITE, QUEEN) == 0 &&
                                        board.pieces(BLACK, KNIGHT) == 0 &&
                                        board.pieces(BLACK, BISHOP) == 0 &&
                                        board.pieces(BLACK, ROOK) == 0 &&
                                        board.pieces(BLACK, QUEEN) == 0;
                    
                    if (isPureEndgame) {
                        // Calculate if enemy king can catch the pawn using square rule
                        // Distance to promotion square (for black, promotion is rank 1)
                        int pawnDistToPromotion = rankOf(sq);  // Black promotes at rank 0
                        
                        // Enemy king distance to promotion square
                        Square promotionSquare = Square(fileOf(sq));  // Rank 1, same file
                        int kingDistToPromotion = std::max(std::abs(rankOf(whiteKing) - rankOf(promotionSquare)),
                                                          std::abs(fileOf(whiteKing) - fileOf(promotionSquare)));
                        
                        // Account for who moves first (if it's black's turn, pawn gets head start)
                        int moveAdvantage = (board.sideToMove() == BLACK) ? 1 : 0;
                        
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
            Bitboard adjacentPassedPawns = blackPassedPawns & adjacentFiles;  // Use cached passed pawns
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
    // Detect game phase using existing system
    search::GamePhase phase = search::detectGamePhase(board);
    
    // Scale passed pawn bonus based on phase
    // Opening: 50% value, Middlegame: 75% value, Endgame: 150% value
    switch (phase) {
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
    switch (phase) {
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
    int penalty = (phase == search::GamePhase::ENDGAME) ? 
                  std::abs(DOUBLED_PAWN_PENALTY_EG) : std::abs(DOUBLED_PAWN_PENALTY_MG);
    
    // White is penalized for its doubled pawns, black's doubled pawns help white
    doubledPawnPenalty = (blackDoubledCount - whiteDoubledCount) * penalty;
    
    // Convert doubled pawn penalty to Score
    Score doubledPawnScore(doubledPawnPenalty);
    
    // Calculate total evaluation from white's perspective
    // Material difference + PST score + passed pawn score + isolated pawn score + doubled pawn score
    Score materialDiff = material.value(WHITE) - material.value(BLACK);
    Score totalWhite = materialDiff + pstValue + passedPawnScore + isolatedPawnScore + doubledPawnScore;
    
    // Return from side-to-move perspective
    if (board.sideToMove() == WHITE) {
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

} // namespace seajay::eval