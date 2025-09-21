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
- **Pre-flight TODO:**
  - Audit `SearchInfo` and `NodeContext` to ensure no lingering legacy `excludedMove` usage once SE1 begins.
  - Confirm quiescence path ignores singular logic (per guardrails).

## 3. Risk Register & Mitigations
| Risk | Impact | Mitigation |
|------|--------|------------|
| False positives trigger frequent singular re-search causing 3-5% NPS loss | High | Tune TT quality threshold, require multi-criteria gating (depth >= 4, TT exact hit, move ordering stability). |
| Interactions with existing check extension cause double-extensions leading to search explosion | High | Add extension-budget tracking per node; temporarily disable check extension during SE1 testing if necessary. |
| TT pollution from low-depth verification search | Medium | Store verification results in separate TT bucket or avoid storing altogether; mark entries with exclusion bit. |
| Verification helper diverges from main search parameters | Medium | Share search limits/time budget struct; unit test verification vs baseline search for identical parameters except excluded move. |
| Increased code complexity without sufficient observability | Medium | Add DEBUG counters (`singularAttempts`, `singularVerified`, `singularExtensionsApplied`), log under `SearchStats`. |

## 4. Staged Implementation Plan
Each stage ends with: `./build.sh Release`, `echo "bench" | ./bin/seajay`, perft spot-check, commit with `bench XXXXX`. SPRT bounds default to `[-3.00, 3.00]` unless noted.

### Stage SE0 — Foundations & Telemetry (No functional change)
- **SE0.1 – Telemetry scaffolding**
  - Add counters to `SearchData` for TT-qualified moves, verification calls, and extensions applied.
  - Extend `info string` diagnostics (guarded by `SearchStats`) to dump singular stats when toggle enabled.
- **SE0.2 – Toggle hardening**
  - Ensure `UseSearchNodeAPIRefactor` and `EnableExcludedMoveParam` are exposed via UCI and persisted in `feature_status.md` template.
  - Add defensive asserts verifying `excluded` flag clears upon node exit.
- **SE0.3 – Legacy cleanup**
  - Remove residual `SearchInfo::excludedMove` usage once NodeContext exclusive path confirmed.

### Stage SE1 — Verification Helper Activation
- **SE1.1 – Implement `verify_exclusion` logic**
  - Use reduced depth `singularDepth = depth - 1 - verificationReduction` with `beta - 1, beta` window.
  - Accept `NodeContext childCtx` to propagate PV/non-PV info correctly.
  - Respect move history pruning toggles (skip if depth <= thresholds, TT flag missing, or move flagged as tactical).
- **SE1.2 – TT hygiene for verification**
  - Decide on TT store policy (likely `NO_STORE` to avoid contamination; optionally use dedicated verification flag).
  - Add instrumentation to confirm verification search doesn’t alter main node TT entry.

### Stage SE2 — Candidate Identification & Qualification
- **SE2.1 – Candidate gating**
  - Implement heuristics mirroring Stockfish/Ethereal: require TT exact hit, depth >= `singularDepthMin`, move hashed as best, unique strong score margin.
  - Compute `singularBeta = moveScore - singularMargin(depth)`; margin table TBD (e.g., {depth>=8: 60, depth>=6: 80, else 100}).
- **SE2.2 – Fail-soft integration**
  - Ensure alpha/beta updates respect fail-soft semantics; clamp singular window accordingly.
  - Provide fallback path if verification search returns <= `singularBeta` (no extension).

### Stage SE3 — Extension Application & Interaction Guards
- **SE3.1 – Extension budget**
  - Add per-node extension cap to prevent stacking (e.g., singular + check <= maxExtensionBudget).
  - Optionally temporarily disable check extension while singular active (toggle controlled for A/B testing).
- **SE3.2 – Apply extension**
  - When verification proves singularity (`score >= singularBeta`), extend search: `depth += singularExtension` (likely +1 ply).
  - Record counters and ensure recursion uses updated NodeContext (propagate PV flag).

### Stage SE4 — Diagnostics, UCI toggles, and Tuning Hooks
- **SE4.1 – UCI controls**
  - Add `UseSingularExtensions` (bool, default false) and optional tuning knobs (`SingularMarginBase`, `SingularDepthMin`).
  - Hook into `engine_config` for default values to allow OpenBench sweeps.
- **SE4.2 – Logging & regression harness**
  - Add optional verbose logging command (`debug singular`) dumping candidate decisions for manual analysis.
  - Integrate into `tests/regression/` harness with deterministic positions verifying on/off parity.

### Stage SE5 — Validation & Rollout
- **SE5.1 – Local validation**
  - Run targeted perft with singular toggle ON to confirm no illegal moves.
  - Depth/time parity runs with 1-thread, 10s per move baseline.
- **SE5.2 – OpenBench SPRT (toggle ON)**
  - Launch `bugfix/20250921-windows-stop-thread` (main) vs `feature/...` with bounds `[0.00, 5.00]` once initial sanity passes.
  - Track singular counters vs ELO gain; adjust margins if regressions.
- **SE5.3 – Windows regression**
  - Rebuild under MSYS2 UCRT64; repeat stop/start stress tests ensuring no regression.
- **SE5.4 – Documentation & handoff**
  - Update `feature_status.md`, `docs/project_docs/Phase6_Search_API_Refactor.md` (append SE1 summary), and add tuning notes.

## 5. Branching & Workflow
- Feature branch prefix: `feature/2025MMDD-singular-ext-phase1` (this plan uses `feature/20250921-singular-extension-plan`).
- Each stage gets its own sub-branch (`feature/SE1-stage-SE0.1`, etc.) merged via PR to maintain review granularity.
- Keep `feature_status.md` updated per Stage Completion Checklist.
- Final merge to `main` only after Stage SE5 SPRT PASS and Windows validation sign-off.

## 6. Testing Matrix
| Scenario | Toggle Config | Target |
|----------|---------------|--------|
| Baseline parity | `UseSingularExtensions=false`, others true | Bench within noise, perft exact |
| Verification sandbox | `UseSingularExtensions=false`, `DEV_ASSERT_SearchAPI=true` | No assertions triggered |
| Singular ON sanity | `UseSingularExtensions=true`, short matches | Verify counters increment, no crashes |
| Full SPRT | `UseSingularExtensions=true` | Bounds `[0.00, 5.00]`, 10+0.1 TC |
| Windows regression | `UseSingularExtensions=true` | Stop/go loop, go depth N + stop |

## 7. Follow-Up Work
- Stage SE2 tuning sweeps (SingularMargin table) once initial enablement stable.
- Explore multi-extension interactions (recapture, passed pawn) using extension budget framework.
- Prepare Phase MC1 (multi-cut) leveraging same verification API.

---

**Next Action:** Kick off Stage SE0.1 telemetry scaffolding on a dedicated feature sub-branch.
