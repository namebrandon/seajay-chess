#include "attack_cache.h"

namespace seajay {

// Thread-local attack cache instance
// Each thread gets its own cache with zero synchronization overhead
thread_local AttackCache t_attackCache;

// Phase 5.2: Thread-local control flags and statistics
// These are set by the search thread at the start of search
thread_local bool t_attackCacheEnabled = false;
thread_local bool t_attackCacheStatsEnabled = false;

// Phase 5.2: Thread-local statistics counters
thread_local uint64_t t_attackCacheHits = 0;
thread_local uint64_t t_attackCacheMisses = 0;
thread_local uint64_t t_attackCacheStores = 0;
thread_local uint64_t t_attackCacheTryProbes = 0;
thread_local uint64_t t_attackCacheTryHits = 0;
thread_local uint64_t t_attackCacheTryMisses = 0;
thread_local uint64_t t_attackCacheTryStores = 0;

} // namespace seajay
