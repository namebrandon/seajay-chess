#include "history_heuristic.h"
#include <cstring>  // For memset
#include <algorithm>  // For std::min
#include <cstdlib>  // For std::abs

namespace seajay::search {

void HistoryHeuristic::clear() {
    // Zero out the entire history table
    std::memset(m_history, 0, sizeof(m_history));
}

void HistoryHeuristic::update(Color side, Square from, Square to, int depth) {
    // Validate inputs
    if (side >= NUM_COLORS || from >= 64 || to >= 64) {
        return;  // Invalid parameters
    }
    
    // Calculate bonus based on depth squared
    // Capped at 400 to prevent rapid overflow (Ethereal-style)
    int bonus = std::min(depth * depth, 400);
    
    // Update the history score
    m_history[side][from][to] += bonus;
    
    // Check if aging is needed
    // Age if any value gets too large (positive or negative)
    if (std::abs(m_history[side][from][to]) >= HISTORY_MAX) {
        ageHistory();
    }
}

int HistoryHeuristic::getScore(Color side, Square from, Square to) const {
    // Validate inputs
    if (side >= NUM_COLORS || from >= 64 || to >= 64) {
        return 0;  // Invalid parameters
    }
    
    return m_history[side][from][to];
}

void HistoryHeuristic::updateFailed(Color side, Square from, Square to, int depth) {
    // Validate inputs
    if (side >= NUM_COLORS || from >= 64 || to >= 64) {
        return;  // Invalid parameters
    }
    
    // Calculate penalty based on depth squared
    // Use much smaller penalty than bonus to avoid over-penalizing moves
    // B4.3: Reduced from /2 to /4 to be less aggressive
    int penalty = std::min(depth * depth / 4, 100);  // Quarter the bonus amount
    
    // Reduce the history score
    m_history[side][from][to] -= penalty;
    
    // Check if aging is needed (for negative values too)
    if (std::abs(m_history[side][from][to]) >= HISTORY_MAX) {
        ageHistory();
    }
}

void HistoryHeuristic::ageHistory() {
    // Divide all history values by 2 to prevent overflow
    // This maintains relative ordering while keeping values manageable
    for (int c = 0; c < 2; ++c) {
        for (int f = 0; f < 64; ++f) {
            for (int t = 0; t < 64; ++t) {
                m_history[c][f][t] /= 2;
            }
        }
    }
}

} // namespace seajay::search