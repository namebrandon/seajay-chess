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
    
    // Calculate passed pawn score
    int passedPawnValue = 0;
    
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
            int bonus = baseBonus;
            
            // Phase PP3: King proximity factors
            // Friendly king proximity - closer is better in endgame
            int friendlyKingDist = distance(whiteKingSq, sq);
            bonus += (7 - friendlyKingDist) * 2;  // 0-14 bonus
            
            // Enemy king proximity - further is better
            int enemyKingDist = distance(blackKingSq, sq);
            bonus += enemyKingDist * 3;  // 0-21 bonus
            
            // Phase PP3: Rook behind passed pawn
            // Check if white rook is behind the pawn (same file, lower rank)
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
            
            // Phase PP3: Blockader evaluation
            Square blockSq = Square(sq + 8);  // Square in front
            if (blockSq <= SQ_H8) {
                Piece blocker = board.pieceAt(blockSq);
                if (blocker != NO_PIECE && colorOf(blocker) == BLACK) {
                    // Penalty based on piece type blocking
                    switch (typeOf(blocker)) {
                        case KNIGHT: bonus -= 5; break;   // Knights are good blockers
                        case BISHOP: bonus -= 15; break;  // Bishops are poor blockers
                        case ROOK:   bonus -= 10; break;  // Rooks are mediocre blockers
                        case QUEEN:  bonus -= 10; break;  // Queens are mediocre blockers
                        default: break;
                    }
                }
            }
            
            // Phase PP3: Protected passed pawns
            Bitboard pawnSupport = pawnAttacks(WHITE, sq) & whitePawns;
            if (pawnSupport) {
                bonus = (bonus * 13) / 10;  // +30% for protected
            }
            
            // Phase PP3: Connected passed pawns
            // Check if adjacent file has another passed pawn
            int file = fileOf(sq);
            Bitboard adjacentFiles = 0;
            if (file > 0) adjacentFiles |= fileBB(file - 1);
            if (file < 7) adjacentFiles |= fileBB(file + 1);
            Bitboard adjacentPassers = adjacentFiles & whitePawns;
            bool hasConnectedPasser = false;
            while (adjacentPassers && !hasConnectedPasser) {
                Square adjSq = popLsb(adjacentPassers);
                if (PawnStructure::isPassed(WHITE, adjSq, blackPawns)) {
                    hasConnectedPasser = true;
                    bonus = (bonus * 15) / 10;  // +50% for connected
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
            int bonus = baseBonus;
            
            // Phase PP3: King proximity factors
            int friendlyKingDist = distance(blackKingSq, sq);
            bonus += (7 - friendlyKingDist) * 2;
            
            int enemyKingDist = distance(whiteKingSq, sq);
            bonus += enemyKingDist * 3;
            
            // Phase PP3: Rook behind passed pawn
            if (blackRooks) {
                int pawnFile = fileOf(sq);
                Bitboard fileRooks = blackRooks & fileBB(pawnFile);
                if (fileRooks) {
                    Square rookSq = lsb(fileRooks);
                    if (rankOf(rookSq) > rankOf(sq)) {
                        bonus += 20;  // Rook behind passer (black perspective)
                    }
                }
            }
            
            // Phase PP3: Blockader evaluation
            Square blockSq = Square(sq - 8);  // Square in front (black perspective)
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
            
            // Phase PP3: Protected passed pawns
            Bitboard pawnSupport = pawnAttacks(BLACK, sq) & blackPawns;
            if (pawnSupport) {
                bonus = (bonus * 13) / 10;  // +30% for protected
            }
            
            // Phase PP3: Connected passed pawns
            int file = fileOf(sq);
            Bitboard adjacentFiles = 0;
            if (file > 0) adjacentFiles |= fileBB(file - 1);
            if (file < 7) adjacentFiles |= fileBB(file + 1);
            Bitboard adjacentPassers = adjacentFiles & blackPawns;
            bool hasConnectedPasser = false;
            while (adjacentPassers && !hasConnectedPasser) {
                Square adjSq = popLsb(adjacentPassers);
                if (PawnStructure::isPassed(BLACK, adjSq, whitePawns)) {
                    hasConnectedPasser = true;
                    bonus = (bonus * 15) / 10;  // +50% for connected
                }
            }
            
            passedPawnValue -= bonus;
        }
    }
    
    // Phase PP3: Unstoppable passer detection (huge bonus in endgame)
    // Only check in endgame when no major pieces remain
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
                    int kingDistToPromotion = distance(blackKingSq, Square(fileOf(sq) + 56));  // Promotion square
                    
                    // Account for tempo (white to move gets 1 extra tempo)
                    int tempoBonus = (board.sideToMove() == WHITE) ? 1 : 0;
                    
                    if (kingDistToPromotion - tempoBonus > pawnDistToPromotion) {
                        // Unstoppable passer!
                        passedPawnValue += 500;  // Huge bonus
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
                if (relRank >= 5) {  // Only consider pawns on rank 6 or 7 (from black's perspective)
                    // Check if white king can catch the pawn
                    int pawnDistToPromotion = rankOf(sq);  // Distance to rank 0
                    int kingDistToPromotion = distance(whiteKingSq, Square(fileOf(sq)));  // Promotion square
                    
                    // Account for tempo
                    int tempoBonus = (board.sideToMove() == BLACK) ? 1 : 0;
                    
                    if (kingDistToPromotion - tempoBonus > pawnDistToPromotion) {
                        // Unstoppable passer!
                        passedPawnValue -= 500;  // Huge bonus for black
                    }
                }
            }
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