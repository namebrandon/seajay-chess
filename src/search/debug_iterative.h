#pragma once

// Debug infrastructure for Stage 13: Iterative Deepening
// These macros are compiled out in release builds

#include "../evaluation/types.h"
#include "../core/types.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace seajay::search {

// Forward declarations
struct IterationInfo;

// Debug output functions (only compiled in debug builds)
#ifdef TRACE_ITERATIVE_DEEPENING

inline void logIteration(const IterationInfo& info);
inline void logWindow(eval::Score alpha, eval::Score beta);
inline void logTime(std::chrono::milliseconds used, std::chrono::milliseconds limit);
inline void logStability(Move move, int count, bool changed);
inline void logEBF(double ebf, uint64_t nodes);

// Trace macros for debug builds
#define TRACE_ITERATION(info) logIteration(info)
#define TRACE_WINDOW(alpha, beta) logWindow(alpha, beta)
#define TRACE_TIME(used, limit) logTime(used, limit)
#define TRACE_STABILITY(move, count, changed) logStability(move, count, changed)
#define TRACE_EBF(ebf, nodes) logEBF(ebf, nodes)

// Implementation of logging functions
inline void logIteration(const IterationInfo& info) {
    std::cerr << "[ITER] Depth " << info.depth 
              << ": score=" << info.score.value()
              << " nodes=" << info.nodes
              << " time=" << info.elapsed
              << "ms";
    if (info.failedHigh) std::cerr << " FAIL_HIGH";
    if (info.failedLow) std::cerr << " FAIL_LOW";
    std::cerr << std::endl;
}

inline void logWindow(eval::Score alpha, eval::Score beta) {
    std::cerr << "[WINDOW] [" << alpha.value() 
              << ", " << beta.value() << "]" 
              << " width=" << (beta - alpha).value()
              << std::endl;
}

inline void logTime(std::chrono::milliseconds used, std::chrono::milliseconds limit) {
    auto percent = (limit.count() > 0) ? (100 * used.count() / limit.count()) : 0;
    std::cerr << "[TIME] Used " << used.count() << "ms"
              << " of " << limit.count() << "ms"
              << " (" << percent << "%)"
              << std::endl;
}

inline void logStability(Move move, int count, bool changed) {
    std::cerr << "[STABILITY] Move " << move.to_string()
              << " count=" << count;
    if (changed) std::cerr << " CHANGED";
    else std::cerr << " STABLE";
    std::cerr << std::endl;
}

inline void logEBF(double ebf, uint64_t nodes) {
    std::cerr << "[EBF] Effective branching factor=" 
              << std::fixed << std::setprecision(2) << ebf
              << " (nodes=" << nodes << ")"
              << std::endl;
}

#else

// No-op macros for release builds
#define TRACE_ITERATION(info) ((void)0)
#define TRACE_WINDOW(alpha, beta) ((void)0)
#define TRACE_TIME(used, limit) ((void)0)
#define TRACE_STABILITY(move, count, changed) ((void)0)
#define TRACE_EBF(ebf, nodes) ((void)0)

#endif // TRACE_ITERATIVE_DEEPENING

// Assertion macros for iterative deepening invariants
#ifdef DEBUG

#define ASSERT_WINDOW_VALID(alpha, beta) \
    do { \
        assert(alpha < beta && "Invalid search window"); \
        assert(alpha >= eval::Score::minus_infinity() && "Alpha out of bounds"); \
        assert(beta <= eval::Score::infinity() && "Beta out of bounds"); \
    } while(0)

#define ASSERT_ITERATION_VALID(info) \
    do { \
        assert(info.depth > 0 && info.depth <= 64 && "Invalid depth"); \
        assert(info.nodes > 0 && "No nodes searched"); \
        assert(info.windowAttempts >= 0 && "Invalid attempt count"); \
    } while(0)

#else

#define ASSERT_WINDOW_VALID(alpha, beta) ((void)0)
#define ASSERT_ITERATION_VALID(info) ((void)0)

#endif // DEBUG

} // namespace seajay::search