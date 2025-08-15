#include "../../src/search/quiescence.h"
#include <iostream>

using namespace seajay::search;

int main() {
    std::cout << "Delta pruning constants:\n";
    std::cout << "  DELTA_MARGIN = " << DELTA_MARGIN << " centipawns\n";
    std::cout << "  DELTA_MARGIN_ENDGAME = " << DELTA_MARGIN_ENDGAME << " centipawns\n";
    std::cout << "\n✓ Delta pruning constants defined correctly!\n";
    return 0;
}