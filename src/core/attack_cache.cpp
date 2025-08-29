#include "attack_cache.h"

namespace seajay {

// Thread-local attack cache instance
// Each thread gets its own cache with zero synchronization overhead
thread_local AttackCache t_attackCache;

} // namespace seajay