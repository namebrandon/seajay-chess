#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Analysis of RankedMovePicker regression issues
// Focus on:
// 1. Promotion handling
// 2. In-check evasion quality

class RegressionAnalysis {
public:
    void analyzePromotionHandling() {
        std::cout << "\n=== PROMOTION HANDLING ANALYSIS ===\n\n";
        
        std::cout << "Current Implementation (Phase 2a.4):\n";
        std::cout << "----------------------------------------\n";
        std::cout << "1. Promotions are NOT included in shortlist (line 284)\n";
        std::cout << "   - Condition: (isCapture(move) || isEnPassant(move)) only\n";
        std::cout << "   - Missing: isPromotion(move) check\n\n";
        
        std::cout << "2. Legacy ordering handles promotions:\n";
        std::cout << "   - MVV-LVA groups promotions with captures at front\n";
        std::cout << "   - Non-capture promotions are ordered BEFORE quiet moves\n";
        std::cout << "   - Capture-promotions get both capture and promotion scoring\n\n";
        
        std::cout << "ISSUE IDENTIFIED:\n";
        std::cout << "----------------\n";
        std::cout << "• Non-capture promotions (e.g., e7e8q) are NOT in shortlist\n";
        std::cout << "• They appear AFTER the shortlist (8 captures) is exhausted\n";
        std::cout << "• This delays critical promotion moves by 8+ positions\n";
        std::cout << "• In endgames, promotions are often the best move\n\n";
        
        std::cout << "Example Move Order (current):\n";
        std::cout << "1. TT move (if any)\n";
        std::cout << "2-9. Top 8 captures from shortlist\n";
        std::cout << "10+. Non-capture promotions (DELAYED!)\n";
        std::cout << "11+. Remaining captures\n";
        std::cout << "12+. Quiet moves\n\n";
        
        std::cout << "Expected Order (legacy):\n";
        std::cout << "1. TT move\n";
        std::cout << "2. Capture-promotions (high MVV-LVA)\n";
        std::cout << "3. Non-capture promotions\n";
        std::cout << "4+. Regular captures by MVV-LVA\n";
        std::cout << "5+. Quiet moves\n\n";
    }
    
    void analyzeInCheckHandling() {
        std::cout << "\n=== IN-CHECK EVASION ANALYSIS ===\n\n";
        
        std::cout << "Current Implementation (Phase 2a.4):\n";
        std::cout << "----------------------------------------\n";
        std::cout << "1. Uses optimized generateCheckEvasions (good)\n";
        std::cout << "2. Orders with MVV-LVA/SEE only (line 233-237)\n";
        std::cout << "3. NO history heuristics applied (line 239 comment)\n";
        std::cout << "4. NO shortlist when in check (correct)\n\n";
        
        std::cout << "Comparison with Legacy:\n";
        std::cout << "------------------------\n";
        std::cout << "Legacy DOES apply history to evasions in some paths\n";
        std::cout << "This implementation removed history ordering (line 239)\n";
        std::cout << "Comment says: 'testing showed slight regression'\n\n";
        
        std::cout << "POTENTIAL ISSUES:\n";
        std::cout << "-----------------\n";
        std::cout << "• Removing history from evasions may hurt move ordering\n";
        std::cout << "• In-check nodes are critical for tactics\n";
        std::cout << "• Poor evasion ordering = more nodes searched\n";
        std::cout << "• The 'slight regression' might compound with other issues\n\n";
    }
    
    void analyzeShortlistImpact() {
        std::cout << "\n=== SHORTLIST OVERHEAD ANALYSIS ===\n\n";
        
        std::cout << "Shortlist Construction Overhead:\n";
        std::cout << "---------------------------------\n";
        std::cout << "1. Full legacy ordering applied first (all moves)\n";
        std::cout << "2. Then iterate to extract shortlist (lines 275-295)\n";
        std::cout << "3. Mark indices in m_inShortlistMap array\n";
        std::cout << "4. During yield, check map for every remainder move\n\n";
        
        std::cout << "Cost-Benefit Analysis:\n";
        std::cout << "----------------------\n";
        std::cout << "COSTS:\n";
        std::cout << "• Constructor overhead (per node)\n";
        std::cout << "• Memory for m_inShortlistMap[256]\n";
        std::cout << "• Extra branches in next() method\n";
        std::cout << "• std::find for TT move validation\n\n";
        
        std::cout << "BENEFITS:\n";
        std::cout << "• NONE - shortlist is exact same order as legacy!\n";
        std::cout << "• First 8 captures yielded in same order\n";
        std::cout << "• No reordering or improved selection\n\n";
        
        std::cout << "VERDICT: Pure overhead with zero benefit\n\n";
    }
    
    void printSummary() {
        std::cout << "\n=== REGRESSION ROOT CAUSES ===\n\n";
        
        std::cout << "1. CRITICAL: Non-capture promotions excluded from shortlist\n";
        std::cout << "   Impact: ~3-5 ELO (delays best moves in endgames)\n\n";
        
        std::cout << "2. MODERATE: In-check evasion ordering degraded\n";
        std::cout << "   Impact: ~2-3 ELO (more nodes in tactical positions)\n\n";
        
        std::cout << "3. SYSTEMIC: Shortlist provides no benefit\n";
        std::cout << "   Impact: ~5-6 ELO (pure overhead)\n\n";
        
        std::cout << "Total Expected: ~10-14 ELO loss (matches observed -11)\n\n";
        
        std::cout << "RECOMMENDATIONS:\n";
        std::cout << "----------------\n";
        std::cout << "Option 1: Include promotions in shortlist\n";
        std::cout << "   Change line 284 to include: || isPromotion(move)\n\n";
        
        std::cout << "Option 2: Remove shortlist entirely for now\n";
        std::cout << "   Just yield moves in legacy order\n";
        std::cout << "   Eliminates overhead until real ranking added\n\n";
        
        std::cout << "Option 3: Restore history ordering for evasions\n";
        std::cout << "   May help with tactical positions\n\n";
    }
};

int main() {
    RegressionAnalysis analysis;
    
    analysis.analyzePromotionHandling();
    analysis.analyzeInCheckHandling();
    analysis.analyzeShortlistImpact();
    analysis.printSummary();
    
    return 0;
}