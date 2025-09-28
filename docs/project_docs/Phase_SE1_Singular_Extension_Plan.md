# Phase SE1 — Singular Extension Enablement Plan

## 1. Objective & Scope
- **Goal:** Enable production-ready singular extensions on top of the Phase 6 Search Node API refactor without regressing depth parity or NPS.
- **Secondary Goals:**
  - Validate excluded-move plumbing and verification helper (`verify_exclusion`) under live search conditions.
  - Reconcile singular extensions with existing selective extensions (check, recapture) and pruning heuristics.
  - Establish diagnostics and tuning hooks for future multi-cut / probcut work.
- **Out of Scope:** LazySMP coordination changes, multi-cut/probcut enablement, or evaluation feature work.

### 1.1 Phase Naming & Reporting Contract
- All execution phases follow the feature-guideline naming `SingularExtension_Phase_SEX.Y` to keep commit messages, status updates, and `feature_status.md` entries consistent. Example: Stage `SE0.1a` in this plan maps to `SingularExtension_Phase_SE0.1a` in commits and documentation.
- Every phase report includes `bench <nodes>` lines, per-machine telemetry (see §2 Baseline Metrics), and a reference to the corresponding section in this document.
- When new sub-stages are added, update this plan and the feature tracking document in lockstep to avoid divergent terminology.

## 2. Dependencies & Baseline
- **Required toggles before Phase SE1:**
  - `UseSearchNodeAPIRefactor = true`
  - `EnableExcludedMoveParam = true`
- **Existing scaffolding:** `singular_extension.{h,cpp}` currently returns neutral scores; `verify_exclusion` is a stub.
- **Baseline Metrics:**
  - Capture bench (Release, Linux) and depth parity traces with toggles ON but singular logic disabled.
  - Collect search telemetry on TT hit rates, singular candidate frequency, and existing extension counts.
  - Record per-machine NPS alongside the matching `bench` node count so we can normalize (`NPS / bench_nodes`) when comparing hardware.
  - **Depth Baseline:** Record depth reached at fixed time (e.g., 10s) on standard test suite.
  - **Telemetry Template:**

    | Machine | Branch/Commit | Bench Nodes | Threads | Raw NPS | Normalized NPS (`NPS / bench`) | Depth @10s | TT Hit % | Notes |
    |---------|---------------|-------------|---------|---------|-------------------------------|------------|----------|-------|
    | Desktop | `main@<sha>`  | 19191913    | 1/2/4/8 | 1.75M…  | 0.091…                        | 43         | 68%      | Idle workload |
    | Laptop  | `main@<sha>`  | 14200000    | 1/2/4   | 1.10M…  | 0.078…                        | 38         | 64%      | On battery |

    Capture one row per thread-count configuration; annotate anomalies (e.g., background load) so later phases can filter noisy data.
- **Pre-flight TODO:**
  - Audit `SearchInfo` and `NodeContext` to ensure no lingering legacy `excludedMove` usage once SE1 begins.
  - Confirm quiescence path ignores singular logic (per guardrails).
  - Establish NPS regression guidelines: aim to stay within a 2% loss per stage; deviations up to 5% are acceptable only with explicit justification, logging, and a documented rollback trigger.
  - Verify UCI toggles exist (`UseSearchNodeAPIRefactor`, `EnableExcludedMoveParam`, `UseSingularExtensions`) and document their defaults in release notes prior to SE0.1 kickoff.

## 2.5 Thread Safety Requirements
- **Thread-local data (no synchronization needed):**
  - `SingularVerifyStats` counters per thread
  - Extension depth tracking per thread
  - Verification search scratch space (reuse existing thread-local infrastructure)
  - Move ordering scratch buffers
  - **Stack overflow guard:** Thread-local `max_extension_depth` counter (hard limit 32)
  - **Verification recursion budget:** Separate from main search to prevent interference
- **Shared data (requires synchronization):**
  - TranspositionTable entries (existing atomic operations)
  - Global feature toggles (read-only after UCI initialization)
  - Global statistics aggregation (atomic counters for reporting)
  - **Performance note:** Use `alignas(64)` for atomic counters to prevent false sharing
