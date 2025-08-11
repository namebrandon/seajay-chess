#pragma once

#include "../core/types.h"
#include <array>
#include <cstdint>

namespace seajay {

// Maximum search depth
constexpr int MAX_PLY = 128;

// Stack entry for search history
struct SearchStack {
    Hash zobristKey = 0;
    Move move = NO_MOVE;
    int ply = 0;
};

// Search-specific information tracking
class SearchInfo {
public:
    SearchInfo() : m_searchPly(0), m_rootGameHistorySize(0) {}
    
    // Reset search info for new search
    void clear() {
        m_searchPly = 0;
        m_rootGameHistorySize = 0;
        // Clear search stack
        for (int i = 0; i < MAX_PLY; ++i) {
            m_searchStack[i] = SearchStack{};
        }
    }
    
    // Set the game history size at root (where game ends, search begins)
    void setRootHistorySize(size_t size) {
        m_rootGameHistorySize = size;
    }
    
    // Push position to search stack
    void pushSearchPosition(Hash zobrist, Move move, int ply) {
        if (ply < MAX_PLY) {
            m_searchStack[ply].zobristKey = zobrist;
            m_searchStack[ply].move = move;
            m_searchStack[ply].ply = ply;
        }
    }
    
    // Check if position repeats in search history
    // CRITICAL: In search, 1 repetition = draw
    bool isRepetitionInSearch(Hash zobrist, int currentPly) const {
        // Check search history backwards, skipping by 2 (same side to move)
        for (int i = currentPly - 4; i >= 0; i -= 2) {
            if (m_searchStack[i].zobristKey == zobrist) {
                return true;  // One repetition in search = draw!
            }
        }
        return false;
    }
    
    // Get search stack entry
    const SearchStack& getStackEntry(int ply) const {
        return m_searchStack[ply];
    }
    
    // Current search ply
    int searchPly() const { return m_searchPly; }
    void setSearchPly(int ply) { m_searchPly = ply; }
    void incrementPly() { m_searchPly++; }
    void decrementPly() { m_searchPly--; }
    
    // Get root game history size
    size_t getRootGameHistorySize() const { return m_rootGameHistorySize; }
    
    // Additional helper methods for performance optimization
    Hash getCurrentZobrist(int ply) const {
        if (ply < MAX_PLY) return m_searchStack[ply].zobristKey;
        return 0;
    }
    
    int getCurrentPly(int ply) const {
        if (ply < MAX_PLY) return m_searchStack[ply].ply;
        return 0;
    }
    
    Hash getZobristAt(int index) const {
        if (index >= 0 && index < MAX_PLY) return m_searchStack[index].zobristKey;
        return 0;
    }
    
private:
    std::array<SearchStack, MAX_PLY> m_searchStack;
    int m_searchPly;
    size_t m_rootGameHistorySize;  // Where game history ends
};

} // namespace seajay