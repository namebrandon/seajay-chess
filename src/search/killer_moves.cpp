#include "killer_moves.h"
#include <cstring>  // For memset

namespace seajay::search {

void KillerMoves::clear() {
    // Initialize all killer slots to NO_MOVE
    for (int ply = 0; ply < MAX_PLY; ++ply) {
        for (int slot = 0; slot < KILLERS_PER_PLY; ++slot) {
            m_killers[ply][slot] = NO_MOVE;
        }
    }
}

void KillerMoves::update(int ply, Move move) {
    // Bounds check
    if (ply < 0 || ply >= MAX_PLY) {
        return;
    }
    
    // Don't store captures or promotions as killers
    // Killers are specifically for quiet moves
    if (isCapture(move) || isPromotion(move)) {
        return;
    }
    
    // Don't store if it's already the first killer (avoid duplicates)
    if (m_killers[ply][0] == move) {
        return;
    }
    
    // Shift killers: new move goes to slot 0, old slot 0 moves to slot 1
    // This implements a simple aging scheme where newer killers are preferred
    m_killers[ply][1] = m_killers[ply][0];
    m_killers[ply][0] = move;
}

bool KillerMoves::isKiller(int ply, Move move) const {
    // Bounds check
    if (ply < 0 || ply >= MAX_PLY) {
        return false;
    }
    
    // Check if move matches either killer slot
    return m_killers[ply][0] == move || m_killers[ply][1] == move;
}

Move KillerMoves::getKiller(int ply, int slot) const {
    // Bounds check
    if (ply < 0 || ply >= MAX_PLY || slot < 0 || slot >= KILLERS_PER_PLY) {
        return NO_MOVE;
    }
    
    return m_killers[ply][slot];
}

} // namespace seajay::search