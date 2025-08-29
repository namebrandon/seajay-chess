// info_builder.cpp
// Phase 5: Structured Info Building System implementation

#include "info_builder.h"
#include "../core/board_safety.h"
#include <iomanip>

namespace seajay::uci {

InfoBuilder::InfoBuilder() : m_hasContent(false) {
}

void InfoBuilder::clear() {
    m_stream.str("");
    m_stream.clear();
    m_hasContent = false;
}

void InfoBuilder::addSpace() {
    if (m_hasContent) {
        m_stream << " ";
    }
    m_hasContent = true;
}

InfoBuilder& InfoBuilder::appendDepth(int depth, int seldepth) {
    addSpace();
    m_stream << "depth " << depth;
    
    addSpace();
    m_stream << "seldepth " << seldepth;
    
    return *this;
}

InfoBuilder& InfoBuilder::appendScore(eval::Score score, Color sideToMove, ScoreBound bound) {
    // UCI Protocol requires scores from the side-to-move perspective
    // Negamax already provides scores from side-to-move perspective
    // No conversion needed - just pass through the score
    if (score.is_mate_score()) {
        // Debug: Log the raw score to understand the issue
        if (false) { // Set to true for debugging
            std::cerr << "DEBUG: appendScore called with score=" << score.value() 
                     << " sideToMove=" << (sideToMove == WHITE ? "WHITE" : "BLACK") << std::endl;
        }
        
        int mateIn = 0;
        if (score > eval::Score::zero()) {
            mateIn = (eval::Score::mate().value() - score.value() + 1) / 2;
        } else {
            mateIn = -(eval::Score::mate().value() + score.value()) / 2;
        }
        return appendMateScore(mateIn, sideToMove);
    } else {
        // Score is already from side-to-move perspective (correct for UCI)
        // Do NOT negate for Black - UCI wants side-to-move perspective
        int cp = score.to_cp();
        return appendCentipawnScore(cp, bound);
    }
}

InfoBuilder& InfoBuilder::appendMateScore(int mateIn, Color sideToMove) {
    addSpace();
    // UCI requires mate scores from side-to-move perspective
    // mateIn is already from side-to-move perspective, no conversion needed
    m_stream << "score mate " << mateIn;
    return *this;
}

InfoBuilder& InfoBuilder::appendCentipawnScore(int cp, ScoreBound bound) {
    addSpace();
    m_stream << "score cp " << cp;
    
    // Add bound information if not exact
    if (bound == ScoreBound::LOWER) {
        m_stream << " lowerbound";
    } else if (bound == ScoreBound::UPPER) {
        m_stream << " upperbound";
    }
    
    return *this;
}

InfoBuilder& InfoBuilder::appendNodes(uint64_t nodes) {
    addSpace();
    m_stream << "nodes " << nodes;
    return *this;
}

InfoBuilder& InfoBuilder::appendTime(uint64_t milliseconds) {
    addSpace();
    m_stream << "time " << milliseconds;
    return *this;
}

InfoBuilder& InfoBuilder::appendNps(uint64_t nps) {
    addSpace();
    m_stream << "nps " << nps;
    return *this;
}

InfoBuilder& InfoBuilder::appendHashfull(int permil) {
    addSpace();
    m_stream << "hashfull " << permil;
    return *this;
}

InfoBuilder& InfoBuilder::appendTbhits(uint64_t tbhits) {
    addSpace();
    m_stream << "tbhits " << tbhits;
    return *this;
}

InfoBuilder& InfoBuilder::appendCurrmove(Move move, int moveNumber) {
    if (move != NO_MOVE) {
        addSpace();
        m_stream << "currmove " << SafeMoveExecutor::moveToString(move);
        addSpace();
        m_stream << "currmovenumber " << moveNumber;
    }
    return *this;
}

InfoBuilder& InfoBuilder::appendCurrmove(const std::string& moveStr, int moveNumber) {
    addSpace();
    m_stream << "currmove " << moveStr;
    addSpace();
    m_stream << "currmovenumber " << moveNumber;
    return *this;
}

InfoBuilder& InfoBuilder::appendPv(Move move) {
    if (move != NO_MOVE) {
        addSpace();
        m_stream << "pv " << SafeMoveExecutor::moveToString(move);
    }
    return *this;
}

InfoBuilder& InfoBuilder::appendPv(const std::string& moveStr) {
    addSpace();
    m_stream << "pv " << moveStr;
    return *this;
}

InfoBuilder& InfoBuilder::appendPv(const std::vector<Move>& moves) {
    if (!moves.empty()) {
        addSpace();
        m_stream << "pv";
        for (const Move& move : moves) {
            if (move != NO_MOVE) {
                m_stream << " " << SafeMoveExecutor::moveToString(move);
            }
        }
    }
    return *this;
}

InfoBuilder& InfoBuilder::appendPv(const std::vector<std::string>& moves) {
    if (!moves.empty()) {
        addSpace();
        m_stream << "pv";
        for (const std::string& moveStr : moves) {
            m_stream << " " << moveStr;
        }
    }
    return *this;
}

InfoBuilder& InfoBuilder::appendMultiPv(int pvIndex) {
    addSpace();
    m_stream << "multipv " << pvIndex;
    return *this;
}

InfoBuilder& InfoBuilder::appendString(const std::string& message) {
    addSpace();
    m_stream << "string " << message;
    return *this;
}

InfoBuilder& InfoBuilder::appendCustom(const std::string& key, const std::string& value) {
    addSpace();
    m_stream << key << " " << value;
    return *this;
}

InfoBuilder& InfoBuilder::appendCustom(const std::string& key, int value) {
    addSpace();
    m_stream << key << " " << value;
    return *this;
}

InfoBuilder& InfoBuilder::appendCustom(const std::string& key, double value) {
    addSpace();
    m_stream << key << " " << std::fixed << std::setprecision(2) << value;
    return *this;
}

std::string InfoBuilder::build() const {
    if (!m_hasContent) {
        return "info\n";
    }
    return "info " + m_stream.str() + "\n";
}

std::string InfoBuilder::buildRaw() const {
    return m_stream.str();
}

bool InfoBuilder::isEmpty() const {
    return !m_hasContent;
}

} // namespace seajay::uci