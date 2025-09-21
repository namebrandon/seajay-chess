# Phase 6 - Search API Refactor (NodeContext)

## Overview
- Objective: Introduce a zero-overhead `NodeContext` descriptor that cleanly represents PV/root state and the optional excluded move without altering current search behaviour.
- Expected ELO impact (Phase 6): 0 +/- 5 when toggles remain OFF; future phases will leverage this API for potential gains.
- Guardrails: All new pathways remain behind `UseSearchNodeAPIRefactor` / `EnableExcludedMoveParam` toggles, defaulting to OFF.

## Timeline
- Start Date: 2025-09-20
- Current Branch: `feature/phase6-stage-6d`
- Base Commit: `ab6816e273129fc524f3024455db2694be3fb06c` (integration/phase6-search-api-refactor tip)
- Bench Baseline: 2350511 nodes (`bin/seajay`, Release build)

## Stage Progress
| Stage | Status | Commit | Bench | Notes |
|-------|--------|--------|-------|-------|
| 6a.1 - NodeContext header | Completed | d8a11a4 | 2350511 | Header-only introduction of packed context struct, no call sites updated yet. |
| 6a.2 - Negamax overload | Completed | 45cd9fa | 2350511 | Added NodeContext overload plus legacy wrapper; behaviour unchanged. |
| 6b.1 - Context through main negamax | Completed | 725dc61 | 2350511 | Threaded NodeContext through primary negamax recursion with `UseSearchNodeAPIRefactor` toggle defaulting to OFF. |
| 6b.2 - Context through quiescence | Completed | 1460ac9 | 2350511 | Quiescence plumbing guarded by NodeContext with parity verified (toggle OFF/ON both 2350511 nodes). |
| 6b.3 - Helper propagation | Completed | d8e7c58 | 2350511 | LMR helpers now use NodeContext-aware wrappers; toggle OFF/ON benches match at 2350511 nodes. |
| 6c - Excluded-move plumbing | Completed | dac2c68 | 2350511 | NodeContext drives excluded move toggle; legacy stack mirrors via `EnableExcludedMoveParam` (default OFF). |
| 6d - Verification helper | In Progress | WIP | 2350511 | Implementing disabled verification helper scaffold for singular extensions. |
| 6e - TT hygiene review | Pending | - | - | Document/store safeguards. |
| 6f - PV clarity/root safety | Pending | - | - | Assert PV/root propagation invariants. |
| 6g - Integration cleanup | Pending | - | - | Final toggles + documentation sweep. |

## Testing Summary
| Stage | Build | Bench | Notes |
|-------|-------|-------|-------|
| 6a.1 | `./build.sh Release` | 2350511 | Build succeeded with existing warnings (TT loop signedness, SEE unused vars, misleading indentation). |
| 6a.2 | `./build.sh Release` | 2350511 | NodeContext overload compiles cleanly; legacy wrapper verified via bench parity. |
| 6b.1 | `./build.sh Release` | 2350511 | Context threaded through core recursion; bench parity maintained (toggle OFF). `bench` also verified with toggle ON. |
| 6b.2 | `cmake --build build --target seajay -- -j1` | 2350511 | Initial LTO link hit glibc buffer-overflow guard; rerunning link single-threaded succeeded, and bench parity held with toggle OFF/ON. |
| 6b.3 | `cmake --build build --target seajay_core -- -j1` + `cmake --build build --target seajay -- -j1` | 2350511 | Required single-threaded LTO link steps; bench parity maintained with toggle OFF/ON. |
| 6c | `cmake --build build --target seajay_core -- -j1` + `cmake --build build --target seajay -- -j1` | 2350511 | Added `EnableExcludedMoveParam` toggle; bench parity confirmed with both toggles OFF and ON (2350511 nodes). |
| 6d | `cmake --build build --target seajay_core -- -j1` + `cmake --build build --target seajay -- -j1` | 2350511 | New verification helper compiles cleanly (NoOp); bench unchanged. |

## Key Learnings / Risks
- Build script reported a `buffer overflow detected` during static library link, but rerunning the single-threaded LTO link succeeds; continue monitoring toolchain instability.
- `EnableExcludedMoveParam` toggle now mirrors NodeContext state into legacy telemetry without behavioural impact (default OFF).
- Verification helper scaffold returns neutral score while feature remains disabled; stats counters only active in DEBUG builds.

## Next Actions
1. Flesh out Stage 6d helper documentation and prepare merge notes once scaffold reviewed.
2. Draft plan for Stage 6e TT hygiene adjustments now that verification hook exists.
