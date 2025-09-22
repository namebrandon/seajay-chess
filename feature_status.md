# Phase SE1 - Singular Extension Enablement

## Overview
- Objective: Enable production-ready singular extensions atop the Search Node API refactor without regressing depth parity or NPS.
- Guardrails: All new behaviour remains behind `UseSingularExtensions` (default false) with supporting toggles `UseSearchNodeAPIRefactor=true` and `EnableExcludedMoveParam=true` during development.
- Success Criteria: +10 nELO cumulative across SE stages, ≤10% cumulative NPS regression, and validated diagnostics for future singular tuning work.

## Timeline
- Start Date: 2025-09-21
- Current Branch: `feature/20250921-se01a-telemetry`
- Base Commit: `76c574d347452ed2069e08e9fac6afe063bc8d87`
- Bench Baseline (Release, Desktop): TBC after Stage SE0.1 telemetry capture

## Stage Progress
| Phase | Status | Commit | Bench | Notes |
|-------|--------|--------|-------|-------|
| SingularExtension_Phase_SE0.1a – Thread-local telemetry scaffold | In Progress | (pending) | (pending) | Introduce aligned `SingularStats` in `SearchData`; no functional change, prep for per-thread instrumentation. |
| SingularExtension_Phase_SE0.1b – Global aggregation | Planned | N/A | N/A | Atomics for cross-thread roll-up; info output integration at 1s cadence. |
| SingularExtension_Phase_SE0.2a – UCI toggle exposure | Planned | N/A | N/A | Publish `UseSingularExtensions` toggle; update docs and defaults. |
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
1. Finalize `SingularStats` definition and reset semantics in `SearchData` (current task).
2. Add unit/system-level smoke check to confirm stats remain zero when that code path is unused.
3. Document baseline telemetry once Stage SE0.1b info plumbing is wired.
