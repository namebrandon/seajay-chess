#include "evaluate.h"
#include "material.h"
#include "pst.h"  // Stage 9: Include PST header
#include "pawn_structure.h"  // Phase PP2: Passed pawn evaluation
#include "../core/board.h"
#include "../core/bitboard.h"
#include "../search/game_phase.h"  // Phase PP2: For phase scaling

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
    
    // Phase PP3 FIXED: Passed pawn evaluation with all features
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
    
    // Calculate passed pawn score
    int passedPawnValue = 0;
    
    // Detect game phase EARLY for proper feature application
    search::GamePhase phase = search::detectGamePhase(board);
    bool isEndgame = (phase == search::GamePhase::ENDGAME);
    
    // Get piece bitboards for PP3 features
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPawns = board.pieces(BLACK, PAWN);
    Bitboard whiteKing = board.pieces(WHITE, KING);
    Bitboard blackKing = board.pieces(BLACK, KING);
    Bitboard whiteRooks = board.pieces(WHITE, ROOK);
    Bitboard blackRooks = board.pieces(BLACK, ROOK);
    
    Square whiteKingSq = whiteKing ? lsb(whiteKing) : SQ_A1;
    Square blackKingSq = blackKing ? lsb(blackKing) : SQ_A1;
    
    // Evaluate white passed pawns
    Bitboard whitePassers = whitePawns;
    while (whitePassers) {
        Square sq = popLsb(whitePassers);
        if (PawnStructure::isPassed(WHITE, sq, blackPawns)) {
            int relRank = PawnStructure::relativeRank(WHITE, sq);
            int baseBonus = PASSED_PAWN_BONUS[relRank];
            
            // FIXED: Apply phase scaling to BASE bonus FIRST
            switch (phase) {
                case search::GamePhase::OPENING:
                    baseBonus = baseBonus / 2;  // 50%
                    break;
                case search::GamePhase::MIDDLEGAME:
                    baseBonus = (baseBonus * 3) / 4;  // 75%
                    break;
                case search::GamePhase::ENDGAME:
                    baseBonus = (baseBonus * 3) / 2;  // 150%
                    break;
            }
            
            int bonus = baseBonus;
            
            // FIXED: King proximity ONLY in endgame (reduced values)
            if (isEndgame) {
                int friendlyKingDist = distance(whiteKingSq, sq);
                bonus += (7 - friendlyKingDist);  // 0-7 bonus (was *2)
                
                int enemyKingDist = distance(blackKingSq, sq);
                bonus += enemyKingDist;  // 0-7 bonus (was *3)
            }
            
            // Rook behind passed pawn (not phase-dependent)
            if (whiteRooks) {
                int pawnFile = fileOf(sq);
                Bitboard fileRooks = whiteRooks & fileBB(pawnFile);
                if (fileRooks) {
                    Square rookSq = lsb(fileRooks);
                    if (rankOf(rookSq) < rankOf(sq)) {
                        bonus += 20;  // Rook behind passer
                    }
                }
            }
            
            // Blockader evaluation
            Square blockSq = Square(sq + 8);
            if (blockSq <= SQ_H8) {
                Piece blocker = board.pieceAt(blockSq);
                if (blocker != NO_PIECE && colorOf(blocker) == BLACK) {
                    switch (typeOf(blocker)) {
                        case KNIGHT: bonus -= 5; break;
                        case BISHOP: bonus -= 15; break;
                        case ROOK:   bonus -= 10; break;
                        case QUEEN:  bonus -= 10; break;
                        default: break;
                    }
                }
            }
            
            // Protected passed pawns
            Bitboard pawnSupport = pawnAttacks(WHITE, sq) & whitePawns;
            if (pawnSupport) {
                bonus = (bonus * 13) / 10;  // +30%
            }
            
            // Connected passed pawns (FIXED: check rank similarity)
            int file = fileOf(sq);
            int rank = rankOf(sq);
            Bitboard adjacentFiles = 0;
            if (file > 0) adjacentFiles |= fileBB(file - 1);
            if (file < 7) adjacentFiles |= fileBB(file + 1);
            Bitboard adjacentPassers = adjacentFiles & whitePawns;
            while (adjacentPassers) {
                Square adjSq = popLsb(adjacentPassers);
                // Must be passed AND on similar rank (within 1)
                if (PawnStructure::isPassed(WHITE, adjSq, blackPawns) &&
                    std::abs(rankOf(adjSq) - rank) <= 1) {
                    bonus = (bonus * 14) / 10;  // +40% (reduced from 50%)
                    break;
                }
            }
            
            passedPawnValue += bonus;
        }
    }
    
    // Evaluate black passed pawns
    Bitboard blackPassers = blackPawns;
    while (blackPassers) {
        Square sq = popLsb(blackPassers);
        if (PawnStructure::isPassed(BLACK, sq, whitePawns)) {
            int relRank = PawnStructure::relativeRank(BLACK, sq);
            int baseBonus = PASSED_PAWN_BONUS[relRank];
            
            // FIXED: Apply phase scaling to BASE bonus FIRST
            switch (phase) {
                case search::GamePhase::OPENING:
                    baseBonus = baseBonus / 2;
                    break;
                case search::GamePhase::MIDDLEGAME:
                    baseBonus = (baseBonus * 3) / 4;
                    break;
                case search::GamePhase::ENDGAME:
                    baseBonus = (baseBonus * 3) / 2;
                    break;
            }
            
            int bonus = baseBonus;
            
            // FIXED: King proximity ONLY in endgame
            if (isEndgame) {
                int friendlyKingDist = distance(blackKingSq, sq);
                bonus += (7 - friendlyKingDist);
                
                int enemyKingDist = distance(whiteKingSq, sq);
                bonus += enemyKingDist;
            }
            
            // Rook behind passed pawn
            if (blackRooks) {
                int pawnFile = fileOf(sq);
                Bitboard fileRooks = blackRooks & fileBB(pawnFile);
                if (fileRooks) {
                    Square rookSq = lsb(fileRooks);
                    if (rankOf(rookSq) > rankOf(sq)) {
                        bonus += 20;
                    }
                }
            }
            
            // Blockader evaluation
            Square blockSq = Square(sq - 8);
            if (blockSq >= SQ_A1) {
                Piece blocker = board.pieceAt(blockSq);
                if (blocker != NO_PIECE && colorOf(blocker) == WHITE) {
                    switch (typeOf(blocker)) {
                        case KNIGHT: bonus -= 5; break;
                        case BISHOP: bonus -= 15; break;
                        case ROOK:   bonus -= 10; break;
                        case QUEEN:  bonus -= 10; break;
                        default: break;
                    }
                }
            }
            
            // Protected passed pawns
            Bitboard pawnSupport = pawnAttacks(BLACK, sq) & blackPawns;
            if (pawnSupport) {
                bonus = (bonus * 13) / 10;  // +30%
            }
            
            // Connected passed pawns (FIXED)
            int file = fileOf(sq);
            int rank = rankOf(sq);
            Bitboard adjacentFiles = 0;
            if (file > 0) adjacentFiles |= fileBB(file - 1);
            if (file < 7) adjacentFiles |= fileBB(file + 1);
            Bitboard adjacentPassers = adjacentFiles & blackPawns;
            while (adjacentPassers) {
                Square adjSq = popLsb(adjacentPassers);
                if (PawnStructure::isPassed(BLACK, adjSq, whitePawns) &&
                    std::abs(rankOf(adjSq) - rank) <= 1) {
                    bonus = (bonus * 14) / 10;  // +40%
                    break;
                }
            }
            
            passedPawnValue -= bonus;
        }
    }
    
    // Unstoppable passer detection (huge bonus in endgame)
    // Only check in PURE endgame when no major pieces remain
    bool inPureEndgame = board.pieces(WHITE, QUEEN) == 0 && board.pieces(BLACK, QUEEN) == 0 &&
                         popCount(board.pieces(WHITE, ROOK)) <= 1 && popCount(board.pieces(BLACK, ROOK)) <= 1;
    
    if (inPureEndgame) {
        // Check white unstoppable passers
        Bitboard whitePassedPawns = whitePawns;
        while (whitePassedPawns) {
            Square sq = popLsb(whitePassedPawns);
            if (PawnStructure::isPassed(WHITE, sq, blackPawns)) {
                int relRank = PawnStructure::relativeRank(WHITE, sq);
                if (relRank >= 5) {  // Only consider pawns on rank 6 or 7
                    // Check if black king can catch the pawn
                    int pawnDistToPromotion = 7 - rankOf(sq);
                    int kingDistToPromotion = distance(blackKingSq, Square(fileOf(sq) + 56));
                    
                    // Account for tempo
                    int tempoBonus = (board.sideToMove() == WHITE) ? 1 : 0;
                    
                    if (kingDistToPromotion - tempoBonus > pawnDistToPromotion) {
                        passedPawnValue += 500;  // Unstoppable!
                    }
                }
            }
        }
        
        // Check black unstoppable passers
        Bitboard blackPassedPawns = blackPawns;
        while (blackPassedPawns) {
            Square sq = popLsb(blackPassedPawns);
            if (PawnStructure::isPassed(BLACK, sq, whitePawns)) {
                int relRank = PawnStructure::relativeRank(BLACK, sq);
                if (relRank >= 5) {
                    int pawnDistToPromotion = rankOf(sq);
                    int kingDistToPromotion = distance(whiteKingSq, Square(fileOf(sq)));
                    
                    int tempoBonus = (board.sideToMove() == BLACK) ? 1 : 0;
                    
                    if (kingDistToPromotion - tempoBonus > pawnDistToPromotion) {
                        passedPawnValue -= 500;
                    }
                }
            }
        }
    }
    
    // Convert passed pawn value to Score
    Score passedPawnScore(passedPawnValue);
    
    // Calculate total evaluation from white's perspective
    // Material difference + PST score + passed pawn score
    Score materialDiff = material.value(WHITE) - material.value(BLACK);
    Score totalWhite = materialDiff + pstValue + passedPawnScore;
    
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