#pragma once

#include <cstdint>
#include <array>
#include <algorithm>
#include "../core/types.h"  // For PieceType definitions

namespace seajay::search {

// Node explosion diagnostic statistics
// This is a temporary diagnostic structure to identify root causes of excessive node counts
struct NodeExplosionStats {
    // Depth distribution analysis
    struct DepthDistribution {
        std::array<uint64_t, 32> nodesAtDepth = {0};      // Nodes searched at each depth/ply
        std::array<uint64_t, 32> qsearchAtDepth = {0};    // Qsearch nodes at each depth
        uint64_t maxDepthReached = 0;                      // Maximum ply reached
        
        void recordNode(int ply, bool isQsearch) {
            if (ply < 32) {
                nodesAtDepth[ply]++;
                if (isQsearch) qsearchAtDepth[ply]++;
                maxDepthReached = std::max(maxDepthReached, static_cast<uint64_t>(ply));
            }
        }
        
        double qsearchRatio(int ply) const {
            if (ply >= 32 || nodesAtDepth[ply] == 0) return 0.0;
            return 100.0 * qsearchAtDepth[ply] / nodesAtDepth[ply];
        }
        
        // Calculate effective branching factor between plies
        double ebfBetween(int ply1, int ply2) const {
            if (ply1 >= 32 || ply2 >= 32 || ply2 <= ply1) return 0.0;
            if (nodesAtDepth[ply1] == 0 || nodesAtDepth[ply2] == 0) return 0.0;
            return static_cast<double>(nodesAtDepth[ply2]) / nodesAtDepth[ply1];
        }
    } depthDist;
    
    // Pruning effectiveness by depth
    struct PruningByDepth {
        // Futility pruning attempts and successes by depth
        std::array<uint64_t, 10> futilityAttempts = {0};
        std::array<uint64_t, 10> futilityPrunes = {0};
        
        // Move count pruning by depth
        std::array<uint64_t, 10> moveCountAttempts = {0};
        std::array<uint64_t, 10> moveCountPrunes = {0};
        
        // LMR by depth
        std::array<uint64_t, 10> lmrAttempts = {0};
        std::array<uint64_t, 10> lmrReductions = {0};
        std::array<uint64_t, 10> lmrReSearches = {0};
        
        void recordFutility(int depth, bool pruned) {
            if (depth > 0 && depth < 10) {
                futilityAttempts[depth]++;
                if (pruned) futilityPrunes[depth]++;
            }
        }
        
        void recordMoveCount(int depth, bool pruned) {
            if (depth > 0 && depth < 10) {
                moveCountAttempts[depth]++;
                if (pruned) moveCountPrunes[depth]++;
            }
        }
        
        void recordLMR(int depth, bool reduced, bool reSearched = false) {
            if (depth > 0 && depth < 10) {
                lmrAttempts[depth]++;
                if (reduced) {
                    lmrReductions[depth]++;
                    if (reSearched) lmrReSearches[depth]++;
                }
            }
        }
        
        double futilityRate(int depth) const {
            if (depth < 1 || depth >= 10 || futilityAttempts[depth] == 0) return 0.0;
            return 100.0 * futilityPrunes[depth] / futilityAttempts[depth];
        }
        
        double moveCountRate(int depth) const {
            if (depth < 1 || depth >= 10 || moveCountAttempts[depth] == 0) return 0.0;
            return 100.0 * moveCountPrunes[depth] / moveCountAttempts[depth];
        }
        
        double lmrRate(int depth) const {
            if (depth < 1 || depth >= 10 || lmrAttempts[depth] == 0) return 0.0;
            return 100.0 * lmrReductions[depth] / lmrAttempts[depth];
        }
    } pruningByDepth;
    
    // Quiescence search explosion analysis
    struct QSearchExplosion {
        uint64_t totalEntries = 0;              // Total quiescence search entries
        uint64_t standPatReturns = 0;           // Immediate returns from stand-pat
        uint64_t deltaPruneSkips = 0;           // Moves skipped by delta pruning
        uint64_t seePruneSkips = 0;             // Captures pruned by SEE
        uint64_t checksGenerated = 0;           // Check moves generated
        uint64_t checksSearched = 0;            // Check moves actually searched
        