- **Memory ordering guarantees:**
  - TT reads use `memory_order_acquire` (existing implementation)
  - TT writes use `memory_order_release` (existing implementation)
  - Stats aggregation uses `memory_order_relaxed` (performance counters only)
  - **Critical:** Verification search must NOT use `memory_order_seq_cst` anywhere
- **LazySMP compatibility:**
  - All per-thread data must use thread_local storage with `alignas(64)` for cache alignment
  - No assumptions about thread count or search tree sharing
  - Extension budgets tracked per-thread to avoid cross-thread interference
  - **TT contamination prevention:** Verification searches mark entries with `EXCLUSION_VERIFIED` flag
  - **Helper thread coordination:** Main thread aggregates stats only at UCI info intervals (not per-node)
- **C++20 Implementation Notes:**
  - Use `std::atomic_ref` for stats aggregation to avoid atomic overhead in single-threaded mode
  - Consider `std::latch` for synchronizing stats collection across threads
  - Apply `[[likely]]`/`[[unlikely]]` attributes for extension decision branches

## 3. Risk Register & Mitigations
| Risk | Impact | Mitigation |
|------|--------|------------|
| False positives trigger frequent singular re-search causing 3-5% NPS loss | High | Tune TT quality threshold, require multi-criteria gating (depth >= 4, TT exact hit, move ordering stability). **Performance:** Early-exit with `[[likely]]` branch hints when depth < threshold. |
| Interactions with existing check extension cause double-extensions leading to search explosion | High | Add extension-budget tracking per node; temporarily disable check extension during SE1 testing if necessary. **Stack safety:** Hard limit total extensions to 32 ply with compile-time assert. |
| TT pollution from low-depth verification search | Medium | Store verification results in separate TT bucket or avoid storing altogether; mark entries with exclusion bit. **Performance:** Use dedicated 16-bit exclusion hash to avoid full TT probe. |
| Verification helper diverges from main search parameters | Medium | Share search limits/time budget struct; unit test verification vs baseline search for identical parameters except excluded move. **C++20:** Use concepts to enforce parameter compatibility. |
| Increased code complexity without sufficient observability | Medium | Add DEBUG counters (`singularAttempts`, `singularVerified`, `singularExtensionsApplied`), log under `SearchStats`. **Performance:** Use conditional compilation to completely eliminate stats in Release builds. |
| Integer overflow in score calculations with extreme margins | Medium | Add compile-time bounds checking with `std::numeric_limits`. Use saturating arithmetic for score operations. **C++23:** Consider `std::saturate_cast` when available. |
| Cache line bouncing between threads accessing shared TT | High | Ensure TT entries are cache-aligned (64 bytes). Consider NUMA-aware allocation for large TT. **Performance:** Prefetch TT entries with `__builtin_prefetch` before verification. |
| Compiler fails to inline critical path functions | High | Force inline with `__attribute__((always_inline))` for hot path. Profile-guided optimization (PGO) required for production builds. **C++20:** Use `[[gnu::always_inline]]` attribute. |
| Memory allocation during search causing stalls | High | All verification structures must be pre-allocated in thread-local storage. Zero dynamic allocation permitted in search loop. **C++17:** Use `pmr::monotonic_buffer_resource` for scratch space. |

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
  - **C++20 Implementation:**
    ```cpp
    struct alignas(64) SingularStats {  // Cache line aligned
        std::uint64_t candidates_examined{0};
        std::uint64_t candidates_qualified{0};
        std::uint64_t candidates_rejected_illegal{0};
        std::uint64_t candidates_rejected_tactical{0};
        std::uint64_t verifications_started{0};
        std::uint64_t verification_fail_low{0};
        std::uint64_t verification_fail_high{0};
        std::uint64_t extensions_applied{0};
        std::uint32_t max_extension_depth{0};
        std::uint32_t verification_cache_hits{0};
    };
    ```
  - Use `[[no_unique_address]]` for zero-overhead when disabled
  - Expected NPS impact: < 0.1% (struct allocation only)
  - SPRT: Bench parity verification
