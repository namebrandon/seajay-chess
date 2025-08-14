#pragma once

// Stage 13, Deliverable 5.2b: Optimize critical sections
// Performance optimizations for hot paths identified in profiling

#include <chrono>

namespace seajay::search {

// Cache time checks to avoid frequent system calls
class TimeCache {
private:
    mutable std::chrono::steady_clock::time_point m_lastCheck;
    mutable std::chrono::milliseconds m_cachedElapsed;
    mutable int m_checkCounter = 0;
    static constexpr int CHECK_INTERVAL = 1000;  // Check time every 1000 nodes
    std::chrono::steady_clock::time_point m_startTime;
    
public:
    TimeCache() : m_startTime(std::chrono::steady_clock::now()) {
        m_lastCheck = m_startTime;
        m_cachedElapsed = std::chrono::milliseconds(0);
    }
    
    // Get elapsed time with caching
    inline std::chrono::milliseconds getElapsed() const {
        if (++m_checkCounter >= CHECK_INTERVAL) {
            m_checkCounter = 0;
            auto now = std::chrono::steady_clock::now();
            m_cachedElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_startTime);
            m_lastCheck = now;
        }
        return m_cachedElapsed;
    }
    
    // Force update (for critical checks)
    inline std::chrono::milliseconds forceUpdate() const {
        m_checkCounter = 0;
        auto now = std::chrono::steady_clock::now();
        m_cachedElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_startTime);
        m_lastCheck = now;
        return m_cachedElapsed;
    }
};

// Inline frequently called small functions
#ifdef NDEBUG  // Release mode optimizations

// Force inline for hot path functions
#define ALWAYS_INLINE __attribute__((always_inline)) inline

// Remove debug output in release
#define DEBUG_LOG(x) ((void)0)

#else  // Debug mode

#define ALWAYS_INLINE inline
#define DEBUG_LOG(x) std::cerr << x << std::endl

#endif

// Optimization hints for the compiler
#if defined(__GNUC__) || defined(__clang__)
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define PREFETCH(addr) __builtin_prefetch(addr, 0, 3)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
    #define PREFETCH(addr) ((void)0)
#endif

// Fast node counter that avoids atomic operations in single-threaded context
class FastNodeCounter {
private:
    uint64_t m_count = 0;
    
public:
    ALWAYS_INLINE void increment() { ++m_count; }
    ALWAYS_INLINE void add(uint64_t n) { m_count += n; }
    ALWAYS_INLINE uint64_t get() const { return m_count; }
    ALWAYS_INLINE void reset() { m_count = 0; }
};

} // namespace seajay::search