# Phase 5: Lightweight Attack Cache - Executive Summary

## Mission Statement
Implement a small, fast cache for `isSquareAttacked()` results to reduce redundant computation in move validation, achieving 5-10% overhead reduction in legality-heavy positions without functional changes.

## Strategic Value
- **Performance Impact:** 5-10% reduction in move validation overhead
- **Depth Gain:** Contributes 0.1-0.2 ply at 1s/move when combined with Phases 1-4
- **Risk Level:** LOW - pure optimization with no chess logic changes
- **Rollback Capability:** UCI toggle for instant disable if needed

## Implementation Strategy: Small, Testable Phases

### Phase 5.1: Infrastructure Only (No Cache Usage)
**Deliverable:** Cache data structure and management code
**Changes:**
- Add `AttackCache` class with store/probe/clear methods implemented in a dedicated module (`attack_cache.{h,cpp}`) with no global state.
- Integrate one `AttackCache` instance into each `SearchData` / worker thread (future LazySMP friendly).
- Add UCI option `UseAttackCache` (default: false) plus telemetry counters (probes, hits, misses) exposed through `SearchStats`.
- Provide RAII helpers so search initialisation/`ucinewgame` can reset cache state deterministically.

**Testing:**
```bash
./build.sh Release
echo "bench" | ./bin/seajay  # Must match baseline exactly
```
**Expected:** bench 2136346 (unchanged)
**Commit:** "feat: Phase 5.1 - Add attack cache infrastructure (disabled)
bench 2136346"
**SPRT:** Not needed (no behavioral change)

---

### Phase 5.2: Cache Integration in isSquareAttacked() Only
**Deliverable:** Cache active in attack detection only
**Changes:**
- Modify `Board::isSquareAttacked()` and related helpers to consult the per-thread cache before recomputing.
- Cache entries store `(hash ^ kingSquare << 1, square, attackerColor)` → attacked flag; collisions fall back to the normal computation.
- Cache size: 256 entries per thread (≈4 KB) direct-mapped with simple masking for speed.
- Reset cache on `ucinewgame`, iteration start, and after bulk board state changes to prevent stale reuse.

**Testing:**
```bash
./build.sh Release
echo "bench" | ./bin/seajay  # Must match baseline exactly
./bin/perft_tool  # Verify all perft positions
```
**Expected:** bench 2136346 (unchanged - same moves explored)
**Commit:** "feat: Phase 5.2 - Enable attack cache in isSquareAttacked()
bench 2136346"
**SPRT:** [0.00, 5.00] at 10+0.1 (performance test)

---

### Phase 5.3: Extend to tryMakeMove() Validation
**Deliverable:** Cache used in move legality checks
**Changes:**
- Modify `Board::tryMakeMove()` (and any helper legality checks) to reuse the same per-thread cache API.
- Ensure cache writes happen after the board reaches the queried state; clear/refresh on `unmakeMove` to avoid stale king-squares.
- No change to cache size or policy; instrumentation extended to track lookups originating from `tryMakeMove`.

**Testing:**
```bash
./build.sh Release
echo "bench" | ./bin/seajay
./tools/tactical_test.py ./bin/seajay external/UHO_4060_v2.epd 2000
```
**Expected:** bench 2136346, tactical ≥85% (cache on/off A/B)
**Commit:** "feat: Phase 5.3 - Extend attack cache to move validation
bench 2136346"
**SPRT:** [0.00, 5.00] at 10+0.1

---

### Phase 5.4: Cache Size Tuning
**Deliverable:** Optimal cache size based on hit rate analysis
**Changes:**
- Test cache sizes: 128, 256, 512, 1024 entries (per thread) and record hit/miss telemetry.
- Add UCI option `AttackCacheSize` (default: best from testing) with validation that enforces power-of-two sizes ≤2048 entries.
- If hit rate < 80%, prototype a 4-way set associative variant while keeping the caller API unchanged.

**Local A/B Testing Protocol:**
```bash
# For each size
./build.sh Release
echo "setoption name AttackCacheSize value [SIZE]" | ./bin/seajay
./tools/depth_vs_time.py --time-ms 1000 --engine ./bin/seajay --uci-option "UseAttackCache true"
# Compare depth achieved and hit rates
```
**Commit:** "feat: Phase 5.4 - Optimize attack cache size to [SIZE]
bench 2136346"
**SPRT:** [0.00, 5.00] at 10+0.1