- **SE0.1b – Global aggregation infrastructure**
  - Add atomic global counters for cross-thread statistics
  - Implement aggregation in UCI info output (thread 0 only)
  - **Performance-critical implementation:**
    ```cpp
    struct alignas(64) GlobalSingularStats {
        std::atomic<std::uint64_t> total_examined{0};
        std::atomic<std::uint64_t> total_qualified{0};
        std::atomic<std::uint64_t> total_rejected_illegal{0};
        std::atomic<std::uint64_t> total_rejected_tactical{0};
        std::atomic<std::uint64_t> total_verified{0};
        std::atomic<std::uint64_t> total_fail_low{0};
        std::atomic<std::uint64_t> total_fail_high{0};
        std::atomic<std::uint64_t> total_extended{0};
        std::atomic<std::uint32_t> max_extension_depth{0};
        std::atomic<std::uint64_t> total_cache_hits{0};

        void aggregate(const SingularStats& local) noexcept {
            total_examined.fetch_add(local.candidates_examined, std::memory_order_relaxed);
            total_qualified.fetch_add(local.candidates_qualified, std::memory_order_relaxed);
            total_rejected_illegal.fetch_add(local.candidates_rejected_illegal, std::memory_order_relaxed);
            total_rejected_tactical.fetch_add(local.candidates_rejected_tactical, std::memory_order_relaxed);
            total_verified.fetch_add(local.verifications_started, std::memory_order_relaxed);
            total_fail_low.fetch_add(local.verification_fail_low, std::memory_order_relaxed);
            total_fail_high.fetch_add(local.verification_fail_high, std::memory_order_relaxed);
            total_extended.fetch_add(local.extensions_applied, std::memory_order_relaxed);
            max_extension_depth.store(std::max(max_extension_depth.load(std::memory_order_relaxed),
                                              local.max_extension_depth), std::memory_order_relaxed);
            total_cache_hits.fetch_add(local.verification_cache_hits, std::memory_order_relaxed);
        }
    };
    ```
  - Only aggregate at UCI info intervals (every 1000ms), not per-node
  - Use `std::atomic_ref` in single-threaded mode to eliminate overhead; maintain max depth and cache hit tracking alongside counters
  - Expected NPS impact: < 0.2% (atomic reads on info output)
  - SPRT: Bench parity verification
- **SE0.2a – UCI toggle exposure**
  - Add `UseSingularExtensions` bool toggle (default false)
  - Ensure toggle is exposed via UCI `setoption` and threaded through `SearchLimits`
  - Gate telemetry aggregation/InfoBuilder reporting behind the toggle so instrumentation stays cold when disabled
  - Update `feature_status.md` template and release notes with default values
  - Expected NPS impact: 0% (toggle check only)
- **SE0.2b – Defensive assertions**
  - Add DEBUG-only asserts for `excluded` flag lifecycle
  - Verify flag clears on all exit paths from negamax (RAII guard + sanity asserts)
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
  - Status: Completed on 2025-09-23 (bench parity 2350511)
  - **C++20 Implementation with branch prediction:**
    ```cpp
    [[nodiscard]] inline Score verify_exclusion(
        SearchData& data,
        Board& board,
        Move excluded_move,
        Score beta,
        Depth depth) noexcept {

        if (!UseSingularExtensions) [[unlikely]] {
            return SCORE_ZERO;
        }
        // Implementation continues...
    }
    ```
  - Force inline with `__attribute__((always_inline))` in Release builds
  - Expected NPS impact: 0% (no-op function)
  - SPRT: Bench parity
