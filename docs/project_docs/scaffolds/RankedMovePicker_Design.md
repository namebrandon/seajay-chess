Title: Ranked MovePicker – Design and Integration Scaffold

Context
- Phase 2 redesign following regressions from staged MovePicker.
- Goal: Improve early decision quality with minimal overhead and safe gating.

Design Summary
- TT move first (deduped), then a small top‑K shortlist from a single‑pass scoring of all legal moves, followed by remaining captures and remaining quiets.
- Rank‑aware pruning gates: for rank ≤ R(depth), bypass futility and move‑count pruning and set LMR reduction = 0; enable pruning gradually beyond R.
- Root safety: keep legacy ordering at ply 0 (or larger R) until A/B PASS.

Scoring (lightweight)
- Captures: SEE score (MVV‑LVA fallback) + promotion bonus + small check bonus.
- Quiets: killers, countermoves, history/refutation‑history; optional check bonus.
- Single‑pass maintenance of a fixed‑size top‑K buffer (K≈8–12) without heap allocations.

Integration Plan (Phases)
1) 2a – Scoring + Shortlist
   - Implement scoring and top‑K extraction, leave pruning unchanged.
   - Return order: TT → shortlist (sorted) → remaining captures (SEE≥0 then others) → remaining quiets.
   - Acceptance: legality parity with generator set; PV stable; neutral Elo.

2) 2b – Rank‑Aware Pruning Gates
   - Thread rank info into negamax for MCR, futility, LMR decisions.
   - R(depth) = clamp(4 + depth/2, 6–12) initial heuristic.
   - Acceptance: improved first‑move cutoff, reduced PVS re‑search; non‑regression SPRT at 10+0.1.

3) 2c – Capture Handling Refinements
   - SEE used for ordering; do not outright discard SEE<0 in tactical replies (recaptures/checks allowed).
   - Acceptance: tactical suite unchanged; neutral nodes.

4) 2d – ID/PV Bias
   - Boost prior‑depth PV and TT refutations into shortlist.
   - Acceptance: PV stability improves; non‑regression SPRT.

5) 2e – Micro‑optimizations
   - Unroll tight loops; static arrays; avoid atomics/copies/allocations; confirm O(n).
   - Acceptance: bench/NPS improvement without Elo tradeoff.

Telemetry & Diagnostics
- Best‑move rank distribution; first‑move cutoff rate; PVS re‑search rate.
- Fraction of top‑K quiets pruned/reduced (target ≈ 0%); LMR reductions by rank.
- Node/score at fixed depths for a small FEN suite.

UCI Toggle (scaffold only)
- UseRankedMovePicker (default: false). Print/accept setoption; no runtime behavior change until Phase 2 code lands.

Testing & A/B
- SPRT vs Phase 1 end at 10+0.1, Hash=128MB, UHO_4060_v2.
- Bounds: start with [-3.00, 3.00] (non‑regression). Promote to [0.00, 5.00] if results suggest a gain.

Risks & Mitigations
- Overfitting via large R/K: keep modest defaults (K≈8–12, R≈6–12) and validate on tactical suites.
- Hidden overhead: keep O(n) and avoid dynamic allocations; ensure debug counters are compiled‑out in Release.

Notes
- This scaffold is documentation‑only; no functional changes. Actual implementation will follow after plan review.

