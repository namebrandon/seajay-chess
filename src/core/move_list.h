#pragma once

#include "types.h"
#include <array>
#include <string>
#include <string_view>
#include <iterator>
#include <algorithm>

namespace seajay {

// Forward declaration
class Board;

// Maximum number of legal moves in any chess position
// Theoretical maximum is around 218, but practical maximum for legal moves is lower
// We use 256 for safety and good cache alignment
constexpr size_t MAX_MOVES = 256;

/**
 * Stack-allocated container for chess moves with efficient iteration
 * Designed for move generation and search where memory allocation overhead
 * is critical for performance.
 */
class MoveList {
public:
    using value_type = Move;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = Move&;
    using const_reference = const Move&;
    using pointer = Move*;
    using const_pointer = const Move*;
    using iterator = Move*;
    using const_iterator = const Move*;

    // Constructors
    constexpr MoveList() noexcept : m_size(0) {}
    
    // Copy constructor
    MoveList(const MoveList& other) noexcept : m_size(other.m_size) {
        std::copy(other.begin(), other.end(), m_moves.begin());
    }
    
    // Move constructor
    MoveList(MoveList&& other) noexcept : m_size(other.m_size) {
        std::copy(other.begin(), other.end(), m_moves.begin());
        other.clear();
    }
    
    // Assignment operators
    MoveList& operator=(const MoveList& other) noexcept {
        if (this != &other) {
            m_size = other.m_size;
            std::copy(other.begin(), other.end(), m_moves.begin());
        }
        return *this;
    }
    
    MoveList& operator=(MoveList&& other) noexcept {
        if (this != &other) {
            m_size = other.m_size;
            std::copy(other.begin(), other.end(), m_moves.begin());
            other.clear();
        }
        return *this;
    }
    
    // Capacity
    constexpr bool empty() const noexcept { return m_size == 0; }
    constexpr size_type size() const noexcept { return m_size; }
    constexpr size_type max_size() const noexcept { return MAX_MOVES; }
    constexpr size_type capacity() const noexcept { return MAX_MOVES; }
    
    // Element access
    constexpr reference operator[](size_type pos) noexcept {
        return m_moves[pos];
    }
    
    constexpr const_reference operator[](size_type pos) const noexcept {
        return m_moves[pos];
    }
    
    constexpr reference at(size_type pos) {
        if (pos >= m_size) {
            throw std::out_of_range("MoveList::at: index out of range");
        }
        return m_moves[pos];
    }
    
    constexpr const_reference at(size_type pos) const {
        if (pos >= m_size) {
            throw std::out_of_range("MoveList::at: index out of range");
        }
        return m_moves[pos];
    }
    
    constexpr reference front() noexcept { return m_moves[0]; }
    constexpr const_reference front() const noexcept { return m_moves[0]; }
    constexpr reference back() noexcept { return m_moves[m_size - 1]; }
    constexpr const_reference back() const noexcept { return m_moves[m_size - 1]; }
    
    constexpr pointer data() noexcept { return m_moves.data(); }
    constexpr const_pointer data() const noexcept { return m_moves.data(); }
    
    // Iterators
    constexpr iterator begin() noexcept { return m_moves.data(); }
    constexpr const_iterator begin() const noexcept { return m_moves.data(); }
    constexpr const_iterator cbegin() const noexcept { return m_moves.data(); }
    
    constexpr iterator end() noexcept { return m_moves.data() + m_size; }
    constexpr const_iterator end() const noexcept { return m_moves.data() + m_size; }
    constexpr const_iterator cend() const noexcept { return m_moves.data() + m_size; }
    
    // Modifiers
    constexpr void clear() noexcept { m_size = 0; }
    
    constexpr void push_back(Move move) noexcept {
        if (m_size < MAX_MOVES) {
            m_moves[m_size++] = move;
        }
    }
    
    constexpr void pop_back() noexcept {
        if (m_size > 0) {
            --m_size;
        }
    }
    
    // Template function to add move with perfect forwarding
    template<ValidMove T>
    constexpr void add(T&& move) noexcept {
        push_back(static_cast<Move>(std::forward<T>(move)));
    }
    
    // Convenience function to add normal move
    constexpr void addMove(Square from, Square to, uint8_t flags = NORMAL) noexcept {
        push_back(makeMove(from, to, flags));
    }
    
    // Convenience function to add promotion moves (all 4 promotion types)
    constexpr void addPromotionMoves(Square from, Square to) noexcept {
        push_back(makePromotionMove(from, to, KNIGHT));
        push_back(makePromotionMove(from, to, BISHOP));
        push_back(makePromotionMove(from, to, ROOK));
        push_back(makePromotionMove(from, to, QUEEN));
    }
    
    // Convenience function to add promotion capture moves (all 4 promotion types)
    constexpr void addPromotionCaptureMoves(Square from, Square to) noexcept {
        push_back(makePromotionCaptureMove(from, to, KNIGHT));
        push_back(makePromotionCaptureMove(from, to, BISHOP));
        push_back(makePromotionCaptureMove(from, to, ROOK));
        push_back(makePromotionCaptureMove(from, to, QUEEN));
    }
    
    // Remove move at specific position (preserves order)
    constexpr void erase(size_type pos) noexcept {
        if (pos < m_size) {
            for (size_type i = pos; i < m_size - 1; ++i) {
                m_moves[i] = m_moves[i + 1];
            }
            --m_size;
        }
    }
    
    // Quick remove (swaps with last element - does not preserve order)
    constexpr void quickRemove(size_type pos) noexcept {
        if (pos < m_size) {
            m_moves[pos] = m_moves[--m_size];
        }
    }
    
    // Find move in list
    constexpr iterator find(Move move) noexcept {
        return std::find(begin(), end(), move);
    }
    
    constexpr const_iterator find(Move move) const noexcept {
        return std::find(begin(), end(), move);
    }
    
    constexpr bool contains(Move move) const noexcept {
        return find(move) != end();
    }
    
    // Comparison operators
    constexpr bool operator==(const MoveList& other) const noexcept {
        return m_size == other.m_size && 
               std::equal(begin(), end(), other.begin());
    }
    
    constexpr bool operator!=(const MoveList& other) const noexcept {
        return !(*this == other);
    }
    
    // String representation for debugging
    std::string toString() const;
    std::string toAlgebraicNotation(const Board& board) const;
    
private:
    std::array<Move, MAX_MOVES> m_moves;
    size_type m_size;
};

// Stream output operator
std::ostream& operator<<(std::ostream& os, const MoveList& moves);

} // namespace seajay