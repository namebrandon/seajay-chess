#include <iostream>
#include <cstdint>

using Bitboard = uint64_t;
using Square = uint8_t;
using File = uint8_t;
using Rank = uint8_t;

constexpr File fileOf(Square s) {
    return s & 7;
}

constexpr Rank rankOf(Square s) {
    return s >> 3;
}

constexpr Square makeSquare(File f, Rank r) {
    return (r << 3) | f;
}

constexpr Bitboard squareBB(Square s) {
    return 1ULL << s;
}

Bitboard computeRookMask(Square sq) {
    Bitboard mask = 0;
    int f = static_cast<int>(fileOf(sq));
    int r = static_cast<int>(rankOf(sq));
    
    std::cout << "Square " << (int)sq << ": file=" << f << ", rank=" << r << "\n";
    
    // North ray (exclude rank 8)
    for (int r2 = r + 1; r2 < 7; ++r2) {
        Square s = makeSquare(static_cast<File>(f), static_cast<Rank>(r2));
        std::cout << "  North: adding square " << (int)s << "\n";
        mask |= squareBB(s);
    }
    
    return mask;
}

int main() {
    std::cout << "Testing minimal computeRookMask...\n";
    
    Bitboard mask = computeRookMask(0);
    std::cout << "Mask: 0x" << std::hex << mask << std::dec << "\n";
    
    return 0;
}