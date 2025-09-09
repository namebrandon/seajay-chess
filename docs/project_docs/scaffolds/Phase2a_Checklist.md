Title: Phase 2a Checklist – Ranked MovePicker (Scoring + Shortlist)

Purpose
- Guard against regressions by enforcing one-change-per-step discipline and Makefile/LTO parity.
- Use this checklist before pushing and before starting each SPRT.

Global Preconditions (apply to all 2a sub‑phases)
- [ ] Release build parity: Makefile with `-O3 -flto -march=native` (same as OpenBench)
- [ ] Diagnostics OFF in Release (no validation/printing on hot paths)
- [ ] UCI options consistent across base/dev (Hash=128MB, Threads=1, UseClusteredTT=true)
- [ ] Root: ranked picker disabled (ply==0)
- [ ] QSearch: ranked picker disabled (captures/promotions only)
- [ ] TT move: yielded first and exactly once; dedup by exact move code (incl. promotion)
- [ ] Arrays: no dynamic allocations; fixed-size arrays have bounds checks in debug builds
- [ ] Commit message includes exact `bench <number>`

2a.0 – Scaffolds only
- [ ] Add `UseRankedMovePicker` UCI toggle (default false); no behavior change
- [ ] Clean build; bench recorded; no Elo test yet

2a.1 – TT‑only sanity
- [ ] Yield TT first (no other ordering changes)
- [ ] Coverage: yielded_count == generated_count (debug-only sampler)
- [ ] SPRT vs Phase 1 end (bounds [-3.00, 3.00])

2a.2 – Captures‑only shortlist (K small), remainder legacy
- [ ] Top‑K captures in shortlist (MVV‑LVA, no SEE)
- [ ] Remainder uses legacy orderer (captures MVV‑LVA; quiets killers/history)
- [ ] In‑check nodes still use check evasions (legacy path OK)
- [ ] SPRT vs Phase 1 end

2a.3 – Add quiets and promotions to shortlist
- [ ] Shortlist includes quiets (killers/countermoves/history) and non‑capture promotions (strong bonus)
- [ ] K≈8–10; deterministic tie‑breaks (score, then MVV‑LVA proxy, then move code)
- [ ] Remainder still legacy ordered
- [ ] SPRT vs Phase 1 end

2a.4 – In‑check parity inside RankedMovePicker
- [ ] When in check, generate check evasions (not all pseudo‑legal moves)
- [ ] Never iterate non‑evasions when in check
- [ ] SPRT vs Phase 1 end

2a.5 – Determinism and bounds
- [ ] Assert shortlist/QS indices within bounds in debug
- [ ] No reads of uninitialized shortlist entries
- [ ] SEARCH_STATS/DEBUG_* ifdefs consistent across TUs (avoid LTO/ODR issues)
- [ ] SPRT vs Phase 1 end

2a.6 – Minimal telemetry (compiled‑out in Release)
- [ ] Best‑move rank distribution histogram
- [ ] SEE call counter (should stay low or 0 in 2a)
- [ ] First‑move cutoff and PVS re‑search rates (existing)
- [ ] SPRT vs Phase 1 end

2a.7 – QS remains legacy (final verify)
- [ ] Confirm no ranked QS path is active (one‑time assert/log in dev build)
- [ ] Short local tactical/bench sanity
- [ ] SPRT vs Phase 1 end (final 2a sign‑off)

OpenBench Run Config (per SPRT)
- Base: Phase 1 end commit (e.g., `b700e88`), no `UseRankedMovePicker`
- Dev: current 2a sub‑phase branch, `UseRankedMovePicker=true`
- Common: `Hash=128`, `Threads=1`, `UseTranspositionTable=true`, `UseClusteredTT=true`, diagnostics OFF
- TC/book: `10+0.1`, `UHO_4060_v2`; bounds `[-3.00, 3.00]` (nELO)

Sanitizers (local, non‑LTO)
- Address/Undefined: `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` (disable LTO)
- Run short selfplay/tactical; fix any UB/overflows before SPRT

