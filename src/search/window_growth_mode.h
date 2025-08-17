#pragma once

#include <string>

namespace seajay::search {

/**
 * @brief Window growth strategies for aspiration window re-searches
 * Stage 13 Remediation Phase 4
 */
enum class WindowGrowthMode {
    LINEAR,      // delta = delta * 1.33 (original Stockfish style)
    MODERATE,    // delta = delta * 1.5
    EXPONENTIAL, // delta = delta * (2 << min(failCount-1, 3))
    ADAPTIVE     // Switch based on position characteristics
};

/**
 * @brief Convert string to WindowGrowthMode
 */
inline WindowGrowthMode parseWindowGrowthMode(const std::string& mode) {
    if (mode == "linear") return WindowGrowthMode::LINEAR;
    if (mode == "moderate") return WindowGrowthMode::MODERATE;
    if (mode == "adaptive") return WindowGrowthMode::ADAPTIVE;
    return WindowGrowthMode::EXPONENTIAL;  // Default as requested
}

/**
 * @brief Convert WindowGrowthMode to string
 */
inline std::string windowGrowthModeToString(WindowGrowthMode mode) {
    switch (mode) {
        case WindowGrowthMode::LINEAR: return "linear";
        case WindowGrowthMode::MODERATE: return "moderate";
        case WindowGrowthMode::ADAPTIVE: return "adaptive";
        case WindowGrowthMode::EXPONENTIAL: 
        default: return "exponential";
    }
}

} // namespace seajay::search