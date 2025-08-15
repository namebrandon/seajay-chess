#include "see.h"
#include "move_generation.h"
#include "bitboard.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <iostream>  // For debug output

namespace seajay {

// Thread-local storage for swap list
thread_local SEECalculator::SwapList SEECalculator::m_swapList;

// Day 4.3: Constructor initializes cache
SEECalculator::SEECalculator() 
    : m_cache(std::make_unique<SEECacheEntry[]>(SEE_CACHE_SIZE)) {
    clearCache();
}

// Day 4.3: Clear the cache
void SEECalculator::clearCache() noexcept {
    for (size_t i = 0; i < SEE_CACHE_SIZE; ++i) {
        m_cache[i].key = 0;
        m_cache[i].value = 0;
        m_cache[i].age = 0;
    }
    m_currentAge = 0;
}

// Day 4.3: Age cache entries for replacement
void SEECalculator::ageCache() noexcept {
    m_currentAge++;
    // Periodically reset ages to prevent overflow
    if (m_currentAge == 255) {
        for (size_t i = 0; i < SEE_CACHE_SIZE; ++i) {
            m_cache[i].age = m_cache[i].age >> 1;  // Halve all ages
        }
        m_currentAge = 128;
    }
}

// Day 4.3: Generate cache key from position and move
uint64_t SEECalculator::makeCacheKey(const Board& board, Move move) const noexcept {
    // Combine board's Zobrist hash with move information
    // We need a unique key for each position + move combination
    uint64_t boardKey = board.zobristKey();
    
    // If zobrist key is 0, board hasn't been initialized properly
    // Create a simple hash from piece positions
    if (boardKey == 0) {
        // Simple fallback hash for testing
        boardKey = 0;
        for (Square sq = A1; sq <= H8; ++sq) {
            Piece p = board.pieceAt(sq);
            if (p != NO_PIECE) {
                boardKey ^= (uint64_t(p) << (sq % 32)) * 0x9E3779B97F4A7C15ULL;
                boardKey = (boardKey << 13) | (boardKey >> 51);  // Rotate
            }
        }
        // Add side to move
        boardKey ^= board.sideToMove() ? 0x1234567890ABCDEFULL : 0;
    }
    
    uint64_t moveKey = (uint64_t(move) << 32) | (uint64_t(move) * 0x9E3779B97F4A7C15ULL);
    return boardKey ^ moveKey;
}

// Day 4.3: Probe the cache for a stored result
SEEValue SEECalculator::probeCache(uint64_t key) const noexcept {
    size_t index = key & SEE_CACHE_MASK;
    uint64_t storedKey = m_cache[index].key.load(std::memory_order_relaxed);
    
    if (m_debugOutput) {
        std::cout << "Probe: key=0x" << std::hex << key 
                  << " index=" << std::dec << index
                  << " stored=0x" << std::hex << storedKey << std::dec << std::endl;
    }
    
    if (storedKey == key && storedKey != 0) {  // Don't match on uninitialized entries
        m_stats.cacheHits.fetch_add(1, std::memory_order_relaxed);
        // Update age on hit
        m_cache[index].age.store(m_currentAge, std::memory_order_relaxed);
        SEEValue result = m_cache[index].value.load(std::memory_order_relaxed);
        if (m_debugOutput) {
            std::cout << "  HIT! value=" << result << std::endl;
        }
        return result;
    }
    
    m_stats.cacheMisses.fetch_add(1, std::memory_order_relaxed);
    return SEE_UNKNOWN;
}

// Day 4.3: Store a result in the cache
void SEECalculator::storeCache(uint64_t key, SEEValue value) const noexcept {
    size_t index = key & SEE_CACHE_MASK;
    
    if (m_debugOutput) {
        std::cout << "Store: key=0x" << std::hex << key 
                  << " index=" << std::dec << index
                  << " value=" << value << std::endl;
    }
    
    // Always store for now (debugging)
    m_cache[index].key.store(key, std::memory_order_relaxed);
    m_cache[index].value.store(value, std::memory_order_relaxed);
    m_cache[index].age.store(m_currentAge, std::memory_order_relaxed);
}

// Day 1.2: Attack detection wrapper
// Day 4.2: Optimized with reduced redundant calculations
inline Bitboard SEECalculator::attackersTo(const Board& board, Square sq, Bitboard occupied) const noexcept {
    Bitboard attackers = 0;
    
    // Day 4.2: Optimized pawn attack detection using precomputed patterns
    // Pawn attacks - check from the target square's perspective
    const Bitboard whitePawnTargets = ((sq > 8 && sq % 8 != 0) ? squareBB(sq - 9) : 0) |
                                      ((sq > 8 && sq % 8 != 7) ? squareBB(sq - 7) : 0);
    const Bitboard blackPawnTargets = ((sq < 56 && sq % 8 != 0) ? squareBB(sq + 7) : 0) |
                                      ((sq < 56 && sq % 8 != 7) ? squareBB(sq + 9) : 0);
    
    attackers |= whitePawnTargets & board.pieces(WHITE, PAWN) & occupied;
    attackers |= blackPawnTargets & board.pieces(BLACK, PAWN) & occupied;
    
    // Knight attacks - single lookup
    const Bitboard knightAttacks = MoveGenerator::getKnightAttacks(sq);
    attackers |= knightAttacks & (board.pieces(WHITE, KNIGHT) | board.pieces(BLACK, KNIGHT)) & occupied;
    
    // King attacks - single lookup
    const Bitboard kingAttacks = MoveGenerator::getKingAttacks(sq);
    attackers |= kingAttacks & (board.pieces(WHITE, KING) | board.pieces(BLACK, KING)) & occupied;
    
    // Day 4.2: Combine queen lookups with bishops/rooks to avoid redundant operations
    const Bitboard queens = (board.pieces(WHITE, QUEEN) | board.pieces(BLACK, QUEEN)) & occupied;
    
    // Bishop/Queen diagonal attacks
    const Bitboard bishopAttacks = MoveGenerator::getBishopAttacks(sq, occupied);
    const Bitboard bishops = (board.pieces(WHITE, BISHOP) | board.pieces(BLACK, BISHOP)) & occupied;
    attackers |= bishopAttacks & (bishops | queens);
    
    // Rook/Queen straight attacks  
    const Bitboard rookAttacks = MoveGenerator::getRookAttacks(sq, occupied);
    const Bitboard rooks = (board.pieces(WHITE, ROOK) | board.pieces(BLACK, ROOK)) & occupied;
    attackers |= rookAttacks & (rooks | queens);
    
    return attackers;
}

// Day 2.2: Find least valuable attacker with proper ordering
// Day 4.2: Marked inline for hot path
inline Bitboard SEECalculator::leastValuableAttacker(const Board& board, Bitboard attackers,
                                                     Color side, PieceType& attacker) const noexcept {
    // Check attackers in order of piece value
    // Order: pawn < knight = bishop < rook < queen < king
    // This ensures we always use the least valuable piece for captures
    
    // Check pawns first (value = 100)
    Bitboard pieces = board.pieces(side, PAWN) & attackers;
    if (pieces) {
        attacker = PAWN;
        return pieces & -pieces;  // Return LSB (one attacker)
    }
    
    // Check knights and bishops (both value = 325)
    // Check knights first for consistency
    pieces = board.pieces(side, KNIGHT) & attackers;
    if (pieces) {
        attacker = KNIGHT;
        return pieces & -pieces;
    }
    
    pieces = board.pieces(side, BISHOP) & attackers;
    if (pieces) {
        attacker = BISHOP;
        return pieces & -pieces;
    }
    
    // Check rooks (value = 500)
    pieces = board.pieces(side, ROOK) & attackers;
    if (pieces) {
        attacker = ROOK;
        return pieces & -pieces;
    }
    
    // Check queens (value = 975)
    pieces = board.pieces(side, QUEEN) & attackers;
    if (pieces) {
        attacker = QUEEN;
        return pieces & -pieces;
    }
    
    // Check king last (value = 10000)
    // King can capture but cannot be captured
    pieces = board.pieces(side, KING) & attackers;
    if (pieces) {
        attacker = KING;
        return pieces & -pieces;
    }
    
    attacker = NO_PIECE_TYPE;
    return 0;
}

// Day 3.1: X-ray detection - find sliding attackers revealed when a piece moves
Bitboard SEECalculator::getXrayAttackers(const Board& board, Square sq, 
                                          Bitboard occupied, Bitboard removedPiece) const noexcept {
    Bitboard xrayAttackers = 0;
    
    // Only sliding pieces can x-ray: bishops, rooks, and queens
    // X-rays only occur along the natural movement pattern of the piece
    
    // Find the square that was just vacated
    if (!removedPiece) return 0;
    Square removedSq = lsb(removedPiece);
    
    // Check if the removed piece was between the target square and potential attackers
    // We need to check all four ray types: diagonal and straight
    
    // Method: For each sliding piece type, check if:
    // 1. It can attack the target square now (with piece removed)
    // 2. It couldn't attack before (with piece present) 
    // 3. The removed piece was between them on the ray
    
    // Get all potential x-ray attackers
    Bitboard bishops = board.pieces(WHITE, BISHOP) | board.pieces(BLACK, BISHOP);
    Bitboard rooks = board.pieces(WHITE, ROOK) | board.pieces(BLACK, ROOK);
    Bitboard queens = board.pieces(WHITE, QUEEN) | board.pieces(BLACK, QUEEN);
    
    // Check diagonal x-rays (bishops and queens)
    Bitboard diagonalAttackers = (bishops | queens) & occupied;
    if (diagonalAttackers) {
        // Get attacks with piece removed
        Bitboard newBishopAttacks = MoveGenerator::getBishopAttacks(sq, occupied);
        
        // Get attacks with piece present
        Bitboard oldOccupied = occupied | removedPiece;
        Bitboard oldBishopAttacks = MoveGenerator::getBishopAttacks(sq, oldOccupied);
        
        // Find pieces that can attack now but couldn't before
        Bitboard potentialXrays = (diagonalAttackers & newBishopAttacks) & ~(diagonalAttackers & oldBishopAttacks);
        
        // Verify the removed piece was actually between them
        Bitboard xraysCopy = potentialXrays;
        while (xraysCopy) {
            Square xraySq = popLsb(xraysCopy);
            Bitboard betweenBB = between(xraySq, sq);
            if (betweenBB & removedPiece) {
                xrayAttackers |= squareBB(xraySq);
            }
        }
    }
    
    // Check straight line x-rays (rooks and queens)
    Bitboard straightAttackers = (rooks | queens) & occupied;
    if (straightAttackers) {
        // Get attacks with piece removed
        Bitboard newRookAttacks = MoveGenerator::getRookAttacks(sq, occupied);
        
        // Get attacks with piece present
        Bitboard oldOccupied = occupied | removedPiece;
        Bitboard oldRookAttacks = MoveGenerator::getRookAttacks(sq, oldOccupied);
        
        // Find pieces that can attack now but couldn't before
        Bitboard potentialXrays = (straightAttackers & newRookAttacks) & ~(straightAttackers & oldRookAttacks);
        
        // Verify the removed piece was actually between them
        Bitboard xraysCopy = potentialXrays;
        while (xraysCopy) {
            Square xraySq = popLsb(xraysCopy);
            Bitboard betweenBB = between(xraySq, sq);
            if (betweenBB & removedPiece) {
                xrayAttackers |= squareBB(xraySq);
            }
        }
    }
    
    return xrayAttackers;
}

// Day 2.1: SEE implementation with full multi-piece exchange support
// Day 4.1: Performance optimizations - early exit, lazy evaluation, branch hints
// Day 4.3: Cache integration
SEEValue SEECalculator::see(const Board& board, Move move) const noexcept {
    // Day 4.4: Statistics tracking
    m_stats.calls.fetch_add(1, std::memory_order_relaxed);
    
    // Day 4.3: Check cache first
    uint64_t cacheKey = makeCacheKey(board, move);
    SEEValue cached = probeCache(cacheKey);
    if (cached != SEE_UNKNOWN && cached != SEE_INVALID) {
        if (m_debugOutput) {
            std::cout << "SEE cache hit for move " 
                      << squareToString(moveFrom(move)) 
                      << squareToString(moveTo(move))
                      << " = " << cached << std::endl;
        }
        return cached;
    }
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    
    // Get the moving piece
    Piece movingPiece = board.pieceAt(from);
    if (movingPiece == NO_PIECE) [[unlikely]] {
        return SEE_INVALID;
    }
    
    PieceType movingType = typeOf(movingPiece);
    Color stm = colorOf(movingPiece);
    
    // Get the captured piece value (if any)
    Piece capturedPiece = board.pieceAt(to);
    int gain = 0;
    
    if (capturedPiece != NO_PIECE) {
        PieceType capturedType = typeOf(capturedPiece);
        gain = pieceValue(capturedType);
        
        // Day 4.1: Early exit for obviously good captures
        // If we capture a more valuable piece with a less valuable one,
        // and the captured piece is undefended, it's always good
        if (gain > pieceValue(movingType)) [[likely]] {
            // Quick check: if opponent has no pieces that can reach this square,
            // this is a free capture
            Bitboard occupied = board.occupied() ^ squareBB(from);
            Bitboard opponentAttackers = attackersTo(board, to, occupied) & board.pieces(~stm);
            if (!opponentAttackers) {
                m_stats.earlyExits.fetch_add(1, std::memory_order_relaxed);
                storeCache(cacheKey, gain);
                return gain;  // Free capture, no need for full SEE
            }
        }
    }
    
    // Special case for en passant
    if (isEnPassant(move)) [[unlikely]] {
        gain = pieceValue(PAWN);
    }
    
    // Special case for promotion (Day 2.4 prep)
    PieceType promType = NO_PIECE_TYPE;
    bool isPromo = isPromotion(move);
    if (isPromo) {
        promType = promotionType(move);
    }
    
    // Start with occupied squares
    Bitboard occupied = board.occupied();
    
    // CRITICAL: Remove the moving piece from occupied (#1 SEE bug!)
    occupied ^= squareBB(from);
    
    // Handle en passant capture square
    if (isEnPassant(move)) {
        Square epCaptureSquare = (stm == WHITE) ? to - 8 : to + 8;
        occupied ^= squareBB(epCaptureSquare);
    }
    
    // Get all attackers to the destination square
    Bitboard attackers = attackersTo(board, to, occupied);
    
    // Remove the moving piece from attackers (it already moved)
    attackers &= occupied;
    
    // If there are no attackers, return the simple gain
    // For promotions, add the difference in value
    if (!attackers) [[likely]] {
        SEEValue result = gain;
        if (isPromo) [[unlikely]] {
            result = gain + pieceValue(promType) - pieceValue(PAWN);
        }
        storeCache(cacheKey, result);
        return result;
    }
    
    // Day 4.1: Lazy evaluation for obviously bad exchanges
    // If we're moving a valuable piece to capture a less valuable one,
    // and the square is defended, do a quick pessimistic check
    if (gain < pieceValue(movingType)) {
        // Check if opponent has a pawn defender (most common)
        Bitboard opponentPawns = board.pieces(~stm, PAWN) & attackers;
        if (opponentPawns) [[likely]] {
            // Pessimistic bound: we lose our piece for their pawn
            int pessimisticValue = gain - pieceValue(movingType);
            // If this is clearly bad even in best case, exit early
            if (pessimisticValue < -200) {  // Threshold for "clearly bad"
                m_stats.lazyEvals.fetch_add(1, std::memory_order_relaxed);
                storeCache(cacheKey, pessimisticValue);
                return pessimisticValue;
            }
        }
    }
    
    // Initialize swap list for full exchange calculation
    m_swapList.clear();
    m_swapList.push(gain);
    
    // Material value of the piece that just moved to the square
    int materialOnSquare = pieceValue(movingType);
    
    // Handle promotion - the piece on the square changes value
    if (isPromo) {
        materialOnSquare = pieceValue(promType);
    }
    
    // Switch to opponent's turn
    stm = ~stm;
    
    // Perform the exchange sequence
    do {
        // Find least valuable attacker
        PieceType attacker;
        Bitboard attackerBB = leastValuableAttacker(board, attackers & board.pieces(stm), stm, attacker);
        
        if (!attackerBB) [[unlikely]] break;
        
        // Make the capture
        m_swapList.push(-m_swapList.gains[m_swapList.depth - 1] + materialOnSquare);
        
        // If this is a king capture, we can't continue (king can't be captured)
        if (materialOnSquare == pieceValue(KING)) [[unlikely]] {
            break;
        }
        
        // Update material on square
        materialOnSquare = pieceValue(attacker);
        
        // Remove attacker from board and check for x-rays (Day 3.2)
        occupied ^= attackerBB;
        
        // Day 3.2: Check for x-ray attackers revealed by removing this piece
        // Day 4.1: Only check x-rays for slider pieces (optimization)
        if (attacker == BISHOP || attacker == ROOK || attacker == QUEEN) [[unlikely]] {
            m_stats.xrayChecks.fetch_add(1, std::memory_order_relaxed);
            Bitboard xrays = getXrayAttackers(board, to, occupied, attackerBB);
            if (xrays) [[unlikely]] {
                // Add the x-ray attackers to our attacker set
                attackers |= xrays;
            }
        }
        
        // Update attackers to only include pieces still on board
        attackers &= occupied;
        
        // Switch sides
        stm = ~stm;
        
    } while (true);
    
    // Evaluate the swap list using minimax
    while (m_swapList.depth > 1) {
        m_swapList.depth--;
        m_swapList.gains[m_swapList.depth - 1] = 
            std::min(m_swapList.gains[m_swapList.depth - 1], 
                    -m_swapList.gains[m_swapList.depth]);
    }
    
    // Account for promotion value if applicable
    if (isPromo) {
        m_swapList.gains[0] += pieceValue(promType) - pieceValue(PAWN);
    }
    
    SEEValue result = m_swapList.gains[0];
    
    // Day 4.3: Store result in cache
    storeCache(cacheKey, result);
    
    // Day 4.4: Debug output if enabled
    if (m_debugOutput) {
        std::cout << "SEE: " << squareToString(from) << squareToString(to)
                  << " = " << result 
                  << " (swaps=" << m_swapList.depth << ")" << std::endl;
    }
    
    return result;
}

// Sign of SEE (positive, negative, or zero)
SEEValue SEECalculator::seeSign(const Board& board, Move move) const noexcept {
    SEEValue value = see(board, move);
    if (value > 0) return 1;
    if (value < 0) return -1;
    return 0;
}

// SEE greater than or equal to threshold
// Day 4.1: Optimized with early exit when threshold is met
bool SEECalculator::seeGE(const Board& board, Move move, SEEValue threshold) const noexcept {
    // For threshold <= 0, most captures will pass, so optimize for this case
    if (threshold <= 0) [[likely]] {
        // Any capture is >= 0 unless it's a losing exchange
        Square to = moveTo(move);
        Piece captured = board.pieceAt(to);
        
        // Non-captures are always == 0 (for now, without considering promotions)
        if (captured == NO_PIECE && !isEnPassant(move) && !isPromotion(move)) [[likely]] {
            return threshold <= 0;
        }
        
        // Quick check for obviously good captures
        if (captured != NO_PIECE) {
            Square from = moveFrom(move);
            Piece moving = board.pieceAt(from);
            
            // Capturing with a pawn is almost always good
            if (typeOf(moving) == PAWN) [[likely]] {
                return true;  // Pawn captures are >= 0 except in rare cases
            }
            
            // Capturing equal or more valuable piece is good if undefended
            if (pieceValue(typeOf(captured)) >= pieceValue(typeOf(moving))) {
                // Do quick undefended check
                Bitboard occupied = board.occupied() ^ squareBB(from);
                Bitboard defenders = attackersTo(board, to, occupied) & board.pieces(colorOf(captured));
                if (!defenders) {
                    return true;  // Free capture
                }
            }
        }
    }
    
    // Fall back to full SEE calculation
    return see(board, move) >= threshold;
}

// Day 2.1: Full swap algorithm with multi-piece exchanges
SEEValue SEECalculator::computeSEE(const Board& board, Square to, Color stm,
                                   Bitboard attackers, Bitboard occupied) const noexcept {
    // Initialize swap list
    m_swapList.clear();
    
    // Find the least valuable attacker
    PieceType attacker;
    Bitboard attackerBB = leastValuableAttacker(board, attackers & board.pieces(stm), stm, attacker);
    
    // If no attacker found, return 0
    if (!attackerBB) {
        return 0;
    }
    
    // Current material on the square
    int materialOnSquare = 0;
    Piece targetPiece = board.pieceAt(to);
    if (targetPiece != NO_PIECE) {
        materialOnSquare = pieceValue(typeOf(targetPiece));
    }
    
    // Perform the capture sequence
    do {
        // Add the gain to swap list
        m_swapList.push(materialOnSquare);
        
        // Update material on square (the attacker becomes the new target)
        materialOnSquare = pieceValue(attacker);
        
        // Switch side to move
        stm = ~stm;
        
        // Remove the attacker from occupied and attackers
        Square attackerSquare = lsb(attackerBB);
        occupied ^= attackerBB;
        
        // Day 3.2: Check for x-ray attackers revealed by removing this piece
        // Day 4.1: Only check x-rays for slider pieces (optimization)
        if (attacker == BISHOP || attacker == ROOK || attacker == QUEEN) [[unlikely]] {
            m_stats.xrayChecks.fetch_add(1, std::memory_order_relaxed);
            Bitboard xrays = getXrayAttackers(board, to, occupied, attackerBB);
            if (xrays) [[unlikely]] {
                // Add the x-ray attackers to our attacker set
                attackers |= xrays;
            }
        }
        
        // Update attackers to only include pieces still on board
        attackers &= occupied;
        
        // Find next least valuable attacker
        attackerBB = leastValuableAttacker(board, attackers & board.pieces(stm), stm, attacker);
        
    } while (attackerBB);
    
    // Now evaluate the swap list using negamax-like algorithm
    // Work backwards through the swap list
    while (m_swapList.depth > 1) {
        m_swapList.depth--;
        m_swapList.gains[m_swapList.depth - 1] = 
            -std::max(-m_swapList.gains[m_swapList.depth - 1], m_swapList.gains[m_swapList.depth]);
    }
    
    return m_swapList.gains[0];
}

} // namespace seajay