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
    // Increased cap from 400 to 800 for stronger history impact
    int bonus = std::min(depth * depth * 2, 800);
    
    // Update the history score
    m_history[side][from][to] += bonus;
    
    // Check if aging is needed - only age when many values are high
    // This prevents premature aging that loses valuable move ordering info
    if (std::abs(m_history[side][from][to]) >= HISTORY_MAX) {
        // Count how many entries are near the max
        int highValueCount = 0;
        const int threshold = HISTORY_MAX * 3 / 4; // 75% of max
        
        for (int c = 0; c < 2; ++c) {
            for (int f = 0; f < 64; ++f) {
                for (int t = 0; t < 64; ++t) {
                    if (std::abs(m_history[c][f][t]) >= threshold) {
                        highValueCount++;
                    }
                }
            }
        }
        
        // Only age if >10% of entries are high (prevents premature aging)
        // 2 colors * 64 from * 64 to = 8192 total entries
        if (highValueCount > 819) { // 10% of 8192
            ageHistory();
        }
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
    // Penalty is proportionally smaller than bonus to avoid over-penalizing
    int penalty = std::min(depth * depth, 400);  // Half the bonus amount
    
    // Reduce the history score
    m_history[side][from][to] -= penalty;
    
    // Check if aging is needed (for negative values too)
    if (std::abs(m_history[side][from][to]) >= HISTORY_MAX) {
        // Count how many entries are near the max
        int highValueCount = 0;
        const int threshold = HISTORY_MAX * 3 / 4; // 75% of max
        
        for (int c = 0; c < 2; ++c) {
            for (int f = 0; f < 64; ++f) {
                for (int t = 0; t < 64; ++t) {
                    if (std::abs(m_history[c][f][t]) >= threshold) {
                        highValueCount++;
                    }
                }
            }
        }
        
        // Only age if >10% of entries are high (prevents premature aging)
        // 2 colors * 64 from * 64 to = 8192 total entries
        if (highValueCount > 819) { // 10% of 8192
            ageHistory();
        }
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