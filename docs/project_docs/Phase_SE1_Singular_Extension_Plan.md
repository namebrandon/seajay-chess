# Phase SE1 — Singular Extension Enablement Plan

## 1. Objective & Scope
- **Goal:** Enable production-ready singular extensions on top of the Phase 6 Search Node API refactor without regressing depth parity or NPS.
- **Secondary Goals:**
  - Validate excluded-move plumbing and verification helper (`verify_exclusion`) under live search conditions.
  - Reconcile singular extensions with existing selective extensions (check, recapture) and pruning heuristics.
  - Establish diagnostics and tuning hooks for future multi-cut / probcut work.
- **Out of Scope:** LazySMP coordination changes, multi-cut/probcut enablement, or evaluation feature work.

## 2. Dependencies & Baseline
- **Required toggles before Phase SE1:**
  - `UseSearchNodeAPIRefactor = true`
  - `EnableExcludedMoveParam = true`
- **Existing scaffolding:** `singular_extension.{h,cpp}` currently returns neutral scores; `verify_exclusion` is a stub.
- **Baseline Metrics:**
  - Capture bench (Release, Linux) and depth parity traces with toggles ON but singular logic disabled.
  - Collect search telemetry on TT hit rates, singular candidate frequency, and existing extension counts.
  - **NPS Baseline:** Document current NPS across thread counts (1, 2, 4, 8 threads)
  - **Depth Baseline:** Record depth reached at fixed time (e.g., 10s) on standard test suite
- **Pre-flight TODO:**
  - Audit `SearchInfo` and `NodeContext` to ensure no lingering legacy `excludedMove` usage once SE1 begins.
  - Confirm quiescence path ignores singular logic (per guardrails).
  - Establish NPS regression threshold: Maximum 2% loss acceptable per stage

## 2.5 Thread Safety Requirements
- **Thread-local data (no synchronization needed):**
  - `SingularVerifyStats` counters per thread
  - Extension depth tracking per thread
  - Verification search scratch space (reuse existing thread-local infrastructure)
  - Move ordering scratch buffers
- **Shared data (requires synchronization):**
  - TranspositionTable entries (existing atomic operations)
  - Global feature toggles (read-only after UCI initialization)
  - Global statistics aggregation (atomic counters for reporting)
- **Memory ordering guarantees:**
  - TT reads use acquire semantics (existing implementation)
  - TT writes use release semantics (existing implementation)
  - Stats aggregation uses relaxed ordering (performance counters only)
- **LazySMP compatibility:**
  - All per-thread data must use thread_local storage
  - No assumptions about thread count or search tree sharing
  - Extension budgets tracked per-thread to avoid cross-thread interference

## 3. Risk Register & Mitigations
| Risk | Impact | Mitigation |
|------|--------|------------|
| False positives trigger frequent singular re-search causing 3-5% NPS loss | High | Tune TT quality threshold, require multi-criteria gating (depth >= 4, TT exact hit, move ordering stability). |
| Interactions with existing check extension cause double-extensions leading to search explosion | High | Add extension-budget tracking per node; temporarily disable check extension during SE1 testing if necessary. |
| TT pollution from low-depth verification search | Medium | Store verification results in separate TT bucket or avoid storing altogether; mark entries with exclusion bit. |
| Verification helper diverges from main search parameters | Medium | Share search limits/time budget struct; unit test verification vs baseline search for identical parameters except excluded move. |
| Increased code complexity without sufficient observability | Medium | Add DEBUG counters (`singularAttempts`, `singularVerified`, `singularExtensionsApplied`), log under `SearchStats`. |

## 4. Staged Implementation Plan
Each stage ends with: `./build.sh Release`, `echo "bench" | ./bin/seajay`, perft spot-check, commit with `bench XXXXX`. SPRT bounds default to `[-3.00, 3.00]` unless noted. **NPS must be tracked and remain within 2% of baseline unless explicitly noted.**

### Stage SE-1 — Baseline Performance Capture (Pre-implementation metrics)
- **SE-1.1 – Performance baseline**
  - Run 100-position EPD test suite at 10s/move, document average depth reached
  - Capture NPS metrics: 1-thread, 2-thread, 4-thread, 8-thread configurations
  - Create `tests/regression/singular_baseline.txt` with results
  - Expected NPS impact: N/A (baseline capture only)