---

### Phase 5.5: Telemetry and Final Tuning
**Deliverable:** Production-ready cache with diagnostics
**Changes:**
- Add `debug attackcache` UCI command for hit/miss stats (and optional CSV export) to simplify profiling.
- Implement cache aging if needed (clear every N moves or on depth rollbacks) to avoid stale data in long searches.
- Ensure the cache stays lock-free; remove any debug atomics or logging from the hot path.
- Flip default `UseAttackCache=true` only after a PASSing SPRT; keep toggle available for emergency rollback.

**Testing:**
```bash
# Full validation suite
./build.sh Release
echo "bench" | ./bin/seajay
./tools/tactical_test.py ./bin/seajay external/UHO_4060_v2.epd 2000
./tools/node_explosion_diagnostic.sh
./tools/depth_vs_time.py --time-ms 3000 --engine ./bin/seajay --uci-option "UseAttackCache true"
echo "setoption name UseAttackCache value false" | ./bin/seajay  # sanity bench without cache
```
**Commit:** "feat: Phase 5.5 - Finalize attack cache with diagnostics
bench 2136346"
**SPRT:** [0.00, 8.00] at 10+0.1 (final validation)

---

## Risk Mitigation

### Rollback Points
Each phase can be independently reverted:
- 5.1: Remove infrastructure (no impact)
- 5.2: Leave `UseAttackCache=false` (default) so the cache is inert
- 5.3: Revert `tryMakeMove` changes to restrict cache usage to `isSquareAttacked`
- 5.4: Reset to baseline size (256) or force direct-mapped mode
- 5.5: Disable via UCI or revert the default flip if regression discovered

### Validation Gates
Before each commit:
1. ✅ Bench count must match 2136346 (cache on/off where applicable)
2. ✅ Perft must pass (move generation unchanged)
3. ✅ Tactical suite ≥85% with cache enabled; spot-check OFF for parity when relevant
4. ✅ No compiler warnings or sanitizer hits
5. ✅ Push to remote immediately

### Performance Monitoring
Track per phase:
- NPS change (should increase 2-5% per phase)
- Cache hit rate (target >80%)
- Time spent in `isSquareAttacked` / `tryMakeMove`
- Depth achieved at 1s (should increase 0.1-0.2)
- Thread-local memory overhead (≤8 KB per worker)

## Success Criteria

### Phase Complete When:
- All 5 sub-phases pass SPRT
- Combined NPS improvement ≥5%
- Cache hit rate ≥80%
- Tactical strength maintained
- No memory leaks or crashes

### Decision Points:
- If Phase 5.2 shows regression → investigate cache overhead
- If Phase 5.3 shows no improvement → cache may be too small
- If Phase 5.4 shows diminishing returns → stop at optimal size
- If hit rate <60% → implement set-associative design

## Timeline Estimate
- Phase 5.1: 2 hours (infrastructure)
- Phase 5.2: 3 hours (integration + testing)
- Phase 5.3: 2 hours (extension)
- Phase 5.4: 4 hours (A/B testing)
- Phase 5.5: 2 hours (cleanup)
- **Total:** 13 hours + SPRT wait times

## Expected Outcomes
- **Primary:** 5-10% overhead reduction in attack calculations
- **Secondary:** 0.1-0.2 additional ply at 1s time control
- **Combined with Phases 1-4:** Achieve depth parity target (within 0.5-1 of competitors)
- **OpenBench:** Non-regression with potential +3-5 Elo from efficiency; `UseAttackCache` toggle remains available for emergency disable

## Communication Protocol
After each sub-phase:
```
=== PHASE 5.[X] COMPLETE ===
Branch: feature/20250918-phase5-attack-cache
Commit: [SHA]
Bench: 2136346
NPS Change: +[X]%
Cache Hit Rate: [X]%
Ready for OpenBench: YES

SPRT Configuration:
Bounds: [0.00, 5.00]
Time Control: 10+0.1
Expected: Non-regression with performance gain
```
