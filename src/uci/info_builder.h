// info_builder.h
// Phase 5: Structured Info Building System
// Creates clean, modular UCI info string building

#pragma once

#include <string>
#include <sstream>
#include <vector>
#include "../core/types.h"
#include "../evaluation/types.h"

namespace seajay::uci {

// UCI Bounds for score reporting
enum class ScoreBound {
    EXACT,      // Normal score
    LOWER,      // Fail high (score >= beta) 
    UPPER       // Fail low (score <= alpha)
};

// InfoBuilder class for constructing UCI info messages
// Provides a clean, fluent interface for building info strings
class InfoBuilder {
public:
    InfoBuilder();
    
    // Clear the builder for reuse
    void clear();
    
    // Depth information
    InfoBuilder& appendDepth(int depth, int seldepth);
    
    // Score information with optional bounds
    InfoBuilder& appendScore(eval::Score score, ScoreBound bound = ScoreBound::EXACT);
    InfoBuilder& appendMateScore(int mateIn);
    InfoBuilder& appendCentipawnScore(int cp, ScoreBound bound = ScoreBound::EXACT);
    
    // Node and time information
    InfoBuilder& appendNodes(uint64_t nodes);
    InfoBuilder& appendTime(uint64_t milliseconds);
    InfoBuilder& appendNps(uint64_t nps);
    
    // Hash information
    InfoBuilder& appendHashfull(int permil);
    InfoBuilder& appendTbhits(uint64_t tbhits);
    
    // Current move information
    InfoBuilder& appendCurrmove(Move move, int moveNumber);
    InfoBuilder& appendCurrmove(const std::string& moveStr, int moveNumber);
    
    // Principal variation
    InfoBuilder& appendPv(Move move);
    InfoBuilder& appendPv(const std::string& moveStr);
    InfoBuilder& appendPv(const std::vector<Move>& moves);
    InfoBuilder& appendPv(const std::vector<std::string>& moves);
    
    // Multi-PV support (for future)
    InfoBuilder& appendMultiPv(int pvIndex);
    
    // String messages
    InfoBuilder& appendString(const std::string& message);
    
    // Custom key-value pairs for extensions
    InfoBuilder& appendCustom(const std::string& key, const std::string& value);
    InfoBuilder& appendCustom(const std::string& key, int value);
    InfoBuilder& appendCustom(const std::string& key, double value);
    
    // Build the final info string (includes "info" prefix and newline)
    std::string build() const;
    
    // Build without prefix/suffix (for custom formatting)
    std::string buildRaw() const;
    
    // Check if builder is empty
    bool isEmpty() const;
    
private:
    std::ostringstream m_stream;
    bool m_hasContent;
    
    // Helper to add space before new content
    void addSpace();
};

} // namespace seajay::uci