        // Capture explosion tracking
        uint64_t capturesGenerated = 0;         // Total captures generated
        uint64_t capturesSearched = 0;          // Captures actually searched
        uint64_t badCapturesSearched = 0;       // QxP, RxP etc. searched
        
        // Depth tracking in qsearch
        std::array<uint64_t, 16> entriesAtPly = {0};  // Qsearch entries by ply depth
        
        void recordEntry(int qsPly) {
            totalEntries++;
            if (qsPly < 16) entriesAtPly[qsPly]++;
        }
        
        double standPatRate() const {
            return totalEntries > 0 ? 100.0 * standPatReturns / totalEntries : 0.0;
        }
        
        double captureSearchRate() const {
            return capturesGenerated > 0 ? 100.0 * capturesSearched / capturesGenerated : 0.0;
        }
        
        double checkSearchRate() const {
            return checksGenerated > 0 ? 100.0 * checksSearched / checksGenerated : 0.0;
        }
    } qsearchExplosion;
    
    // Move ordering failure analysis
    struct MoveOrderingFailure {
        // Track when good moves appear late
        uint64_t ttMoveNotFirst = 0;            // TT move existed but wasn't searched first
        uint64_t killerNotInTop3 = 0;           // Killer caused cutoff but wasn't in top 3
        uint64_t cutoffAfterMove10 = 0;         // Beta cutoff after move 10
        
        // Track bad capture ordering
        uint64_t qxpSearchedBeforeCutoff = 0;   // QxP searched when better move existed
        uint64_t rxpSearchedBeforeCutoff = 0;   // RxP searched when better move existed
        
        // Late move statistics
        std::array<uint64_t, 64> cutoffMoveIndex = {0};  // Which move index caused cutoff
        
        void recordCutoff(int moveIndex) {
            if (moveIndex < 64) cutoffMoveIndex[moveIndex]++;
            if (moveIndex >= 10) cutoffAfterMove10++;
        }
        
        double firstMoveRate() const {
            uint64_t total = 0;
            for (auto count : cutoffMoveIndex) total += count;
            return total > 0 ? 100.0 * cutoffMoveIndex[0] / total : 0.0;
        }
        
        double top3Rate() const {
            uint64_t total = 0, top3 = 0;
            for (int i = 0; i < 64; i++) {
                total += cutoffMoveIndex[i];
                if (i < 3) top3 += cutoffMoveIndex[i];
            }
            return total > 0 ? 100.0 * top3 / total : 0.0;
        }
    } moveOrderingFailure;
    
    // SEE-specific explosion tracking
    struct SEEExplosion {
        uint64_t seeCallsInQsearch = 0;         // SEE evaluations in quiescence
        uint64_t seeCallsInMain = 0;            // SEE evaluations in main search
        uint64_t seeFalsePositives = 0;         // SEE said winning but was losing
        uint64_t seeFalseNegatives = 0;         // SEE said losing but was winning
        uint64_t seeEqualExchanges = 0;         // SEE returned 0 (equal exchange)
        
        // Track expensive captures being searched
        uint64_t queenTakesPawn = 0;            // QxP moves searched
        uint64_t rookTakesPawn = 0;             // RxP moves searched
        uint64_t minorTakesPawn = 0;            // NxP, BxP moves searched
        
        void recordCapture(PieceType attacker, PieceType victim) {
            if (attacker == QUEEN && victim == PAWN) queenTakesPawn++;
            else if (attacker == ROOK && victim == PAWN) rookTakesPawn++;
            else if ((attacker == KNIGHT || attacker == BISHOP) && victim == PAWN) minorTakesPawn++;
        }
    } seeExplosion;
    
    // Reset all statistics
    void reset() {
        depthDist = {};
        pruningByDepth = {};
        qsearchExplosion = {};
        moveOrderingFailure = {};
        seeExplosion = {};
    }
    
    // Get a summary report
    void printSummary() const {
        // This will be implemented to print key findings
        // For now, data is collected for analysis
    }
};

// Global diagnostic stats (thread-local for thread safety with UCI)
extern thread_local NodeExplosionStats g_nodeExplosionStats;

} // namespace seajay::search