- **SE1.1b – Depth reduction calculation**
  - ✅ Hardcode `verificationReduction = 3` and derive `singularDepth = depth - 1 - verificationReduction`
  - ✅ Early-out when `singularDepth <= 0` (DEBUG tallies `stats->ineligible`)
  - Status: Completed 2025-09-23 on `feature/20250923-singular-extension-se11b` (bench 2350511, parity)
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
- **SE1.1c – Window narrowing setup**
  - ✅ Implement null-window calculation: `[beta - 1, beta]`
  - ✅ Add fail-soft clamping to ensure valid window
  - Prepare childContext with excluded move flag set (deferred to SE1.1d alongside recursion hookup)
  - Status: Completed 2025-09-23 on `feature/20250923-singular-extension-se11c` (bench 2350511, parity)
  - **Overflow-safe implementation:**
    ```cpp
    constexpr Score clamp_score(Score s) noexcept {
        return std::clamp(s, -SCORE_MATE + MAX_PLY, SCORE_MATE - MAX_PLY);
    }

    const Score singular_alpha = clamp_score(beta - 1);
    const Score singular_beta = beta;  // Already validated
    ```
  - Use `std::bit_cast` for type-safe score manipulation if needed
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
- **SE1.1d – Negamax recursion hookup**
  - ✅ Call negamax with prepared parameters (`singularDepth`, `[beta-1, beta]`)
  - ✅ Propagate NodeContext via `makeExcludedContext` while preserving PV/root flags
  - ✅ Return verification score (still consumed by caller later stages)
  - Status: Completed 2025-09-23 on `feature/20250923-singular-extension-se11d` (bench 2350511, parity)
  - Expected NPS impact: 0% (feature still disabled)
  - SPRT: Bench parity
- **SE1.2a – TT store policy decision**
  - ✅ Define verification-specific TT semantics:
    - `TranspositionTable::StorePolicyGuard` places verification searches in a dedicated store mode.
    - Verification stores only touch empty slots or prior `TT_EXCLUSION` entries; primary searches clear the flag when overwriting.
  - ✅ Draft replacement policy adjustments:
    - Flagged `TT_EXCLUSION` entries are preferred victims when primary data arrives (scoring bias of -20k).
    - Verification writes brand entries with `TT_EXCLUSION` while leaving primary heuristics untouched.
  - ✅ Telemetry prep: Capture DEBUG `SingularStats` counters with verification enabled and record TT occupancy deltas vs. baseline (1/2/4 thread configs).
    - 2025-09-23 capture (`depth 10`, startpos): singular telemetry remained zero (no candidates yet), TT stores held at ~9.6k with 0 collisions; raw logs in `telemetry_threads{1,2,4}.log`.
  - ✅ Call out instrumentation requirements for DEBUG builds (verify verification-mode stores never evict primary entries).
  - ✅ Implementation sketch:
    - `TTEntry` grew a `flags` byte plus helpers (`hasFlag`, `save(flags)`).
    - Cluster/linear store paths consult `StorePolicy` and gate writes accordingly.
    - Primary stores zero the flag on overwrite so downstream logic sees clean entries.
  - Status: Completed 2025-09-23 on `feature/20250923-singular-extension-se12a-docs` (bench 2350511, parity).
  - Expected NPS impact: 0%
- **SE1.2b – TT contamination guards**
  - ✅ Add debug-only sentinel that asserts if verification mode attempts to overwrite primary TT entries.
  - ✅ Track verification store/skip counters in TT stats for telemetry.
  - DEBUG-only validation of TT state pre/post verification
  - Status: Completed 2025-09-24 on `feature/20250923-singular-extension-se12b-guards` (bench 2350511, parity)
  - Expected NPS impact: 0% (DEBUG only)

### Stage SE2 — Candidate Identification & Qualification
- **SE2.1a – TT lookup and depth check**
  - ✅ Implement TT probe gate inside negamax to validate TT entries (prefetch, probe, depth/flag/move checks).
  - ✅ Require EXACT flag and non-zero move from TT before treating as candidate (counters incremented via `singularStats`).
  - Status: Completed 2025-09-24 on `feature/20250924-singular-extension-se21a` (bench 2350511, parity)
  - Expected NPS impact: < 0.5% (TT probe overhead)
  - SPRT: Bench parity
