// PVS Diagnostic Test
// This test reveals why re-search rates are so low

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

// Test positions that should trigger PVS re-searches
struct TestPosition {
    std::string name;
    std::string fen;
    int depth;
    std::string description;
};

std::vector<TestPosition> getPVSTestPositions() {
    return {
        // Positions where first move is NOT best - forces re-searches
        {
            "Tactical Position 1",
            "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
            6,
            "After 1.e4 e5 2.Nf3 Nc6 3.Bb5 Nf6 - Multiple good moves"
        },
        {
            "Complex Middle Game",
            "r1bq1rk1/pp2ppbp/2np1np1/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQ - 0 8",
            7,
            "King's Indian structure - many moves have similar evaluations"
        },
        {
            "Endgame with Multiple Paths",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            8,
            "Rook endgame where move order matters"
        },
        {
            "Position After Exchange",
            "rnbqk2r/pp2ppbp/3p1np1/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 7",
            6,
            "Position after exchange - unstable evaluation"
        },
        {
            "Critical Tactical Position",
            "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            5,
            "Wild tactical position with many forcing moves"
        }
    };
}

// Script to run the diagnostic
void generateDiagnosticScript() {
    std::cout << "#!/bin/bash\n";
    std::cout << "# PVS Diagnostic Script\n";
    std::cout << "# This script tests positions that should trigger PVS re-searches\n\n";
    
    std::cout << "ENGINE=\"./bin/seajay\"\n";
    std::cout << "RESULTS_FILE=\"pvs_diagnostic_results.txt\"\n\n";
    
    std::cout << "echo \"PVS Diagnostic Test Results\" > $RESULTS_FILE\n";
    std::cout << "echo \"===========================\" >> $RESULTS_FILE\n";
    std::cout << "echo \"\" >> $RESULTS_FILE\n\n";
    
    auto positions = getPVSTestPositions();
    
    for (const auto& pos : positions) {
        std::cout << "echo \"Testing: " << pos.name << "\" | tee -a $RESULTS_FILE\n";
        std::cout << "echo \"FEN: " << pos.fen << "\" >> $RESULTS_FILE\n";
        std::cout << "echo \"Description: " << pos.description << "\" >> $RESULTS_FILE\n";
        std::cout << "echo \"\" >> $RESULTS_FILE\n";
        
        std::cout << "$ENGINE << EOF | grep -E \"(bestmove|PVS|re-search|depth " 
                  << pos.depth << ")\" | tee -a $RESULTS_FILE\n";
        std::cout << "uci\n";
        std::cout << "setoption name ShowPVSStats value true\n";
        std::cout << "position fen " << pos.fen << "\n";
        std::cout << "go depth " << pos.depth << "\n";
        std::cout << "quit\n";
        std::cout << "EOF\n\n";
        std::cout << "echo \"\" >> $RESULTS_FILE\n";
        std::cout << "echo \"----------------------------------------\" >> $RESULTS_FILE\n";
        std::cout << "echo \"\" >> $RESULTS_FILE\n\n";
    }
    
    std::cout << "echo \"Test complete. Results saved to $RESULTS_FILE\"\n";
    std::cout << "cat $RESULTS_FILE\n";
}

// Analysis of what SHOULD happen
void explainPVSBehavior() {
    std::cout << "\nExpected PVS Behavior Analysis:\n";
    std::cout << "================================\n\n";
    
    std::cout << "In a properly functioning PVS implementation:\n\n";
    
    std::cout << "1. Scout Search Window:\n";
    std::cout << "   - Scout uses null window: [alpha, alpha+1]\n";
    std::cout << "   - This is a minimal window to quickly test if move beats alpha\n\n";
    
    std::cout << "2. Re-search Triggers:\n";
    std::cout << "   - Scout fails high: score > alpha\n";
    std::cout << "   - But score might be >> alpha+1 due to fail-soft\n";
    std::cout << "   - Need full window search to get exact score\n\n";
    
    std::cout << "3. Your Implementation Issue:\n";
    std::cout << "   - Scout search: -(alpha+1), -alpha (negated becomes [alpha, alpha+1])\n";
    std::cout << "   - Fail high returns score >= alpha+1\n";
    std::cout << "   - Your condition: score > alpha && score < beta\n";
    std::cout << "   - Problem: score is often >= beta after scout fail-high!\n\n";
    
    std::cout << "4. The Fix:\n";
    std::cout << "   - Change condition to: if (score > alpha)\n";
    std::cout << "   - This triggers re-search whenever scout finds better move\n";
    std::cout << "   - But wait... you might be doing fail-hard in scout?\n\n";
}

int main() {
    std::cout << "PVS Diagnostic Test Generator\n";
    std::cout << "==============================\n\n";
    
    explainPVSBehavior();
    
    std::cout << "\nGenerating diagnostic script...\n";
    std::cout << "--------------------------------\n\n";
    
    generateDiagnosticScript();
    
    return 0;
}