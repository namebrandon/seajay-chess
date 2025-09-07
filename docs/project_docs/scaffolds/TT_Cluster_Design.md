Title: Clustered Transposition Table (4‑Way) – Design Scaffold (Thread‑Safe, LazySMP‑Ready)

Overview
- Replace single-entry TT slots with 4‑entry set-associative clusters (64B) to reduce collisions and increase hit rate.
- Keep 16‑byte TTEntry layout and generation+bound packing; cluster scans are cheap and cache‑friendly.

API Compatibility
- Preserve existing API shape: prefetch(key), probe(key) → TTEntry*, store(key, move, score, eval, depth, bound), resize(), clear(), newSearch().
- Added internals only; external call sites unchanged.

Thread‑Safety & LazySMP
- TT allows benign data races (standard practice). For LazySMP:
  - Each worker probes/stores in a shared TT without locks.
  - Writes are non-atomic per field but logically consistent due to entry save() being a single struct write on aligned 16B with generation updated last.
  - Generation increments via newSearch() from the main thread before spawning workers.
  - Optional: per‑thread “cluster preference offset” to reduce contention (not required initially).
- No per‑entry locks. Replacements are best‑effort; correctness unaffected.

Replacement Policy (victim selection)
1) Empty slot (genBound == 0)
2) Old generation entries
3) Shallower depth entries
4) Non‑EXACT bound entries
5) NO_MOVE entries (heuristics) over entries carrying moves
6) Oldest among equals (round‑robin within cluster)

Hashing & Indexing
- Keep current index(key) mask with 2^N entries.
- A “cluster” is 4 consecutive TTEntry elements starting at index(key & mask) & ~3.
- probe(): linearly scan 4 entries; return match on key32; collect stats (hits/misses, mismatches).
- prefetch(): prefetch first line only; cluster fits in a single 64B line.

Storage Semantics
- On store, compute victim and write entry.save(key32, move, score, eval, depth, bound, gen).
- Adjust mate scores by ply outside (unchanged behavior).
- Depth=0 allowed for qsearch and heuristic bounds.

Stats & Diagnostics
- probes, hits, stores, collisions, probeEmpties, probeMismatches.
- Optional: clusterScanLen histogram; replacement reason counters.

Migration Plan
- Implement a new clustered table backend behind the same public interface.
- UCI flag UseClusteredTT to switch between backends during A/B.
- Keep serialization/clear/hashfull semantics identical; hashfull samples still scan 1000 clusters.

Files (scaffold only; not compiled yet)
- src/experimental/tt/ClusteredTT.hpp – header‑only conceptual interface mirroring TranspositionTable.

Testing Strategy
- Unit tests to verify probe/store semantics, replacement choices, and hit rate under synthetic workloads.
- Perft and engine functional tests must be identical; only performance changes.

