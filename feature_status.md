# Phase 6 - Search API Refactor (NodeContext)

## Overview
- Objective: Introduce a zero-overhead `NodeContext` descriptor that cleanly represents PV/root state and the optional excluded move without altering current search behaviour.
- Expected ELO impact (Phase 6): 0 +/- 5 when toggles remain OFF; future phases will leverage this API for potential gains.
- Guardrails: All new pathways remain behind `UseSearchNodeAPIRefactor` / `EnableExcludedMoveParam` toggles, defaulting to OFF.

## Timeline
- Start Date: 2025-09-20
- Current Branch: `feature/phase6-stage-6b2`
- Base Commit: `ab6816e273129fc524f3024455db2694be3fb06c` (integration/phase6-search-api-refactor tip)
- Bench Baseline: 2350511 nodes (`bin/seajay`, Release build)

## Stage Progress
| Stage | Status | Commit | Bench | Notes |
|-------|--------|--------|-------|-------|
| 6a.1 - NodeContext header | Completed | d8a11a4 | 2350511 | Header-only introduction of packed context struct, no call sites updated yet. |
| 6a.2 - Negamax overload | Completed | 45cd9fa | 2350511 | Added NodeContext overload plus legacy wrapper; behaviour unchanged. |
| 6b.1 - Context through main negamax | Completed | HEAD | 2350511 | Threaded NodeContext through primary negamax recursion with `UseSearchNodeAPIRefactor` toggle defaulting to OFF. |
| 6b.2 - Context through quiescence | Completed | TBD | 2350511 | Quiescence plumbing guarded by NodeContext with parity verified (toggle OFF/ON both 2350511 nodes). |
| 6b.3 - Helper propagation | Completed | TBD | 2350511 | LMR helpers now use NodeContext-aware wrappers; toggle OFF/ON benches match at 2350511 nodes. |
| 6c - Excluded-move plumbing | Pending | - | - | Replace legacy flags with context; SPRT required once toggled. |
| 6d - Verification helper | Pending | - | - | Add DEBUG verification scaffolding. |
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

## Key Learnings / Risks
- Build script reported a `buffer overflow detected` during static library link, but completed successfully; monitor on subsequent stages in case the new header exacerbates existing issue.
- No behaviour change yet; all toggles remain OFF.

## Next Actions
1. Prepare Stage 6c implementation plan (excluded-move plumbing) with updated helper context.
2. Monitor LTO link instability; continue single-threaded link workaround until toolchain update.
