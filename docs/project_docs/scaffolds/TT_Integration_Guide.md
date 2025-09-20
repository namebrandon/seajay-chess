Title: TranspositionTable – Clustered Backend Integration Guide

Decision
- Integrate 4‑way clustered behavior directly into TranspositionTable with a runtime switch. Keep the public type and pointer semantics unchanged so all search code continues to take TranspositionTable*.

Rationale
- Avoids templating or virtual interfaces in hot paths.
- Preserves ABI and signatures; minimal churn to search, UCI, and tests.
- Keeps probe/store calls inlined and branch‑predictable.

Implementation Steps (do in this order)
1) Add clustering configuration
   - Fields:
     - bool m_clustered = false;
     - static constexpr size_t CLUSTER_SIZE = 4;
   - Methods:
     - void setClustered(bool on) { m_clustered = on; }
     - bool isClustered() const { return m_clustered; }
   - Note: Ensure this flag is not toggled mid‑search.

2) Resize with clusters
   - calculateNumEntries(sizeInMB): when m_clustered, ensure the final entry count is a power of two AND a multiple of CLUSTER_SIZE.
   - Keep contiguous TTEntry* m_entries; a cluster is 4 contiguous TTEntry (64 bytes total).

3) Indexing helpers
   - size_t raw = mix(key) & m_mask;
   - size_t clusterStart = m_clustered ? (raw & ~(CLUSTER_SIZE - 1)) : raw;

4) probe(key)
   - If m_clustered:
     - Scan entries [clusterStart .. clusterStart+3]; return pointer on key32 match.
     - Update stats: probes++, hits++, probeEmpties (no entry had genBound!=0), probeMismatches otherwise.
   - Else: keep current single‑slot logic.

5) prefetch(key)
   - Prefetch &m_entries[clusterStart] (entire cluster line) when clustered; otherwise current prefetch.

6) store(key, move, score, eval, depth, bound)
   - If m_clustered:
     - First pass: same‑key update preferring deeper depth and move‑carrying entries.
     - Else choose victim by policy:
       1) Empty
       2) Old generation (entry.generation() != m_generation)
       3) Shallower depth
       4) Non‑EXACT bound
       5) NO_MOVE entries
       6) Oldest among equals (round‑robin or first‑fit is fine initially)
     - Write via entry.save(k32, move, score, eval, depth, bound, m_generation) in one shot.
   - Else: keep existing single‑slot replacement logic; retain protections against NO_MOVE pollution.

7) hashfull/fillRate
   - Sample 1000 clusters: for i in [0..999], idx = (i * m_numEntries) / (1000*CLUSTER_SIZE); scan the cluster and count entries with gen==m_generation. Convert to 0..1000 permil like current code.

8) Stats (optional but useful)
   - Keep existing TTStats intact.
   - Optionally add: clusterScanLen (avg entries scanned), victimReason counters. Gate printing from UCI debug path to avoid breaking existing output.

9) UCI toggle wiring
   - In handleSetOption("UseClusteredTT"):
     - If search is active, defer to after stop.
     - m_tt.setClustered(on);
     - m_tt.resize(current Hash MB); (or a reinit() that rebuilds with same size)
     - Log: info string Clustered TT enabled/disabled

10) Deprecate wrapper in hot path
   - Remove TranspositionTableWrapper from the engine path.
   - Keep your clustered_transposition_table.* as a reference or fold useful code into private helpers.

Thread‑Safety / LazySMP Notes
- Shared TT across threads, no locks. Writes are benign races; generation increments on newSearch() from controller thread.
- Align clusters to 64B; probe/store access stays within a cache line.
- Do not resize while searching; require stop or ucinewgame.

Validation Checklist
- Build OK with both UseClusteredTT=false/true.
- Unit tests: probe/store semantics, replacement preferences, resize/clear behavior.
- Perft unchanged; tactical suite unchanged or improved.
- Depth vs time (tools/depth_vs_time.py): fewer nodes, higher TT hit rate when clustered.
- UCI debug tt: verify hit%, collisions, and any cluster stats look sane.

Rollback
- Toggle UseClusteredTT=false to revert to single-entry mode without rebuilding code.

