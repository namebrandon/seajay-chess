# Phase SE1 - Singular Extension Enablement

## Overview
- Objective: Enable production-ready singular extensions atop the Search Node API refactor without regressing depth parity or NPS.
- Guardrails: All new behaviour remains behind `UseSingularExtensions` (default false) with supporting toggles `UseSearchNodeAPIRefactor=true` and `EnableExcludedMoveParam=true` during development.
- Success Criteria: +10 nELO cumulative across SE stages, ≤10% cumulative NPS regression, and validated diagnostics for future singular tuning work.

## Timeline
- Start Date: 2025-09-21
- Current Branch: `feature/SE1-se1-1a-verify-skeleton`
- Base Commit: `76c574d347452ed2069e08e9fac6afe063bc8d87`
- Bench Baseline (Release, Desktop): TBC after Stage SE0.1 telemetry capture

## Stage Progress
| Phase | Status | Commit | Bench | Notes |
|-------|--------|--------|-------|-------|
| SingularExtension_Phase_SE0.1a – Thread-local telemetry scaffold | Completed | 0e924ae | 2350511 | `SingularStats` embedded per thread with cache alignment and zero-cost reset semantics. |
| SingularExtension_Phase_SE0.1b – Global aggregation | Completed | 62f260f | 2350511 | Atomic roll-up of per-thread stats with InfoBuilder reporting gated by telemetry flush cadence. |
| SingularExtension_Phase_SE0.2a – UCI toggle exposure | Completed | HEAD | 2350511 | `UseSingularExtensions` wired through UCI → SearchLimits → SearchData with telemetry/reporting gated behind the toggle. |
| SingularExtension_Phase_SE0.2b – Defensive assertions | Completed | 95229b3 | 2350511 | RAII guard + DEBUG asserts ensure excluded move lifecycle stays clean on all negamax exit paths. |
| SingularExtension_Phase_SE0.3 – Legacy cleanup | Completed | 0fab434 | 2350511 | Removed SearchInfo excluded-move plumbing; NodeContext now exclusively manages lifecycle. |
| SingularExtension_Phase_SE1.1a – Verification helper skeleton | Completed | 4a62f46 | 2350511 | `verify_exclusion` now guards on `UseSingularExtensions`/`EnableExcludedMoveParam` and logs bypass/invoked counts in DEBUG. |
| SingularExtension_Phase_SE1.1b – Depth reduction clamp | Completed | ec7f07c | 2350511 | Clamp verification depth to `depth - 1 - 3`; DEBUG `ineligible` counter tracks early exits. |
| SingularExtension_Phase_SE1.1c – Window narrowing | Completed | 91ab2ce | 2350511 | Compute singular verification window `[beta-1, beta]` with clamping; child context wiring deferred to SE1.1d. |
| SingularExtension_Phase_SE1.1d – Negamax recursion hookup | Completed | c912525 | 2350511 | Wire verification search into `negamax` using excluded child context and narrow window; still returns score to caller. |
| SingularExtension_Phase_SE1.2a – TT store policy | Completed | 83e92c5 | 2350511 | Verification mode uses `StorePolicyGuard`; TT entries marked `TT_EXCLUSION` reside only in empty/flagged slots and are first to be evicted by primary stores. |
| SingularExtension_Phase_SE1.2b – TT contamination guards | Completed | 1b185fa | 2350511 | DEBUG sentinel/asserts guard verification stores; TT stats track verification store/skip counters for telemetry. |
| SingularExtension_Phase_SE2.1a – TT probe gate | Completed | a586d04 | 2350511 | TT probe now validates depth/EXACT/move before counting singular candidates (telemetry via `singularStats`). |
| SingularExtension_Phase_SE2.1b – Score margin calculation | Completed | a157286 | 2350511 | Added constexpr `singular_margin` table and TT-driven windowing; `verify_exclusion` now clamps `singularBeta` via margin-based subtract. |
| SingularExtension_Phase_SE2.1c – Move qualification | Completed | 8cb8359 | 2350511 | TT move legality/quiet filters populate `singularStats` qualified/reject counters; NodeContext primed for exclusion until verification wiring lands. |
| SingularExtension_Phase_SE2.2a – Verification trigger | Completed | e29f0d9 | 2350511 | Launch verification search with margin-clamped window; record `verificationsStarted` ahead of SE2.2b comparisons. |
| SingularExtension_Phase_SE2.2b – Verification outcome tracking | In progress | (pending) | 2350511 | Classify verification score vs `singularBeta`, populate fail-low/fail-high telemetry, and keep extension flagging deferred to SE3. |

## Telemetry Checklist
| Machine | Branch/Commit | Bench Nodes | Threads | Raw NPS | Normalized NPS (`NPS / bench`) | Depth @10s | TT Hit % | Notes |
|---------|---------------|-------------|---------|---------|-------------------------------|------------|----------|-------|
| Desktop | (pending) | (pending) | 1 / 2 / 4 / 8 | (pending) | (pending) | (pending) | (pending) | Idle load assumption |
| Laptop | (pending) | (pending) | 1 / 2 / 4 | (pending) | (pending) | (pending) | (pending) | Document battery/thermal state |

## Risk Notes
- Telemetry counters must stay cache-aligned; verify with `static_assert(alignof(SingularStats) == 64)` before enabling instrumentation.
- Ensure Release builds strip telemetry when toggle disabled to avoid exceeding ≤2% NPS budget for SE0 stages.
- Cross-machine baseline comparisons rely on normalized NPS; capture bench outputs alongside raw NPS for each data point.

## Next Actions
1. Capture telemetry with singular toggles enabled to baseline qualified/rejection/fail-low/high counters.
2. Begin SE3.1a extension infrastructure once telemetry confirms verification signal health.
