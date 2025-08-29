#pragma once

#include "../core/types.h"
#include <array>
#include <cstring>
#include <algorithm>

namespace seajay::search {

/**
 * TriangularPV - Triangular Principal Variation array for storing the best move sequence
 * 
 * This class implements a triangular array structure optimized for cache performance
 * and thread safety. Each search thread maintains its own PV array, eliminating
 * synchronization overhead in multi-threaded search.
 * 
 * Memory layout uses a flattened triangular array for better cache locality.
 * Total memory usage: ~17KB per thread.
 */
class TriangularPV {
public:
    // Maximum search depth we support
    static constexpr int MAX_DEPTH = 128;
    
    // Cache line size for alignment (typical x86-64)
    static constexpr int CACHE_LINE_SIZE = 64;
    
    // Constructor - zero initialize
    TriangularPV() {
        clear();
    }
    
    /**
     * Update PV when a new best move is found
     * @param ply Current ply depth
     * @param move Best move found
     * @param childPV PV from the child node (nullptr for leaf nodes)
     */
    inline void updatePV(int ply, Move move, const TriangularPV* childPV = nullptr) {
        // Bounds check
        if (ply >= MAX_DEPTH) return;
        
        // Set the first move at this ply
        pvArray[getIndex(ply, 0)] = move;
        
        // Copy the child's PV after our move
        if (childPV && ply + 1 < MAX_DEPTH) {
            int childLength = childPV->pvLength[ply + 1];
            
            if (childLength > 0 && childLength < MAX_DEPTH - ply - 1) {
                // Get source and destination pointers
                const Move* src = &childPV->pvArray[getIndex(ply + 1, 0)];
                Move* dst = &pvArray[getIndex(ply, 1)];
                
                // Optimized copy for common case (short PVs)
                if (childLength <= 8) {
                    // Unrolled copy for better performance
                    for (int i = 0; i < childLength; ++i) {
                        dst[i] = src[i];
                    }
                } else {
                    // Bulk copy for longer PVs
                    std::memcpy(dst, src, childLength * sizeof(Move));
                }
                
                pvLength[ply] = static_cast<uint8_t>(childLength + 1);
            } else {
                pvLength[ply] = 1;
            }
        } else {
            pvLength[ply] = 1;
        }
    }
    
    /**
     * Clear PV at given ply (used for fail-low nodes)
     */
    inline void clearPV(int ply) {
        if (ply < MAX_DEPTH) {
            pvLength[ply] = 0;
        }
    }
    
    /**
     * Clear entire PV array
     */
    void clear() {
        std::memset(pvLength.data(), 0, sizeof(pvLength));
        // Note: We don't need to clear pvArray as pvLength tracks valid data
    }
    
    /**
     * Get the length of PV at given ply
     */
    inline int getLength(int ply) const {
        return (ply < MAX_DEPTH) ? pvLength[ply] : 0;
    }
    
    /**
     * Get a specific move from the PV
     * @param ply The ply depth
     * @param index The move index in the PV (0 = first move)
     */
    inline Move getMove(int ply, int index) const {
        if (ply >= MAX_DEPTH || index >= pvLength[ply]) {
            return NO_MOVE;
        }
        return pvArray[getIndex(ply, index)];
    }
    
    /**
     * Extract the full PV line from a given ply as a vector
     * Useful for UCI output
     */
    std::vector<Move> extractPV(int ply = 0) const {
        std::vector<Move> pv;
        if (ply >= MAX_DEPTH) return pv;
        
        int length = pvLength[ply];
        if (length > 0) {
            pv.reserve(length);
            for (int i = 0; i < length; ++i) {
                Move move = pvArray[getIndex(ply, i)];
                if (move == NO_MOVE) break;  // Safety check
                pv.push_back(move);
            }
        }
        return pv;
    }
    
    /**
     * Check if PV is empty at given ply
     */
    inline bool isEmpty(int ply) const {
        return ply >= MAX_DEPTH || pvLength[ply] == 0;
    }

private:
    // Calculate flat array index for triangular storage
    // This maps 2D triangular indices to 1D array
    static constexpr size_t getIndex(int ply, int moveIndex) {
        // Triangular indexing formula:
        // Row ply starts at: ply * (2 * MAX_DEPTH - ply + 1) / 2
        // Then add moveIndex offset within the row
        return static_cast<size_t>(ply) * (2 * MAX_DEPTH - ply + 1) / 2 + moveIndex;
    }
    
    // Flattened triangular array for PV storage
    // Size: MAX_DEPTH * (MAX_DEPTH + 1) / 2
    // This stores all possible PV moves in a cache-friendly layout
    alignas(CACHE_LINE_SIZE) std::array<Move, (MAX_DEPTH * (MAX_DEPTH + 1)) / 2> pvArray;
    
    // Length of PV at each ply (small array, fits in cache)
    alignas(CACHE_LINE_SIZE) std::array<uint8_t, MAX_DEPTH> pvLength;
};

// Verify the structure size is reasonable
static_assert(sizeof(TriangularPV) < 20000, "TriangularPV size exceeds expected bounds");

} // namespace seajay::search