# Phase SE1 - Singular Extension Enablement

## Overview
- Objective: Enable production-ready singular extensions atop the Search Node API refactor without regressing depth parity or NPS.
- Guardrails: All new behaviour remains behind `UseSingularExtensions` (default false) with supporting toggles `UseSearchNodeAPIRefactor=true` and `EnableExcludedMoveParam=true` during development.
- Success Criteria: +10 nELO cumulative across SE stages, ≤10% cumulative NPS regression, and validated diagnostics for future singular tuning work.

## Timeline
- Start Date: 2025-09-21
- Current Branch: `feature/20250921-se02a-toggle`
- Base Commit: `76c574d347452ed2069e08e9fac6afe063bc8d87`
- Bench Baseline (Release, Desktop): TBC after Stage SE0.1 telemetry capture

## Stage Progress
| Phase | Status | Commit | Bench | Notes |
|-------|--------|--------|-------|-------|
| SingularExtension_Phase_SE0.1a – Thread-local telemetry scaffold | Completed | 0e924ae | 2350511 | `SingularStats` embedded per thread with cache alignment and zero-cost reset semantics. |
| SingularExtension_Phase_SE0.1b – Global aggregation | Completed | 62f260f | 2350511 | Atomic roll-up of per-thread stats with InfoBuilder reporting gated by telemetry flush cadence. |
| SingularExtension_Phase_SE0.2a – UCI toggle exposure | Completed | HEAD | 2350511 | `UseSingularExtensions` wired through UCI → SearchLimits → SearchData with telemetry/reporting gated behind the toggle. |
| SingularExtension_Phase_SE0.2b – Defensive assertions | Planned | N/A | N/A | DEBUG-only guards for excluded move lifecycle. |
| SingularExtension_Phase_SE0.3 – Legacy cleanup | Planned | N/A | N/A | Remove legacy `SearchInfo::excludedMove` plumbing. |

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
1. Capture quick bench with `UseSingularExtensions=true` to confirm parity before SE0.2b.
2. Draft defensive assertion coverage plan for SE0.2b.
3. Refresh public docs/release notes with new toggle description prior to enabling feature work.
