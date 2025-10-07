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
    int16_t bonus = static_cast<int16_t>(std::min(depth * depth * 2, 800));
    
    // Update the history score with saturating arithmetic
    // This eliminates the need for global aging (critical for thread safety)
    int16_t& entry = m_history[from][to][side];
    int32_t newValue = static_cast<int32_t>(entry) + bonus;
    
    // Saturate at HISTORY_MAX to prevent overflow
    // No aging needed - much better for concurrent access
    if (newValue > HISTORY_MAX) {
        entry = HISTORY_MAX;
    } else {
        entry = static_cast<int16_t>(newValue);
    }
}

void HistoryHeuristic::updateFailed(Color side, Square from, Square to, int depth) {
    // Validate inputs
    if (side >= NUM_COLORS || from >= 64 || to >= 64) {
        return;  // Invalid parameters
    }
    
    // Calculate penalty based on depth squared
    // Penalty is proportionally smaller than bonus to avoid over-penalizing
    int16_t penalty = static_cast<int16_t>(std::min(depth * depth, 400));  // Half the bonus amount
    
    // Reduce the history score with saturating arithmetic
    int16_t& entry = m_history[from][to][side];
    int32_t newValue = static_cast<int32_t>(entry) - penalty;
    
    // Saturate at -HISTORY_MAX to prevent underflow
    // No aging needed - much better for concurrent access
    if (newValue < -HISTORY_MAX) {
        entry = -HISTORY_MAX;
    } else {
        entry = static_cast<int16_t>(newValue);
    }
}

void HistoryHeuristic::ageHistory() {
    // With saturating arithmetic, aging is rarely needed
    // Only called manually if needed for special circumstances
    // Divide all history values by 2 to prevent overflow
    // This maintains relative ordering while keeping values manageable
    for (int f = 0; f < 64; ++f) {
        for (int t = 0; t < 64; ++t) {
            for (int c = 0; c < 2; ++c) {
                m_history[f][t][c] /= 2;
            }
        }
    }
}

} // namespace seajay::search
