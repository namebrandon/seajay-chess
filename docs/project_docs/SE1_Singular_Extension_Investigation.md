# SE1 Singular Extension Investigation

## Context Snapshot
- Verification nodes now ignore primary EXACT TT entries while still honoring verification-tagged entries; the new counters (`verificationNodes*`) document the control flow (`src/search/types.h:360`, `src/search/negamax.cpp:399`).
- Instrumentation plus `BypassSingularTTExact` (debug hook) let us reproduce legacy behaviour on demand, but the default build already bypasses non-verification TT EXACT cutoffs.
- Singular margins currently use {64,56,48,40,36,32 cp} per depth bucket (0‑3, 4‑5, 6‑7, 8‑9, 10‑11, ≥12) based on initial slack telemetry.
- Goal: quantify the impact of the new TT rules/margins, ensure caching works for repeated verifications, and only then evaluate verification reduction and check-extension interactions.

## Hypothesis Backlog

### H1 — TT Exact Reuse Short-Circuits Verification
- What we see: every verification search returns the original TT exact score, so `singularVerificationScore >= singularVerificationBeta` persists.
- Mechanism: verification re-enters `negamax` with `makeExcludedContext`, but the TT probe still returns the parent EXACT entry and exits immediately (`src/search/negamax.cpp:475`). The helper never reaches move generation, so excluding the candidate has no effect.
- Supporting clues: `TranspositionTable::StorePolicyGuard` changes store mode, yet reads stay untouched; no guard checks `context.hasExcludedMove()` before honoring TT exact cutoffs.
- Proposed diagnostics:
  - Log when a verification node returns via TT cutoff vs. full search; confirm dominant path.
  - Temporarily gate TT EXACT returns whenever `context.hasExcludedMove()` to observe if `fail_low` appears.
  - Add counter for `verificationViaTT` to quantify impact.

### H2 — Move Pipeline Interactions (Check Extension / Stacking)
- Concern: other extension logic might neutralize the exclusion effect or mask candidate weaknesses.
- Observations:
  - Check extension bumps depth before we qualify singular candidates (`src/search/negamax.cpp:406`). For in-check nodes, the depth reaching verification could differ from TT depth, potentially reinforcing TT reuse.
  - Recapture stacking guardrails adjust extension budgets; need to ensure they are not reverting contexts before verification applies (`src/search/negamax.cpp:1700`).
- Proposed diagnostics:
  - Instrument verification nodes with current extension state (check extension applied? stacked budgets?) to see correlations with zero fail-lows.
  - Run targeted suites with check extensions disabled to test sensitivity.

### H3 — Depth Gating & Search Exit Criteria
- Question: are we gating verification too aggressively before depth can drop below quiescence thresholds?
- Notes:
  - Singular detection only activates when `depth >= SINGULAR_DEPTH_MIN` (currently 8, `src/search/negamax.cpp:219`). With verification reduction of 3, singular search depth becomes `depth-4`; still > 0, so no quiescence shortcut expected.
  - Need confirmation that no other pruning (e.g., null move, static null) terminates verification early once TT reuse is mitigated.
- Proposed diagnostics:
  - Track `(depth, singularDepth)` distributions for verification entries and ensure we rarely fall into `<=0` path.
  - Stress test with TT disabled entirely to see if fail-lows emerge; if so, revisit gating assumptions instead of tuning values.

### H4 — Candidate Qualification Bias
- We currently reject captures/promotions from singular consideration (`src/search/negamax.cpp:552`). If truly singular moves are mostly tactical, we may be filtering them out.
- Telemetry shows thousands of `verificationsStarted`, so enough candidates survive, but worth validating the quiet-only assumption.
- Diagnostic idea: log move types for qualified candidates and cross-check with known singular test positions (Phase plan §8).

## Immediate Experiment Queue
1. **Verification Cache Audit:** confirm that entries tagged with `TTEntryFlags::Exclusion` supply stable cutoffs, and measure how often verification retries hit those caches at various depths.
2. **Margin/Depth Sensitivity:** rerun focused telemetry (e.g., `wacnew.epd`, `bratko_kopec.epd`) with current margins to observe slack now that fail-lows trigger reliably.
3. **Check-Extension Interaction:** once telemetry is stable, add the planned `DisableCheckDuringSingular` toggle and sweep to ensure check extensions aren’t masking singular effects.
4. **TT-Off Backstop:** keep the TT-disabled depth run in reserve to validate results if cache behaviour shows unexpected anomalies.

## 2025-09-27 Findings
- **Baseline telemetry (current default):** `nodes=91`, `tt_exact=0`, `expanded=91`, `fail_low=17`, `extended=17` over a 20-position / 200 ms sweep—fail-lows now appear without any debug toggles.
- **Bypass telemetry (debug mode):** forcing the bypass still yields the same counts, confirming the default logic fully suppresses primary EXACT reuse while leaving cache entries untouched.
- **Per-position highlights:**
  - *Endgame pawn race* now records `fail_low=16`, `extended=16` with bypass → the canonical singular test finally extends.
  - *Karpov–Kasparov G16* produces `fail_low=1`, `extended=1`, demonstrating PV nodes can extend when the verification search actually runs.
