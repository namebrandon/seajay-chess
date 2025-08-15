#include <iostream>
#include <iomanip>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/search/negamax.h"
#include "../src/search/quiescence.h"
#include "../src/core/transposition_table.h"

using namespace seajay;

// Positions where the king is in check
struct CheckPosition {
    const char* fen;
    const char* description;
};

CheckPosition checkPositions[] = {
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 b kq - 0 1", "Complex check position"},
    {"rnbqkb1r/pp1p1ppp/4pn2/8/2PP4/2N5/PP2PPPP/R1BQKBNR w KQkq - 0 4", "After Nf6 check"},
    {"r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 3", "Bishop check"},
    {"8/8/8/4k3/8/8/4R3/4K3 b - - 0 1", "Rook check on king"}
};

int main() {
    std::cout << "Testing Escape Route Prioritization\n\n";
    
    TranspositionTable tt(16);  // 16 MB
    
    for (const auto& pos : checkPositions) {
        std::cout << "Position: " << pos.description << "\n";
        std::cout << "FEN: " << pos.fen << "\n";
        
        Board board;
        auto result = board.parseFEN(pos.fen);
        if (!result.hasValue()) {
            std::cout << "Failed to parse FEN\n";
            continue;
        }
        
        // Check if position is actually in check
        if (!inCheck(board)) {
            std::cout << "Position is not in check, skipping...\n\n";
            continue;
        }
        
        SearchInfo searchInfo;
        search::SearchData data;
        data.timeLimit = std::chrono::milliseconds(100);
        
        // Run quiescence search
        eval::Score score = search::quiescence(
            board, 0,
            eval::Score(-10000), eval::Score(10000),
            searchInfo, data, tt, 0
        );
        
        std::cout << "Score: " << score.value() << "\n";
        std::cout << "Nodes searched: " << data.qsearchNodes << "\n";
        std::cout << "Cutoffs: " << data.qsearchCutoffs << "\n";
        
        // Calculate cutoff efficiency
        if (data.qsearchNodes > 0) {
            double cutoffRate = (100.0 * data.qsearchCutoffs) / data.qsearchNodes;
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Cutoff rate: " << cutoffRate << "%\n";
        }
        
        std::cout << "---\n";
    }
    
    std::cout << "\nEscape route prioritization test complete!\n";
    std::cout << "King moves should be searched first, improving cutoff rates.\n";
    
    return 0;
}