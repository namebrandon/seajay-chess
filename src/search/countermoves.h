#pragma once

#include "../core/types.h"
#include <cstring>

namespace seajay {

class CounterMoves {
public:
    static constexpr int DEFAULT_BONUS = 8000;
    
    CounterMoves() { 
        clear(); 
    }
    
    void clear() {
        std::memset(m_counters, 0, sizeof(m_counters));
    }
    
    void update(Move prevMove, Move counterMove) {
        if (prevMove == NO_MOVE) {
            return;
        }
        
        if (isCapture(counterMove) || isPromotion(counterMove)) {
            return;
        }
        
        const Square from = moveFrom(prevMove);
        const Square to = moveTo(prevMove);
        m_counters[from][to] = counterMove;
    }
    
    Move getCounterMove(Move prevMove) const {
        if (prevMove == NO_MOVE) {
            return NO_MOVE;
        }
        
        const Square from = moveFrom(prevMove);
        const Square to = moveTo(prevMove);
        return m_counters[from][to];
    }
    
    bool hasCounterMove(Move prevMove) const {
        return getCounterMove(prevMove) != NO_MOVE;
    }

private:
    alignas(64) Move m_counters[64][64];
};

} // namespace seajay