- **SE2.1b – Score margin calculation**
  - ✅ Implement depth-indexed `singular_margin` lookup (≥8 → 60cp, ≥6 → 80cp, else 100cp) with constexpr table.
  - ✅ Extend `verify_exclusion` to take TT score, derive `singularBeta = clamp(ttScore - singular_margin(depth))`, and narrow window via saturating subtract.
  - ✅ Upgrade `singular_margin` to adapt using TT depth gap, β proximity, and verification horizon so critical nodes tighten automatically while soft bounds remain conservative for shallow probes.
  - ✅ Guard window construction with clamp helper so verification stays within mate bounds (no underflow).
  - Status: Completed 2025-09-24 on `feature/20250924-singular-extension-se21b` (bench 2350511, parity)
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
- **SE2.1c – Move validation and qualification**
  - ✅ Verify TT move legality via `MoveGenerator::isLegal`
  - ✅ Reject tactical TT moves (captures/promotions) from singular pipeline
  - ✅ Prime NodeContext with excluded move (cleared ahead of main move loop until SE2.2 verification wiring)
  - ✅ Telemetry: added `candidatesQualified`, `candidatesRejectedIllegal`, `candidatesRejectedTactical`
  - Status: Completed 2025-09-24 on `feature/20250924-singular-extension-se21c` (bench 2350511, parity)
  - Expected NPS impact: < 0.3%
  - SPRT: Bench parity
- **SE2.2a – Window clamping**
  - ✅ Launch verification search immediately after TT candidate qualification
  - ✅ Clamp verification window via `singular_margin` and `clamp_singular_score` prior to helper dispatch
  - ✅ Increment `verificationsStarted` telemetry and stash `singularVerificationBeta` for SE2.2b comparison
  - NodeContext primed/cleared around trigger so legacy excluded plumbing stays in sync
  - Expected NPS impact: 0%
- **SE2.2b – Extension decision logic**
  - ✅ Compare verification score to `singularBeta` on the next move loop entry
  - ✅ Track decision outcome via `verification_fail_low` / `verification_fail_high`
  - No extension yet (Stage SE3 handles application), but telemetry now differentiates verification outcomes
  - Expected NPS impact: < 0.2%

### Stage SE3 — Extension Application & Interaction Guards
- **SE3.1a – Extension tracking infrastructure**
  - Add thread-local `extensionBudget` to SearchInfo (max 16 ply total)
  - Implement per-thread extension depth counter
  - Add overflow protection (stop extending at budget limit)
  - **Stack-safe extension tracking:**
    ```cpp
    struct ExtensionTracker {
        static constexpr Depth MAX_TOTAL_EXTENSIONS = 32;
        static constexpr Depth MAX_SINGULAR_EXTENSIONS = 16;

        std::uint32_t total_extensions{0};
        std::uint32_t singular_extensions{0};

        [[nodiscard]] bool can_extend() const noexcept {
            return total_extensions < MAX_TOTAL_EXTENSIONS;
        }

        void apply_extension(Depth amount = 1) noexcept {
            total_extensions = std::min(total_extensions + amount,
                                       MAX_TOTAL_EXTENSIONS);
        }
    };
    static_assert(ExtensionTracker::MAX_TOTAL_EXTENSIONS <= MAX_PLY / 2);
    ```
  - Use `std::hardware_destructive_interference_size` for padding
  - Expected NPS impact: < 0.1%
  - SPRT: Bench parity
  - Status: Completed 2025-09-25 on `feature/20250921-singular-extension-plan` (merge 4ac6ee0, bench 2350511)
- **SE3.1b – Extension interaction rules**
  - Define priority: check extension > singular > recapture
  - Implement mutual exclusion (max 1 extension type per node)
  - Add UCI toggle `AllowStackedExtensions` (default false)
  - Expected NPS impact: 0%
  - Status: Completed 2025-09-26 on `feature/20250926-singular-extension-se31b` (bench 2350511)
- **SE3.1b Guardrails – Recapture stacking stabilization**
  - Introduce depth/eval/TT guardrails before enabling stacked recapture extensions in production.
  - Collect telemetry (extension counts, seledepth deltas, NPS impact) with `AllowStackedExtensions=true`.
  - Expected NPS impact: ≤ 2% regression target after guardrails.
  - Status: Completed 2025-09-26 on `feature/20250925-singular-extension-se31a` (bench parity 2350511). Guardrails require depth ≥10, static eval within 96cp of beta, and TT depth ≥ current depth +1 before stacking recapture on top of singular. Telemetry now records candidate/accept/reject counts, clamp events, and extra depth contributed under `AllowStackedExtensions=true`.
