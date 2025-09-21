# Phase 6 - Search API Refactor (NodeContext)

## Overview
- Objective: Introduce a zero-overhead `NodeContext` descriptor that cleanly represents PV/root state and the optional excluded move without altering current search behaviour.
- Expected ELO impact (Phase 6): 0 +/- 5 when toggles remain OFF; future phases will leverage this API for potential gains.
- Guardrails: All new pathways remain behind `UseSearchNodeAPIRefactor` / `EnableExcludedMoveParam` toggles, defaulting to OFF.

## Timeline
- Start Date: 2025-09-20
- Current Branch: `integration/phase6-search-api-refactor`
- Base Commit: `73f939cd70b47803955e7ab81d60178f4f62265b` (Stage 6f merge)
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
| 6d - Verification helper | Completed | c048813 | 2350511 | Verification helper scaffold merged; returns neutral score until singular logic arrives. |
| 6e - TT hygiene review | Completed | 3ed5786 | 2350511 | Tightened TT replacement guards to avoid NO_MOVE pollution; documentation refreshed. |
| 6f - PV clarity/root safety | Completed | 73f939c | 2350511 | DEBUG-only NodeContext asserts validated via SPRT (neutral); protects PV and excluded move invariants; toggle-on (`EnableExcludedMoveParam=true`, `UseSearchNodeAPIRefactor=true`) bench run neutral at 2350511 nodes. |
| 6g - Integration cleanup | Completed | HEAD | 2350511 | DEV-mode validation complete; toggle default now ON (`UseSearchNodeAPIRefactor=true`), release bench parity confirmed with sequential build tooling. |

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
| 6e | `cmake --build build --target seajay_core -- -j1` + `cmake --build build --target seajay -- -j1` | 2350511 | TT hygiene guard adjustments; validated with bench parity. |
| 6f | `cmake --build build --target seajay_core -- -j1` + `cmake --build build --target seajay -- -j1` | 2350511 | Added root/PV/excluded context asserts; bench parity confirmed locally; SPRT and toggle-on (EnableExcludedMoveParam/UseSearchNodeAPIRefactor) runs reported neutral outcome. |
| 6g | `./build.sh Release` + `./build.sh Debug` | 2350511 | Sequential make avoids LTO jobserver crash; release bench parity with defaults (UseSearchNodeAPIRefactor=true) 1761049 nps, toggle OFF run 1836718 nps, and DEV-mode ON run (EnableExcludedMoveParam=false) 1747382 nps. |

## Key Learnings / Risks
- LTO jobserver failures traced to inherited `MAKEFLAGS` without descriptors; sequential make invocation in build.sh now provides stable Release/Debug builds.
- With Stage 6g accepted, `UseSearchNodeAPIRefactor` ships enabled by default; toggles remain exposed for rollback/testing.
- `EnableExcludedMoveParam` toggle now mirrors NodeContext state into legacy telemetry without behavioural impact (default OFF).
- Verification helper scaffold returns neutral score while feature remains disabled; stats counters only active in DEBUG builds.
- TT replacement policy now explicitly preserves fresh move-carrying entries when incoming data is a shallow NO_MOVE heuristic, reducing pollution risk.
- DEBUG-only NodeContext asserts now guard root invariants and prevent pruning logic from running when an excluded move is active.

## Next Actions
1. Monitor integration SPRT results with default `UseSearchNodeAPIRefactor=true` and confirm no regressions on OpenBench.
2. Draft Phase SE1 (singular extension enablement) plan once Stage 6 is signed off.