- **Bench impact:**
  - Singular toggle ON (default TT rules): `bench 2350511`, `1776697 nps` (within bench noise vs. previous baseline `1744576 nps`).
- **Interpretation:** TT exact reuse was the sole blocker—ignoring primary EXACT entries while retaining verification-tagged caches restores singular activity without harming bench throughput. Focus now shifts to measuring cache hit rates, tuning margins, and validating interactions with other extensions.

### Verification Cache & Slack Telemetry (2025-09-27)
| Scenario | Verified | Fail-Low | Avg Slack (β - score) | Fail-High | Avg Slack (score - β) | TT Cache Hits |
|----------|----------|----------|------------------------|-----------|-----------------------|---------------|
| 20-pos / 200 ms (old margins) | 91 | 17 | **3.35 cp** | 74 | **37.1 cp** | 0 |
| `wacnew.epd` / 1 s (old margins) | 373 | 47 | **10.2 cp** | 323 | **41.7 cp** | 0 |
| `wacnew.epd` / 5 s (old margins) | 536 | 53 | **23.5 cp** | 469 | **41.4 cp** | 0 |
| `wacnew.epd` / depth 18 (old margins) | 406 | 18 | **2.6 cp** | 353 | **22.6 cp** | 0 |
| `UHO_4060_v2.epd` / 500 ms (old margins) | 12 948 | 31 | **1.0 cp** | 12 738 | **22.9 cp** | 0 |

**After margin update to {64,56,48,40,36,32 cp}:**

| Scenario | Verified | Fail-Low | Avg Slack (β - score) | Fail-High | Avg Slack (score - β) | TT Cache Hits |
|----------|----------|----------|------------------------|-----------|-----------------------|---------------|
| 20-pos / 200 ms | 66 | 16 | **2.1 cp** | 50 | **24.3 cp** | 0 |
| `wacnew.epd` / 1 s | 254 | 32 | **10.1 cp** | 222 | **23.0 cp** | 0 |
| `wacnew.epd` / 5 s | 487 | 49 | **40.5 cp** | 425 | **23.0 cp** | 0 |
| `wacnew.epd` / depth 18 | 371 | 30 | **1.6 cp** | 317 | **13.3 cp** | 0 |
| `UHO_4060_v2.epd` / 500 ms | 13 075 | 129 | **1.0 cp** | 12 891 | **14.2 cp** | 0 |

- Cache reuse remains effectively zero: verification searches behave as single-use probes, so the exclusion-tagged TT entries serve as fallback insurance without influencing current runs.
- Fail-high slack on UHO drops to ~14 cp with the new table, indicating we still have margin to tighten further once stability is confirmed across broader suites.
- Fail-low slack remains tiny on real-game positions (~1 cp) but balloons to ~40 cp on the tactical `wacnew` suite at 5 s. This suggests aggressive puzzles still require larger buffers; we may need depth- or node-dependent margins rather than a static table.

### 2025-09-27 — UHO/WAC chunked telemetry with adaptive margins
- **Execution details:** seven `UHO_4060_v2.epd` chunks (80 positions each, 5 s) plus three `wacnew.epd` chunks (≈333 positions per chunk, 5 s) using the chunked telemetry harness.
- **UHO summary:** 90 630 verifications, 658 fail-lows (all extended), 88 166 fail-highs. Average fail-high slack sits at ~15.9 cp; the mate-driven outlier chunk now caps at 256 cp after the telemetry clamp (re-run shows fail-low slack sum 200 cp, `chk_sup=0`, `chk_app=54`).
- **WAC summary:** 48 288 verifications, 398 fail-lows (all extended) with aggregate fail-low slack ≈1.0 cp and fail-high slack ≈16.2 cp, confirming the adaptive margins preserve the tight tactical coverage we had before while still reducing verification workloads.
- **Tooling:** `tools/stacked_telemetry.py` now supports `--offset`, `--chunk-size`, `--max-chunks`, and `--passes` so long sweeps stay under the 10 minute harness limit while preserving per-chunk summaries and overall aggregates.
- **Check-extension toggle:** `DisableCheckDuringSingular` suppresses the automatic in-check extension on verification nodes; telemetry now reports `chk_sup/chk_app` so we can A/B stacking depth parity versus the default behaviour.

## Open Questions
- Do we need a dedicated TT namespace/flag for verification probes to avoid returning stale exact scores (`TranspositionTable::StorePolicy` logic review)?
- Should margin calculations consume the *verification* depth instead of parent depth to better reflect search horizon?
- Are we double-counting failure paths when verification searches share PV context state (e.g., `childContext.setPv(true)`)?

## Next Steps Checklist
1. **Tactical telemetry sweep:** run `wacnew.epd` (≥1 000 positions) at 5 s/move and record P50/P90/P95 fail-low slack to quantify the buffer tactical lines require.
2. **Adaptive margin prototype:** design a depth/TT-gap aware adjustment (e.g., add slack when verification depth ≥12 or TT score gap > threshold) and implement behind a toggle for A/B testing.
3. **Validation pass:** compare WAC/UHO telemetry with the adaptive margins enabled; ensure real-game slack stays tight while tactical slack settles near 10–15 cp before promoting the change.

---
_Last updated: 2025-09-27_