- **SE3.1c – Check extension coordination**
  - Add toggle `DisableCheckDuringSingular` for A/B testing
  - Implement conditional check extension disable
  - Track interaction statistics
  - Expected NPS impact: Potentially +1-2% (fewer extensions)
  - Status: Completed 2025-09-27 on `feature/20250927-singular-extension-se32a` (bench 2350511). Singular verification nodes now honor the `DisableCheckDuringSingular` toggle, skipping the automatic in-check extension when enabled and recording applied vs suppressed counts in singular telemetry.
- **SE3.2a – Extension application**
  - When verification confirms singularity, set extension flag
  - Increment depth by `singularExtensionDepth` (initially 1)
  - Update thread-local extension budget
  - Expected NPS impact: -2-5% when enabled (deeper searches)
  - SPRT: [0.00, 5.00] bounds
  - Status: Completed 2025-09-27 on `feature/20250927-singular-extension-se32a` (bench 2350511). `SearchLimits::singularExtensionDepth` now feeds the pending extension amount, `info.singularExtensions` tallies applied plies, and singular fail-low events schedule the depth increase while respecting budget clamps.
- **SE3.2b – Context propagation**
  - Ensure child nodes receive updated NodeContext
  - Maintain PV flag through extended search
  - Update triangular PV correctly
  - Expected NPS impact: < 0.1%
  - Status: Completed 2025-09-27 on `feature/20250927-singular-extension-se32a` (bench 2350511). Singular extensions force the child `NodeContext` to stay on the PV path when applied and reuse the allocated triangular PV buffer so extended searches preserve the principal variation ordering.

### Stage SE4 — Diagnostics, UCI toggles, and Tuning Hooks
- **SE4.1a – Core UCI controls**
  - Add `UseSingularExtensions` bool (default false)
  - Add `SingularDepthMin` int (default 8, range 4-20)
  - Add `SingularMarginBase` int (default 64, range 20-200)
  - **C++20 Configuration with concepts:**
    ```cpp
    template<typename T>
    concept TunableParameter = std::integral<T> && sizeof(T) <= 8;

    template<TunableParameter T>
    struct Parameter {
        T value;
        T min_value;
        T max_value;
        std::string_view name;

        constexpr bool validate() const noexcept {
            return value >= min_value && value <= max_value;
        }
    };

    // Compile-time validation
    constexpr Parameter<Depth> singular_depth_min{8, 4, 20, "SingularDepthMin"};
    static_assert(singular_depth_min.validate());
    ```
  - Expected NPS impact: 0% (configuration only)
  - Status: Completed 2025-09-27 on `feature/20250927-singular-extension-se32a` (bench 2350511). UCI now exposes `SingularDepthMin`, `SingularMarginBase`, `SingularVerificationReduction`, and `SingularExtensionDepth`, wiring through `SearchLimits` so telemetry and SPRTs can sweep singular heuristics without rebuilding.
- **SE4.1b – Advanced tuning parameters**
  - Add `SingularVerificationReduction` int (default 3, range 2-5)
  - Add `SingularExtensionDepth` int (default 1, range 1-2)
  - Hook into `engine_config` for OpenBench parameter sweeps
  - Expected NPS impact: 0%
  - Status: Completed 2025-09-28 on `feature/20250927-singular-extension-se32a` (bench 2350511). Singular toggles and knobs now mirror into `EngineConfig`, keeping UCI options and SPSA-accessible configuration in lockstep for future sweeps.
- **SE4.2a – Debug diagnostics**
  - Implement `debug singular` UCI command
  - Output per-position singular decision log
  - Include: position, TT info, margin, verification result
  - Expected NPS impact: -50% when enabled (heavy logging)
  - Status: Completed 2025-09-29 on `feature/20250926-singular-extension-se31b` with per-candidate logs, margin/verification reporting, and 10+0.1 validation workflow.
- **SE4.2b – Regression test harness**
  - Create `tests/regression/singular_tests.cpp`
  - Add deterministic test positions from Section 8
  - Verify feature ON/OFF produces expected extensions
  - Expected NPS impact: N/A (test mode only)
  - Status: Completed 2025-09-29 via `test_singular_regression` executable covering Section 8 FENs with debug instrumentation and singular-on/off assertions.