- **SE-1.2 – Test position suite**
  - Compile positions known to benefit from singular extensions
  - Add deterministic test harness in `tests/singular/`
  - Document expected behavior for each position

### Stage SE0 — Foundations & Telemetry (No functional change)
- **SE0.1a – Thread-local telemetry structure**
  - Add thread-local `SingularStats` struct to `SearchData`
  - Implement fields: `candidates_examined`, `verifications_started`, `extensions_applied`
  - Expected NPS impact: < 0.1% (struct allocation only)
  - SPRT: Bench parity verification
- **SE0.1b – Global aggregation infrastructure**
  - Add atomic global counters for cross-thread statistics
  - Implement aggregation in UCI info output (thread 0 only)
  - Expected NPS impact: < 0.2% (atomic reads on info output)
  - SPRT: Bench parity verification
- **SE0.2a – UCI toggle exposure**
  - Add `UseSingularExtensions` bool toggle (default false)
  - Ensure toggle is exposed via UCI setoption
  - Update `feature_status.md` template
  - Expected NPS impact: 0% (toggle check only)
- **SE0.2b – Defensive assertions**
  - Add DEBUG-only asserts for `excluded` flag lifecycle
  - Verify flag clears on all exit paths from negamax
  - Expected NPS impact: 0% (DEBUG only)
- **SE0.3 – Legacy cleanup**
  - Remove residual `SearchInfo::excludedMove` usage once NodeContext exclusive path confirmed
  - Simplify toggle structure after Phase 6 validation
  - Expected NPS impact: 0%

### Stage SE1 — Verification Helper Activation
- **SE1.1a – Basic verification structure**
  - Implement `verify_exclusion` function skeleton that returns zero (no-op)
  - Add toggle guard: early return if `UseSingularExtensions == false`
  - Wire up stats tracking (increment `bypassed` counter)
  - Expected NPS impact: 0% (no-op function)
  - SPRT: Bench parity
- **SE1.1b – Depth reduction calculation**
  - Add `verificationReduction` parameter (initially hardcoded to 3)
  - Calculate `singularDepth = depth - 1 - verificationReduction`
  - Add bounds checking (return if singularDepth <= 0)
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
- **SE1.1c – Window narrowing setup**
  - Implement null-window calculation: `[beta - 1, beta]`
  - Add fail-soft clamping to ensure valid window
  - Prepare childContext with excluded move flag set
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
- **SE1.1d – Negamax recursion hookup**
  - Call negamax with prepared parameters
  - Propagate NodeContext correctly (maintain PV flag)
  - Return verification score (initially unused)
  - Expected NPS impact: 0% (feature still disabled)
  - SPRT: Bench parity
- **SE1.2a – TT store policy decision**
  - Implement `NO_STORE` flag for verification searches
  - Add dedicated TT flag bit for "verified under exclusion"
  - Expected NPS impact: 0%
- **SE1.2b – TT contamination guards**
  - Add instrumentation to verify main TT entry unchanged
  - DEBUG-only validation of TT state pre/post verification
  - Expected NPS impact: 0% (DEBUG only)

### Stage SE2 — Candidate Identification & Qualification
- **SE2.1a – TT lookup and depth check**
  - Implement TT probe for current position
  - Check depth >= `singularDepthMin` (initially 8)
  - Verify TT entry has EXACT flag and valid best move
  - Expected NPS impact: < 0.5% (TT probe overhead)
  - SPRT: Bench parity
- **SE2.1b – Score margin calculation**
  - Implement `singularMargin(depth)` function
  - Initial table: `{depth>=8: 60cp, depth>=6: 80cp, else: 100cp}`
  - Calculate `singularBeta = ttScore - singularMargin(depth)`
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
- **SE2.1c – Move validation and qualification**
  - Verify TT move is legal in current position
  - Check move is non-tactical (not capture/promotion)
  - Set up excluded move in NodeContext
  - Expected NPS impact: < 0.3%
  - SPRT: Bench parity
- **SE2.2a – Window clamping**
  - Implement fail-soft window adjustment
  - Ensure `singularBeta` stays within valid score range
  - Add overflow protection for extreme scores
  - Expected NPS impact: 0%
- **SE2.2b – Extension decision logic**
  - Compare verification score to `singularBeta`
  - Return extension flag (initially no action taken)
  - Track decision in thread-local stats
  - Expected NPS impact: < 0.2%

