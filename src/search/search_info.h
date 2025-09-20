#pragma once

#include "../core/types.h"
#include "../evaluation/types.h"  // For eval::Score (Phase A3)
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
    bool wasNullMove = false;  // Track if this was a null move
    int staticEval = 0;        // Static evaluation at this node
    int moveCount = 0;         // Number of moves searched at this node
    bool isPvNode = false;      // Track if this is a PV node (Phase P1)
    int searchedMoves = 0;      // Count of moves already searched (Phase P1)
    Move excludedMove = NO_MOVE; // Move to exclude in singular search
    bool gaveCheck = false;      // Whether the move leading to this node delivered check
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
        m_extensionApplied.fill(0);
        m_extensionTotal.fill(0);
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
            m_searchStack[ply].gaveCheck = false;  // Reset; set after legality if needed
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

    void setGaveCheck(int ply, bool gaveCheck) {
        if (ply >= 0 && ply < MAX_PLY) {
            m_searchStack[ply].gaveCheck = gaveCheck;
        }
    }

    bool moveGaveCheck(int ply) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_searchStack[ply].gaveCheck;
        }
        return false;
    }

    void setExtensionApplied(int ply, int extension) {
        if (ply < 0 || ply >= MAX_PLY) {
            return;
        }
        m_extensionApplied[ply] = extension;
        int parentTotal = (ply > 0) ? m_extensionTotal[ply - 1] : 0;
        m_extensionTotal[ply] = parentTotal + extension;
    }

    int extensionApplied(int ply) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_extensionApplied[ply];
        }
        return 0;
    }

    int totalExtensions(int ply) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_extensionTotal[ply];
        }
        return 0;
    }
    
    // Current search ply
    int searchPly() const { return m_searchPly; }
    void setSearchPly(int ply) { m_searchPly = ply; }
    void incrementPly() { m_searchPly++; }
    void decrementPly() { m_searchPly--; }
    
    // Get root game history size
    size_t getRootGameHistorySize() const { return m_rootGameHistorySize; }
    
    // Check if move at given ply was a null move
    bool wasNullMove(int ply) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_searchStack[ply].wasNullMove;
        }
        return false;
    }
    
    // Set null move flag at given ply
    void setNullMove(int ply, bool wasNull) {
        if (ply >= 0 && ply < MAX_PLY) {
            m_searchStack[ply].wasNullMove = wasNull;
        }
    }
    
    // Phase A3: Set static evaluation at given ply
    void setStaticEval(int ply, eval::Score eval) {
        if (ply >= 0 && ply < MAX_PLY) {
            m_searchStack[ply].staticEval = eval.value();
        }
    }
    
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
    
    // Phase P1: PV node tracking helper methods
    void setPvNode(int ply, bool isPv) {
        if (ply >= 0 && ply < MAX_PLY) {
            m_searchStack[ply].isPvNode = isPv;
        }
    }
    
    bool isPvNode(int ply) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_searchStack[ply].isPvNode;
        }
        return false;
    }
    
    void incrementSearchedMoves(int ply) {
        if (ply >= 0 && ply < MAX_PLY) {
            m_searchStack[ply].searchedMoves++;
        }
    }
    
    void resetSearchedMoves(int ply) {
        if (ply >= 0 && ply < MAX_PLY) {
            m_searchStack[ply].searchedMoves = 0;
        }
    }
    
    int getSearchedMoves(int ply) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_searchStack[ply].searchedMoves;
        }
        return 0;
    }
    
    // Singular extension support: excluded move management
    void setExcludedMove(int ply, Move move) {
        if (ply >= 0 && ply < MAX_PLY) {
            m_searchStack[ply].excludedMove = move;
        }
    }
    
    Move getExcludedMove(int ply) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_searchStack[ply].excludedMove;
        }
        return NO_MOVE;
    }
    
    bool isExcluded(int ply, Move move) const {
        if (ply >= 0 && ply < MAX_PLY) {
            return m_searchStack[ply].excludedMove == move;
        }
        return false;
    }
    
private:
    std::array<SearchStack, MAX_PLY> m_searchStack;
    std::array<int, MAX_PLY> m_extensionApplied{};
    std::array<int, MAX_PLY> m_extensionTotal{};
    int m_searchPly;
    size_t m_rootGameHistorySize;  // Where game history ends
};

} // namespace seajay