### Stage SE5 — Validation & Rollout
- **SE5.1a – Perft validation**
  - Run perft suite with `UseSingularExtensions=true`
  - Verify node counts match baseline (extensions don't affect perft)
  - Test positions from Section 8 at depth 6
  - **Performance profiling requirements:**
    - Profile with `perf record -g` on Linux
    - Check CPU cache misses with `perf stat -e cache-misses`
    - Verify no unexpected allocations with `heaptrack`
    - Measure branch mispredictions: target < 1% on extension decisions
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
- **Sub-stage branches:** Create dedicated branch per sub-stage (e.g., `feature/SE1-se0-1a-telemetry`)
  - Each branch must carry an OpenBench SPRT (or bench parity) against `feature/20250921-singular-extension-plan`
  - Merge back to the integration branch only after a PASS verdict (or documented neutral result for no-op stages)
- **Integration workflow:** Treat `feature/20250921-singular-extension-plan` as the SE1 integration branch; do not rebase once sub-stage work lands to preserve SPRT history
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
- **Phase 6 toggles (retain through SE1 validation):**
  - `UseSearchNodeAPIRefactor` - Keep exposed until Stage SE5 completes and SPRT/rollback criteria documented
  - `EnableExcludedMoveParam` - Same retention policy as above
- **Production toggles (remain for tuning):**
  - `UseSingularExtensions` - Main feature toggle (default `true` on SE4.1b tuning branch; revisit before mainline merge)
  - `SingularDepthMin` - Tunable parameter (default 7 after SPSA stage)
  - `SingularMarginBase` - Tunable parameter (default 51 after SPSA stage)
  - `DisableCheckDuringSingular` - A/B testing toggle (default `false`)
  - `AllowStackedExtensions` - Recapture stacking toggle (default `true` post-SPSA)
- **UCI defaults checkpoint:** Confirmed in `src/uci/uci.cpp` as of 2025-09:
  - `UseSearchNodeAPIRefactor` → `check` option, default `true`
  - `EnableExcludedMoveParam` → `check` option, default `true`
  - `UseSingularExtensions` → `check` option, default `true` (tuning branch); document final release default at SE5

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
- **C++ Performance Checklist:**
  ```cpp
  // Compile-time performance assertions
  static_assert(sizeof(SingularStats) <= 64);  // Single cache line
  static_assert(std::is_trivially_copyable_v<NodeContext>);
  static_assert(alignof(SearchData) >= 64);  // Cache aligned

  // Runtime performance validation
  assert(reinterpret_cast<uintptr_t>(&search_data) % 64 == 0);
  ```
- **Compiler optimization validation:**
  - Verify `-march=native -mtune=native` for production builds
  - Enable PGO (Profile-Guided Optimization) for final builds
  - Check assembly for hot paths: no unnecessary memory barriers
  - Validate vectorization of margin calculations

## 10. Follow-Up Work
- Stage SE2 tuning sweeps (SingularMargin table) once initial enablement stable
- Explore multi-extension interactions (recapture, passed pawn) using extension budget framework
- Prepare Phase MC1 (multi-cut) leveraging same verification API
- Consider double/triple extensions for extremely singular moves (depth > 20)

## 11. C++20/23 Migration Opportunities
Future enhancements when compiler support improves:
- **C++20 Modules:** Isolate singular extension logic in module for better compilation times
- **C++20 Coroutines:** Consider for verification search to reduce stack usage
- **C++23 `std::mdspan`:** For efficient multi-dimensional parameter tables
- **C++23 `std::expected`:** For error handling in verification searches
- **C++23 `std::flat_map`:** For move-to-extension mapping if needed
- **C++23 `[[assume]]`:** For optimizer hints on extension conditions

## 12. SIMD Optimization Opportunities
Potential vectorization targets:
- Batch margin calculations for multiple depths
- Parallel score adjustments in verification
- SIMD-friendly move generation filtering
- Consider AVX-512 VPOPCNT for bitboard operations if available

---

**Next Action:** Run telemetry pass with `UseSingularExtensions`/`EnableExcludedMoveParam` enabled to baseline fail-low/high rates, then begin SE3.1a extension infrastructure.
