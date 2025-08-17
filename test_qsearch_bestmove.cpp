// Test program to verify quiescence search best move tracking
#include <iostream>
#include <iomanip>
#include "../src/core/board.h"
#include "../src/search/quiescence.h"
#include "../src/search/search_info.h"
#include "../src/search/types.h"
#include "../src/core/transposition_table.h"
#include "../src/movegen/movegen.h"

using namespace seajay;

int main() {
    // Test position with obvious best capture
    // After 1.e4 e5 2.Nf3 Nc6 3.Bb5 a6 4.Bxc6 (White just captured on c6)
    std::string fen = "r1bqkbnr/1ppp1ppp/p1n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 4";
    
    Board board;
    board.fromFEN(fen);
    
    std::cout << "Testing position: " << fen << "\n";
    std::cout << "Black to move - obvious recapture dxc6 available\n\n";
    
    // Create TT with 16MB
    TranspositionTable tt(16);
    tt.enable();
    
    // Setup search structures
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    search::SearchData data;
    search::SearchLimits limits;
    
    // Call quiescence search
    std::cout << "Calling quiescence search...\n";
    eval::Score score = search::quiescence(board, 0, 
                                          eval::Score(-100), eval::Score(100),
                                          searchInfo, data, limits, tt);
    
    std::cout << "Quiescence score: " << score.value() << " cp\n";
    std::cout << "Nodes searched: " << data.qnodes << "\n";
    
    // Check if TT entry was stored with best move
    TTEntry* entry = tt.probe(board.zobristKey());
    if (entry && entry->hasMove()) {
        Move ttMove = entry->getMove();
        std::cout << "\nTT entry found with best move!\n";
        std::cout << "Best move from TT: " << ttMove.toString() << "\n";
        
        // Verify it's the expected recapture
        if (ttMove.toString() == "d7c6" || ttMove.toString() == "b7c6") {
            std::cout << "✓ Correct recapture stored in TT\n";
        } else {
            std::cout << "Move stored but not the expected recapture\n";
        }
        
        std::cout << "TT bound type: ";
        switch(entry->getBound()) {
            case Bound::EXACT: std::cout << "EXACT\n"; break;
            case Bound::LOWER: std::cout << "LOWER\n"; break;
            case Bound::UPPER: std::cout << "UPPER\n"; break;
            default: std::cout << "UNKNOWN\n";
        }
    } else if (entry) {
        std::cout << "\n✗ TT entry found but NO MOVE stored (this was the bug)\n";
    } else {
        std::cout << "\n✗ No TT entry found\n";
    }
    
    // Try another position - tactical position with clear best capture
    std::cout << "\n--- Test 2: Tactical position ---\n";
    fen = "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4";
    board.fromFEN(fen);
    
    std::cout << "Position: " << fen << "\n";
    std::cout << "White to move - Bxc6 is available\n\n";
    
    // Clear for new search
    tt.clear();
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    data = search::SearchData();
    
    score = search::quiescence(board, 0,
                              eval::Score(-100), eval::Score(100),
                              searchInfo, data, limits, tt);
    
    std::cout << "Quiescence score: " << score.value() << " cp\n";
    std::cout << "Nodes searched: " << data.qnodes << "\n";
    
    entry = tt.probe(board.zobristKey());
    if (entry && entry->hasMove()) {
        Move ttMove = entry->getMove();
        std::cout << "\nTT entry found with best move: " << ttMove.toString() << "\n";
        if (ttMove.toString() == "b5c6") {
            std::cout << "✓ Correct capture Bxc6 stored\n";
        }
    } else if (entry) {
        std::cout << "\n✗ TT entry found but NO MOVE stored\n";
    } else {
        std::cout << "\n✗ No TT entry found\n";
    }
    
    std::cout << "\n=== Best move tracking test complete ===\n";
    
    return 0;
}