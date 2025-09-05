Key Observations

- TT design: single-slot (direct-mapped) table with 16-byte TTEntry keyed by key32 (upper 32 bits), index = (key * 0x9E3779B97F4A7C15) & mask (src/core/
transposition_table.*).
- Probe/store usage: returns on TT when depth sufficient; stores at node end with correct bound classification and mate adjustments; quiescence uses TT only at depth 0
(src/search/negamax.cpp, src/search/quiescence.cpp).
- Generation: newSearch() is called in searchIterativeTest (used by UCI go) before ID loop; not called in the legacy search variant (but UCI uses the former).
- Replacement policy: conservative on collisions (needs depth > entry.depth+2 or entry old and ≥ depth). Same-key updates require depth ≥ entry.depth (or within 2 ply if
old gen).
- Collisions metric: counted only in store path and only when !isEmpty() && entry->key32 != key32 before making replacement decision. No probe-side accounting.
- Critical stores missing: several early-return pruning paths (notably null-move and static null/reverse futility) return without storing to TT in the main search. Razoring
does store.

Why Hit Rate Can Decrease With Depth

- Single-slot table thrashes under heavy branching: deeper iterations touch more unique indices, displacing shallower, still-useful nodes. Conservative collision
replacement exacerbates this.
- Missing stores for prolific cutoffs (null move, static null) deny future reuse precisely where deeper searches would benefit most.
- Aspiration failures and re-searches churn the TT with inconsistent bounds/windows; with direct mapping this yields more displacement.

Why “0 Collisions” Is Reported

- Collisions are only incremented on store, not probe. Many mismatches are encountered on probe and never counted.
- The current collision counter increments but your test scripts may read a different counter; also, your doc mentions “120% hashfull”, but TranspositionTable::hashfull()
returns a permille 0..1000 from current generation only, so the 120% likely comes from a separate metric or misinterpretation worth reconciling.

High-Impact Next Steps (Prioritized)

- TT Store Coverage:
    - Add TT store on null-move fail-high (both verification pass and the shallow “no verify” branch). Store a LOWER bound with NO_MOVE at the current depth.
    - Add TT store on static null (reverse futility) early return (UPPER bound) with NO_MOVE.
    - Ensure all other early-return prunes that finalize a bound store an entry (you already do so for razoring and qsearch fail-high/final).
- TT Collision Handling:
    - Add probe-side collision/mismatch metrics: count how often the probed slot is non-empty yet key32 mismatches (and how often it’s empty).
    - Record store-skip reasons in the TT itself (collision but shallow; same-key but depth smaller; old gen grace) with counters to quantify current policy effects at
scale.
- Replacement Policy:
    - Short term: relax collision replacement to replace if depth >= entry->depth - 1 or if entry is old gen (regardless of +2 threshold). This reduces thrash from shallow
entries persisting over deeper ones.
    - Medium term: move to 2-way or 4-way set-associative buckets within a cache line:
    - Probe: check 2–4 slots for key match.
    - Store: pick victim by a score function like `victimScore = entry->depth + ageBias` and replace the worst; or prefer empty/oldest/shallower entries.
    - This is the standard fix for direct-map TT collision pathologies.
- Search Stability Diagnostics:
    - Per-iteration “reuse” metric: percent of TT hits where ttEntry->depth >= currentDepth-1. Add to IterationInfo and UCI info.
    - Aspiration window telemetry is present; compare runs with aspiration disabled to see if hit rate still decreases with depth. If it stabilizes, tune aspiration growth
or widen initial window.
    - PV stability: you already track move changes and stability; correlate with TT hit rate.
- TT Entry Quality:
    - On same-key store, update even when depth equal but bound “improves” (e.g., UPPER→EXACT, LOWER→EXACT, or tighter score). Don’t require strictly deeper for same key.
    - Consider refreshing generation on probe-hit for LRU behavior in a future 2–4 way design.
- Metrics Clarification:
    - Align your “hashfull/occupancy” reporting with UCI semantics (permille). Keep a separate “fill rate” metric including old generations. Make sure scripts interpret
both correctly.
    - Validate collision counters after probe-side instrumentation; verify nonzero at high occupancy.

Concrete Code Touchpoints

- Add stores:
    - src/search/negamax.cpp
    - Null-move cutoff return paths around: after `if (nullScore >= beta)` block; also in the shallow trust branch inside that block. Store LOWER with `NO_MOVE`, with mate
adjust.
    - Static null (reverse futility) early return around where you do `return staticEval - margin;` → store UPPER with `NO_MOVE`.
- Improve replacement now (if you want a quick iteration):
    - src/core/transposition_table.cpp::store — in the else (collision) branch: relax thresholds as above or add a simple age bias. Keep it reversible for testing.
- Add diagnostics:
    - Probe-side mismatch counters in TranspositionTable::probe (e.g., probeNonEmptyMismatches++, probeEmpties++).
    - Store-skip reason counters in TranspositionTable::store (e.g., skipCollisionTooShallow++, skipSameKeyShallower++, skipOldGenGrace++).
    - Per-iteration reuse metric in IterativeSearchData based on ttDepth captured on hit.

Experiments To Run

- Toggle stores:
    - A/B: with/without TT store on null-move cutoff + static-null return. Expect TT hit rate to increase and node count to drop, especially at deeper depths.
- Replacement policy:
    - A/B: current vs. relaxed collision replace.
    - A/B: single-slot vs. 2-way (if implemented). Expect collisions >0 and hit rate trend to increase with depth.
- Aspiration:
    - A/B: disable aspiration windows. If hit rate trend stops decreasing, tune initial window/growth.
- Capacity sanity:
    - Re-run your test_tt_sizes.sh; with probe-side collision metrics you should now see nonzero collisions at high fill.

Longer-Term Improvements

- Implement 4-way set-associative TT with in-line cluster (fits 64-byte line: 4 x 16B entries), age refresh on hit, and replacement by “lowest age/depth score”.
- Consider separate buckets or flags for qsearch entries vs main search, to avoid overwriting deeper main-search info with depth-0 quiescence entries.
- Track and utilize TT move reliability (cutoff rate) to influence ordering and replacement weighting.
