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
    
    // Calculate passed pawn score
    int passedPawnValue = 0;
    
    // Get pawn bitboards
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPawns = board.pieces(BLACK, PAWN);
    
    // Evaluate white passed pawns
    Bitboard whitePassers = whitePawns;
    while (whitePassers) {
        Square sq = popLsb(whitePassers);
        if (PawnStructure::isPassed(WHITE, sq, blackPawns)) {
            int relRank = PawnStructure::relativeRank(WHITE, sq);
            int bonus = PASSED_PAWN_BONUS[relRank];
            
            // PP3a: Protected passer bonus ONLY
            Bitboard pawnSupport = pawnAttacks(BLACK, sq) & whitePawns;  // Squares that protect this pawn
            if (pawnSupport) {
                bonus = (bonus * 12) / 10;  // +20% for protected (conservative)
            }
            
            // PP3b: Connected passer bonus - passed pawns on adjacent files
            // More conservative approach: small fixed bonus, stricter rank requirement
            int file = fileOf(sq);
            Bitboard adjacentFiles = 0ULL;
            if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
            if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
            
            // Check if there's another white passed pawn on adjacent files
            Bitboard adjacentPawns = whitePawns & adjacentFiles;
            bool hasConnectedPasser = false;
            while (adjacentPawns && !hasConnectedPasser) {
                Square adjSq = popLsb(adjacentPawns);
                if (PawnStructure::isPassed(WHITE, adjSq, blackPawns)) {
                    // Only count as connected if on same or adjacent ranks (strict)
                    int rankDiff = std::abs(rankOf(sq) - rankOf(adjSq));
                    if (rankDiff <= 1) {
                        hasConnectedPasser = true;
                        // Moderate bonus: +15% (sweet spot per expert analysis)
                        // And only if this pawn is more advanced (to avoid double counting)
                        if (rankOf(sq) >= rankOf(adjSq)) {
                            bonus = (bonus * 115) / 100;  // +15% for connected passers
                        }
                    }
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
            int bonus = PASSED_PAWN_BONUS[relRank];
            
            // PP3a: Protected passer bonus ONLY
            Bitboard pawnSupport = pawnAttacks(WHITE, sq) & blackPawns;  // Squares that protect this pawn
            if (pawnSupport) {
                bonus = (bonus * 12) / 10;  // +20% for protected (conservative)
            }
            
            // PP3b: Connected passer bonus - passed pawns on adjacent files
            // More conservative approach: small fixed bonus, stricter rank requirement
            int file = fileOf(sq);
            Bitboard adjacentFiles = 0ULL;
            if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
            if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
            
            // Check if there's another black passed pawn on adjacent files
            Bitboard adjacentPawns = blackPawns & adjacentFiles;
            bool hasConnectedPasser = false;
            while (adjacentPawns && !hasConnectedPasser) {
                Square adjSq = popLsb(adjacentPawns);
                if (PawnStructure::isPassed(BLACK, adjSq, whitePawns)) {
                    // Only count as connected if on same or adjacent ranks (strict)
                    int rankDiff = std::abs(rankOf(sq) - rankOf(adjSq));
                    if (rankDiff <= 1) {
                        hasConnectedPasser = true;
                        // Moderate bonus: +15% (sweet spot per expert analysis)
                        // And only if this pawn is more advanced (to avoid double counting)
                        // For black, "more advanced" means lower rank number
                        if (rankOf(sq) <= rankOf(adjSq)) {
                            bonus = (bonus * 115) / 100;  // +15% for connected passers
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