### Stage SE3 — Extension Application & Interaction Guards
- **SE3.1a – Extension tracking infrastructure**
  - Add thread-local `extensionBudget` to SearchInfo (max 16 ply total)
  - Implement per-thread extension depth counter
  - Add overflow protection (stop extending at budget limit)
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
- **SE3.1b – Extension interaction rules**
  - Define priority: check extension > singular > recapture
  - Implement mutual exclusion (max 1 extension type per node)
  - Add UCI toggle `AllowStackedExtensions` (default false)
  - Expected NPS impact: 0%
- **SE3.1c – Check extension coordination**
  - Add toggle `DisableCheckDuringSingular` for A/B testing
  - Implement conditional check extension disable
  - Track interaction statistics
  - Expected NPS impact: Potentially +1-2% (fewer extensions)
- **SE3.2a – Extension application**
  - When verification confirms singularity, set extension flag
  - Increment depth by `singularExtensionDepth` (initially 1)
  - Update thread-local extension budget
  - Expected NPS impact: -2-5% when enabled (deeper searches)
  - SPRT: [0.00, 5.00] bounds
- **SE3.2b – Context propagation**
  - Ensure child nodes receive updated NodeContext
  - Maintain PV flag through extended search
  - Update triangular PV correctly
  - Expected NPS impact: < 0.1%

### Stage SE4 — Diagnostics, UCI toggles, and Tuning Hooks
- **SE4.1a – Core UCI controls**
  - Add `UseSingularExtensions` bool (default false)
  - Add `SingularDepthMin` int (default 8, range 4-20)
  - Add `SingularMarginBase` int (default 60, range 20-200)
  - Expected NPS impact: 0% (configuration only)
- **SE4.1b – Advanced tuning parameters**
  - Add `SingularVerificationReduction` int (default 3, range 2-5)
  - Add `SingularExtensionDepth` int (default 1, range 1-2)
  - Hook into `engine_config` for OpenBench parameter sweeps
  - Expected NPS impact: 0%
- **SE4.2a – Debug diagnostics**
  - Implement `debug singular` UCI command
  - Output per-position singular decision log
  - Include: position, TT info, margin, verification result
  - Expected NPS impact: -50% when enabled (heavy logging)
- **SE4.2b – Regression test harness**
  - Create `tests/regression/singular_tests.cpp`
  - Add deterministic test positions from Section 8
  - Verify feature ON/OFF produces expected extensions
  - Expected NPS impact: N/A (test mode only)

