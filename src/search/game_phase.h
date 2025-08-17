#pragma once

#include "../core/board.h"
#include "../core/bitboard.h"

namespace seajay::search {

/**
 * @brief Game phases for stability adjustment
 * Stage 13 Remediation Phase 4
 */
enum class GamePhase {
    OPENING,
    MIDDLEGAME,
    ENDGAME
};

/**
 * @brief Detect game phase based on material count
 * Uses non-pawn material (NPM) to determine phase
 * 
 * @param board Current board position
 * @return Current game phase
 */
inline GamePhase detectGamePhase(const Board& board) {
    // Count non-pawn material for both sides
    // Queen = 9, Rook = 5, Bishop = 3, Knight = 3
    int npm = 0;
    
    // White pieces
    npm += seajay::popCount(board.pieces(WHITE, QUEEN)) * 9;
    npm += seajay::popCount(board.pieces(WHITE, ROOK)) * 5;
    npm += seajay::popCount(board.pieces(WHITE, BISHOP)) * 3;
    npm += seajay::popCount(board.pieces(WHITE, KNIGHT)) * 3;
    
    // Black pieces
    npm += seajay::popCount(board.pieces(BLACK, QUEEN)) * 9;
    npm += seajay::popCount(board.pieces(BLACK, ROOK)) * 5;
    npm += seajay::popCount(board.pieces(BLACK, BISHOP)) * 3;
    npm += seajay::popCount(board.pieces(BLACK, KNIGHT)) * 3;
    
    // Maximum npm = 2*(9+10+6+6) = 62
    // Thresholds based on percentage of max material
    if (npm > 50) return GamePhase::OPENING;      // >80% material
    if (npm > 25) return GamePhase::MIDDLEGAME;   // 40-80% material
    return GamePhase::ENDGAME;                    // <40% material
}

/**
 * @brief Get stability threshold adjusted for game phase
 * 
 * @param phase Current game phase
 * @param baseThreshold Base stability threshold from UCI
 * @param openingThreshold Specific threshold for opening (if configured)
 * @param middlegameThreshold Specific threshold for middlegame (if configured)
 * @param endgameThreshold Specific threshold for endgame (if configured)
 * @param usePhaseSpecific Whether to use phase-specific thresholds
 * @return Adjusted stability threshold
 */
inline int getPhaseStabilityThreshold(GamePhase phase, 
                                     int baseThreshold,
                                     int openingThreshold,
                                     int middlegameThreshold,
                                     int endgameThreshold,
                                     bool usePhaseSpecific) {
    if (!usePhaseSpecific) {
        // Use automatic adjustment from base
        switch(phase) {
            case GamePhase::OPENING:    
                return std::max(2, baseThreshold - 2);  // Typically 4
            case GamePhase::MIDDLEGAME: 
                return baseThreshold;                   // Typically 6
            case GamePhase::ENDGAME:    
                return baseThreshold + 2;               // Typically 8
        }
    } else {
        // Use specific configured values
        switch(phase) {
            case GamePhase::OPENING:    return openingThreshold;
            case GamePhase::MIDDLEGAME: return middlegameThreshold;
            case GamePhase::ENDGAME:    return endgameThreshold;
        }
    }
    
    return baseThreshold;  // Fallback
}

} // namespace seajay::search