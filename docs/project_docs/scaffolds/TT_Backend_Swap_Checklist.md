Title: Clustered TT Backend Swap – Integration Checklist (Review Before Coding)

Purpose
Integrate a 4‑way clustered TT behind the existing public API with a runtime toggle, ensuring correctness and performance across single and multi-thread (LazySMP) modes.

Preconditions
- Current TT passes all unit tests.
- Baseline TT stats recorded (probes, hits, hashfull, mismatches) on representative search runs.

Design Alignment
- Header scaffold: src/experimental/tt/ClusteredTT.hpp mirrors API.
- Entry size remains 16B; cluster = 4 entries (64B), aligned.
- Generation aging: newSearch() increments 6-bit generation before starting threads.

Integration Steps
1) Introduce a compile‑time alias or runtime indirection in TranspositionTable to delegate to either backend:
   - Option A (preferred): add an internal variant that holds either single‑entry or clustered implementation but preserves the current public methods.
   - Option B: maintain two classes and a thin facade chosen at construction.
2) UCI option: UseClusteredTT (default off) selects clustered backend at engine init and on ‘setoption’ Hash resize events.
3) Ensure `resize()` maps MB to a power-of-two number of clusters (not entries); preserve hashfull semantics sampling 1000 clusters.
4) Maintain `prefetch(key)` to prefetch the cluster line.
5) Replacement policy order: empty → old gen → shallower depth → non‑EXACT → NO_MOVE → oldest; implement round‑robin tie-breaker if needed.

Thread‑Safety / LazySMP
- Shared TT across threads; no locks. Updates are best-effort and safe.
- Align clusters to 64B; avoid tearing by writing the entry via a single save() call.
- Generation modified only from main thread between searches.

Diagnostics & Telemetry
- Expose: probes, hits, stores, collisions, probeEmpties, probeMismatches.
- Optional histogram: average cluster scan length; victim reason counts.

Validation
- Unit: probe/store semantics, replacement preference tests, eviction tests, resize/clear behavior.
- Functional: perft identical; engine decisions reproducible under same seed/time.
- A/B: depth_vs_time.py & node_explosion_diagnostic.sh; expect ↑hit rate and ↓nodes.

Rollback
- UCI toggle reverts to legacy single-entry backend without code changes.

