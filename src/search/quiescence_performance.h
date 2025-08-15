#pragma once

// Phase 2.3 Performance Profiling Infrastructure
// Missing Item 3 from original Stage 14 plan

#include "../core/types.h"
#include "../evaluation/types.h"
#include <chrono>
#include <vector>
#include <string>

namespace seajay::search {

// Performance benchmark data for quiescence search
struct QuiescencePerformanceData {
    // Node count measurements
    uint64_t totalNodes = 0;
    uint64_t qsearchNodes = 0;
    uint64_t mainSearchNodes = 0;
    
    // Timing measurements  
    std::chrono::nanoseconds totalTime{0};
    std::chrono::nanoseconds qsearchTime{0};
    
    // Node ratio analysis
    double getQsearchRatio() const {
        return totalNodes > 0 ? static_cast<double>(qsearchNodes) / totalNodes : 0.0;
    }
    
    double getNodeIncrease() const {
        return mainSearchNodes > 0 ? static_cast<double>(qsearchNodes) / mainSearchNodes : 0.0;
    }
    
    // Performance metrics
    uint64_t getNodesPerSecond() const {
        auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count();
        return timeMs > 0 ? (totalNodes * 1000) / timeMs : 0;
    }
    
    uint64_t getQsearchNPS() const {
        auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(qsearchTime).count();
        return timeMs > 0 ? (qsearchNodes * 1000) / timeMs : 0;
    }
};

// Tactical test position for benchmarking
struct TacticalBenchmarkPosition {
    std::string fen;
    std::string description;
    int expectedDepth;          // Depth to search
    eval::Score expectedScore;  // Expected evaluation
    uint64_t expectedNodes;     // Expected total nodes (approximate)
    
    TacticalBenchmarkPosition(const std::string& f, const std::string& desc, 
                            int depth, int score, uint64_t nodes = 0)
        : fen(f), description(desc), expectedDepth(depth), 
          expectedScore(eval::Score(score)), expectedNodes(nodes) {}
};

// Performance benchmark suite for quiescence
class QuiescencePerformanceBenchmark {
public:
    // Standard tactical positions for quiescence testing
    static std::vector<TacticalBenchmarkPosition> getTacticalPositions();
    
    // Run performance benchmark on a single position
    static QuiescencePerformanceData benchmarkPosition(
        const std::string& fen, 
        int depth,
        bool enableQuiescence = true
    );
    
    // Run benchmark suite and generate report
    static void runFullBenchmark();
    
    // Compare performance with/without quiescence
    static void compareQuiescenceImpact();
    
    // Profile hot paths in quiescence search
    static void profileHotPaths();
    
    // Measure stack usage during deep quiescence
    static void measureStackUsage();
    
private:
    // Tactical positions from chess literature
    static void addStandardTacticalPositions(std::vector<TacticalBenchmarkPosition>& positions);
    
    // High-capture-density positions for stress testing
    static void addCaptureHeavyPositions(std::vector<TacticalBenchmarkPosition>& positions);
    
    // Promotion positions for move ordering tests
    static void addPromotionPositions(std::vector<TacticalBenchmarkPosition>& positions);
};

// RAII timer for measuring quiescence performance
class QuiescenceTimer {
private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::nanoseconds& m_duration;
    
public:
    explicit QuiescenceTimer(std::chrono::nanoseconds& duration) 
        : m_start(std::chrono::high_resolution_clock::now()), m_duration(duration) {}
    
    ~QuiescenceTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        m_duration += end - m_start;
    }
};

// Macro for easy timing of quiescence functions
#define QSEARCH_TIMER(duration) QuiescenceTimer timer(duration)

} // namespace seajay::search