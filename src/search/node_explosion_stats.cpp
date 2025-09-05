#include "node_explosion_stats.h"

namespace seajay::search {

// Thread-local instance for thread safety with UCI interactive analysis
thread_local NodeExplosionStats g_nodeExplosionStats;

} // namespace seajay::search