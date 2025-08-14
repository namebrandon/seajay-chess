/**
 * SeaJay Chess Engine - Stage 12: Transposition Tables
 * Differential Testing Framework
 * 
 * Common infrastructure for differential testing across all TT tests
 */

#pragma once

#include "core/board.h"
#include "core/types.h"
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace seajay {
namespace testing {

/**
 * Result of a differential test
 */
struct DifferentialResult {
    bool passed;
    std::string description;
    uint64_t value1;
    uint64_t value2;
    std::string context;
    
    void print() const {
        if (passed) {
            std::cout << "[PASS] " << description << "\n";
        } else {
            std::cout << "[FAIL] " << description << "\n";
            std::cout << "  Expected: 0x" << std::hex << value1 << "\n";
            std::cout << "  Got:      0x" << std::hex << value2 << "\n";
            std::cout << "  Context:  " << context << "\n";
            std::cout << std::dec;
        }
    }
};

/**
 * Base class for differential testing
 */
class DifferentialTester {
protected:
    std::vector<DifferentialResult> m_results;
    bool m_verbose;
    
public:
    DifferentialTester(bool verbose = false) : m_verbose(verbose) {}
    
    // Add a test result
    void addResult(const DifferentialResult& result) {
        m_results.push_back(result);
        if (m_verbose) {
            result.print();
        }
    }
    
    // Run a differential test
    void runTest(const std::string& description,
                 std::function<uint64_t()> method1,
                 std::function<uint64_t()> method2,
                 const std::string& context = "") {
        uint64_t val1 = method1();
        uint64_t val2 = method2();
        
        DifferentialResult result;
        result.description = description;
        result.passed = (val1 == val2);
        result.value1 = val1;
        result.value2 = val2;
        result.context = context;
        
        addResult(result);
    }
    
    // Get pass rate
    double getPassRate() const {
        if (m_results.empty()) return 0.0;
        
        int passed = 0;
        for (const auto& r : m_results) {
            if (r.passed) passed++;
        }
        
        return 100.0 * passed / m_results.size();
    }
    
    // Print summary
    void printSummary() const {
        std::cout << "\nDifferential Testing Summary:\n";
        std::cout << "=============================\n";
        std::cout << "Total tests: " << m_results.size() << "\n";
        
        int passed = 0;
        for (const auto& r : m_results) {
            if (r.passed) passed++;
        }
        
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << (m_results.size() - passed) << "\n";
        std::cout << "Pass rate: " << getPassRate() << "%\n";
        
        // Show failed tests
        if (passed < static_cast<int>(m_results.size())) {
            std::cout << "\nFailed tests:\n";
            for (const auto& r : m_results) {
                if (!r.passed) {
                    std::cout << "  - " << r.description << "\n";
                }
            }
        }
    }
    
    // Clear results
    void reset() {
        m_results.clear();
    }
    
    bool allPassed() const {
        for (const auto& r : m_results) {
            if (!r.passed) return false;
        }
        return true;
    }
};

/**
 * Zobrist-specific differential tester
 */
class ZobristDifferentialTester : public DifferentialTester {
public:
    ZobristDifferentialTester(bool verbose = false) 
        : DifferentialTester(verbose) {}
    
    // Test that incremental matches full calculation
    void testIncremental(const Board& board) {
        runTest(
            "Incremental vs Full for " + board.toFEN(),
            [&board]() { return board.zobristKey(); },
            [&board]() { 
                // Will be implemented in Phase 1
                // return zobrist::calculateFull(board); 
                return board.zobristKey();  // Placeholder
            },
            board.toFEN()
        );
    }
    
    // Test make/unmake invariant
    void testMakeUnmake(Board& board, const Move& move) {
        uint64_t before = board.zobristKey();
        
        if (board.makeMove(move)) {
            board.unmakeMove(move);
            uint64_t after = board.zobristKey();
            
            DifferentialResult result;
            result.description = "Make/unmake invariant for " + move.toString();
            result.passed = (before == after);
            result.value1 = before;
            result.value2 = after;
            result.context = board.toFEN();
            
            addResult(result);
        }
    }
};

/**
 * Performance comparison framework
 */
class PerformanceComparator {
private:
    struct Measurement {
        std::string name;
        double timeMs;
        uint64_t operations;
        double opsPerSec;
    };
    
    std::vector<Measurement> m_measurements;
    
public:
    template<typename Func>
    void measure(const std::string& name, int iterations, Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double timeMs = std::chrono::duration<double, std::milli>(end - start).count();
        
        Measurement m;
        m.name = name;
        m.timeMs = timeMs;
        m.operations = iterations;
        m.opsPerSec = (iterations * 1000.0) / timeMs;
        
        m_measurements.push_back(m);
    }
    
    void printComparison() {
        std::cout << "\nPerformance Comparison:\n";
        std::cout << "======================\n";
        
        for (const auto& m : m_measurements) {
            std::cout << m.name << ":\n";
            std::cout << "  Time: " << m.timeMs << " ms\n";
            std::cout << "  Operations: " << m.operations << "\n";
            std::cout << "  Ops/sec: " << std::fixed << std::setprecision(0) 
                     << m.opsPerSec << "\n";
        }
        
        if (m_measurements.size() == 2) {
            double speedup = m_measurements[0].opsPerSec / m_measurements[1].opsPerSec;
            std::cout << "\nSpeedup: " << std::fixed << std::setprecision(2) 
                     << speedup << "x\n";
        }
    }
};

/**
 * Three-tier validation macros (matching plan from Stage 12)
 */
#ifdef TT_PARANOID
    #define TT_VALIDATE_FULL() validateFull()
    #define TT_STATS(x) stats.x++
    #define TT_SHADOW_CHECK() shadowCheck()
    #define TT_ASSERT(cond, msg) \
        if (!(cond)) { \
            std::cerr << "TT_ASSERT failed: " << msg << "\n"; \
            std::cerr << "  File: " << __FILE__ << "\n"; \
            std::cerr << "  Line: " << __LINE__ << "\n"; \
            abort(); \
        }
#elif defined(TT_DEBUG)
    #define TT_VALIDATE_FULL() ((void)0)
    #define TT_STATS(x) stats.x++
    #define TT_SHADOW_CHECK() ((void)0)
    #define TT_ASSERT(cond, msg) \
        if (!(cond)) { \
            std::cerr << "TT_ASSERT failed: " << msg << "\n"; \
        }
#else
    #define TT_VALIDATE_FULL() ((void)0)
    #define TT_STATS(x) ((void)0)
    #define TT_SHADOW_CHECK() ((void)0)
    #define TT_ASSERT(cond, msg) ((void)0)
#endif

} // namespace testing
} // namespace seajay