### Stage SE5 — Validation & Rollout
- **SE5.1a – Perft validation**
  - Run perft suite with `UseSingularExtensions=true`
  - Verify node counts match baseline (extensions don't affect perft)
  - Test positions from Section 8 at depth 6
  - Expected NPS impact: -5% to -10% (extensions active)
- **SE5.1b – Time-to-depth validation**
  - Run 100-position suite at 10s/move
  - Compare average depth with/without singular extensions
  - Target: +0.3 to +0.5 ply average depth increase
  - Document NPS vs depth trade-off
- **SE5.2a – Initial SPRT sanity check**
  - Run short SPRT with bounds `[-5.00, 0.00]`
  - Verify no regression with feature enabled
  - Duration: ~1000 games at 10+0.1
  - Expected: PASS (no regression)
- **SE5.2b – Full SPRT validation**
  - Launch main vs feature with bounds `[0.00, 5.00]`
  - Time control: 10+0.1, minimum 5000 games
  - Track: ELO gain, singular trigger rate, extension rate
  - Expected: +15-25 ELO improvement
- **SE5.3a – Windows build validation**
  - Rebuild under MSYS2 UCRT64 environment
  - Run bench to verify deterministic node count
  - Test UCI stop/start commands during search
  - Expected NPS impact: Same as Linux
- **SE5.3b – Multi-platform stress test**
  - Run 24-hour stability test on Windows/Linux
  - Monitor for memory leaks or crashes
  - Verify thread-local storage cleanup
  - Expected: Zero crashes, stable memory usage
- **SE5.4 – Documentation & handoff**
  - Update `feature_status.md` with SE1 completion status
  - Append summary to `Phase6_Search_API_Refactor.md`
  - Document final tuning parameters and ELO gain
  - Create tuning guide for OpenBench parameter sweeps

## 5. Branching & Workflow
- **Main feature branch:** `feature/20250921-singular-extension-plan`
- **Sub-stage branches:** Create from main feature branch for each sub-stage
  - Example: `feature/20250921-se0-1a-telemetry`
  - Merge back to feature branch after SPRT pass
- **Commit message format:**
  ```
  feat(SE1): [Stage X.Ya] Description - bench NNNNNNN

  NPS impact: X% (measured vs baseline)
  SPRT result: PASS/PENDING
  ```
- **Stage completion checklist:**
  1. Run `./build.sh Release && echo "bench" | ./bin/seajay`
  2. Verify bench count matches expected
  3. Measure NPS impact vs baseline
  4. Run perft verification if applicable
  5. Update `feature_status.md` with stage completion
  6. Commit with proper bench count
  7. Push and create OpenBench SPRT test
  8. Wait for SPRT completion before next stage
- **Final merge criteria:**
  - All SE5 validation stages passed
  - Overall ELO gain > 10
  - NPS regression < 10% cumulative
  - Windows/Linux parity confirmed
  - Documentation complete

## 6. Testing Matrix
| Scenario | Toggle Config | Target |
|----------|---------------|--------|
| Baseline parity | `UseSingularExtensions=false`, others true | Bench within noise, perft exact |
| Verification sandbox | `UseSingularExtensions=false`, `DEV_ASSERT_SearchAPI=true` | No assertions triggered |
| Singular ON sanity | `UseSingularExtensions=true`, short matches | Verify counters increment, no crashes |
| Full SPRT | `UseSingularExtensions=true` | Bounds `[0.00, 5.00]`, 10+0.1 TC |
| Windows regression | `UseSingularExtensions=true` | Stop/go loop, go depth N + stop |

## 7. Toggle Management Strategy
After Phase 6 validation and SE1 completion, simplify toggle structure:
- **Phase 6 toggles (to be removed after SE1 stability):**
  - `UseSearchNodeAPIRefactor` - Set true permanently, remove toggle
  - `EnableExcludedMoveParam` - Set true permanently, remove toggle
- **Production toggles (remain for tuning):**
  - `UseSingularExtensions` - Main feature toggle
  - `SingularDepthMin` - Tunable parameter (default 8)
  - `SingularMarginBase` - Tunable parameter (default 60)
  - `DisableCheckDuringSingular` - A/B testing toggle
  - `AllowStackedExtensions` - Future enhancement toggle

## 8. Test Positions for Validation
Key positions for singular extension validation:

```
# Position 1: Karpov-Kasparov 1985, Game 16 - Famous singular defensive move
# Black's Bf5-e6 is singular, preventing White's breakthrough
8/8/4kpp1/3p1b2/p6P/2B5/6P1/6K1 b - - 0 47
Expected: Bf5-e6 should trigger singular extension

# Position 2: Deep tactical position with unique defensive resource
# White must find the singular Qf3-e3 to defend
r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
Expected: Qf3-e3 should be identified as singular move

# Position 3: Endgame with critical pawn breakthrough
# White's h5 is the only winning move
8/5pk1/4p1p1/7P/5P2/6K1/8/8 w - - 0 1
Expected: h5-h6 should receive singular extension

# Position 4: Complex middlegame with hidden tactical resource
# Black's Nxe4 is the only move maintaining equality
r1bqkb1r/pp3ppp/2n1pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R b KQkq - 0 6
Expected: Nf6xe4 should be singular when deeply analyzed

# Position 5: Forced sequence with singular intermediate move
# White's Rxf7 is singular, leading to forced win
3r1rk1/p1q2pbp/1np1p1p1/8/2PNP3/1P3P2/PB3QPP/3R1RK1 w - - 0 1
Expected: Rf1xf7 should trigger extension in critical line
```

## 9. Performance Regression Tests
Create automated performance regression suite:
- Baseline NPS tolerance: -2% per stage (cumulative -10% for full SE1)
- Depth parity: Must maintain same depth at fixed time (10s)
- Node count: Bench must remain deterministic when feature disabled
- Memory usage: Track peak memory, should not increase > 1%

## 10. Follow-Up Work
- Stage SE2 tuning sweeps (SingularMargin table) once initial enablement stable
- Explore multi-extension interactions (recapture, passed pawn) using extension budget framework
- Prepare Phase MC1 (multi-cut) leveraging same verification API
- Consider double/triple extensions for extremely singular moves (depth > 20)

---

**Next Action:** Kick off Stage SE-1.1 baseline performance capture on current branch, then proceed to SE0.1a telemetry scaffolding on dedicated feature sub-branch.
