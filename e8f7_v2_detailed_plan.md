e8f7 – V2 Detailed Plan (Do, Measure, Decide)

Objective

- Stabilize move choice on the e8f7 FEN and reduce node explosion without losing Elo.
- Validate quiescence improvements (Option A kept) and identify remaining drivers in main search and TT interplay.

Scope and Non‑Goals

- Scope: Configuration sweeps, diagnostics, and A/B test design. No core code changes in this plan; changes are proposed as toggleable experiments to run offline.
- Non‑Goals: Immediate refactors or broad pruning changes without data; perft/eval changes not directly tied to e8f7.

Key Assets

- Problem FEN: `r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17`
- Tools: `tools/analyze_position.sh`, `tools/node_explosion_diagnostic.sh`
- UCI controls (assumed available): `UseQuiescence`, `QSearchNodeLimit`, `MaxCheckPly`, `SEEPruningMode`, `UseAspirationWindows`, `AspirationWindow`, `AspirationMaxAttempts`.

Metrics to Capture (per run)

- PV per depth, selected move, score, nodes, nps, tthits, moveeff, ebf, seldepth.
- For targeted runs: alpha/beta re-search incidence (aspiration), quiescence entry count, qsearch nodes.
- For TT diagnosis (later): first TT store for chosen root move (bound, depth), root TT hits that set best move.

Phase 1 — Configuration Sweeps (No Code Changes)

1) Baseline confirmation
- Purpose: Reconfirm Option A effect and current persistence of e8f7.
- Command example (depth sweep 4/6/8/10):
  - `./tools/analyze_position.sh "r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17" depth 8`
- Capture PV, nodes, tthits, moveeff, ebf at each depth.

2) Quiescence check extension sweep
- Hypothesis: Lower `MaxCheckPly` reduces the in‑check bias and node growth.
- Settings: `MaxCheckPly ∈ {2,3,4,6}` with `UseQuiescence=true`.
- Command template:
  - `echo -e "uci\nsetoption name UseQuiescence value true\nsetoption name MaxCheckPly value 3\nposition fen <FEN>\ngo depth 8\nquit" | ./bin/seajay`
- Measure: PV stability (avoid e8f7 at depths 4–10), nodes, ebf.
- Success criteria: PV stabilized away from e8f7 on this FEN; node reduction ≥10% vs default; Elo-neutral risk to be checked later.

3) SEE pruning mode sweep (captures)
- Hypothesis: Aggressive SEE ordering/pruning reduces deep tactical tails.
- Settings: `SEEPruningMode ∈ {off, conservative, aggressive}`.
- Command template:
  - `echo -e "uci\nsetoption name SEEPruningMode value aggressive\nposition fen <FEN>\ngo depth 8\nquit" | ./bin/seajay`
- Measure: Node deltas, PV changes, any move flip regression.

4) Aspiration window stability
- Hypothesis: Tighter aspirations increase root stability and reduce oscillation back to king moves.
- Settings: `UseAspirationWindows=true`, sweep `AspirationWindow` (e.g., 8→13→18) and `AspirationMaxAttempts` (e.g., 3→5); compare with `UseAspirationWindows=false`.
- Measure: PV re-search count, stability (bestmove changes across depths), nodes.

5) Quiescence off/on sanity
- Purpose: Contrast PV/nodes with `UseQuiescence=false` to validate the remaining influence post-Option A.
- Note: Do not ship this; use only for diagnostics.

6) Node explosion mini-suite
- Use `tools/node_explosion_diagnostic.sh` with a tactical‑heavy set including the problem FEN; run depths 5/7/10.
- Output: CSV + report; track SeaJay vs peer engines if locally available; otherwise use SeaJay-on-SeaJay ratios across configs.

Phase 1 Data Table (suggested CSV columns)

- `run_id, cfg_label, depth, bestmove, score_cp, nodes, nps, tthits, moveeff, ebf, seldepth, use_q, max_check_ply, see_mode, aw_enabled, aw_window, aw_attempts`

Phase 1 Decision Gates

- Gate A: If `MaxCheckPly=3` improves PV stability and reduces nodes ≥10% with no obvious tactical misses on a short tactical suite, promote as Candidate 1 for OB.
- Gate B: If SEE aggressive yields additional node savings without destabilizing PV on this FEN, include in Candidate 1b.
- Gate C: If aspiration tweaks reduce root PV oscillation (fewer aspiration re-searches, fewer PV flips), include in Candidate stabilization config for OB.

Phase 2 — Targeted A/B Experiments (Toggle-Backed; Implement Offline)

Note: These are implementation directions; keep changes behind UCI flags for A/B. Do not implement until approved.

1) Main-search in-check evasion ordering (Priority: HIGH)
- Idea: Mirror Option A in main search: in check, prefer capture-checker and blocks before king moves; optionally SEE-order evasions.
- Files: `src/search/negamax.cpp` (move generation/ordering for in-check nodes).
- UCI toggle: `InCheckEvasionOrder=king_last|capture_first|see_evasion` (design suggestion).
- Tests: Problem FEN depth sweep; tactical mini-suite; track PV stability, nodes, moveeff.

2) Root quiet re-ranking (Priority: MEDIUM)
- Idea: Strengthen root penalty for king moves and small boost for developing/centralizing moves.
- Files: `src/search/negamax.cpp` root (ply==0) quiet move scoring block.
- UCI toggle: `RootKingPenalty` (int), default current; test +100 to +200 penalty relative to current.
- Tests: Depth 4–10 PV; ensure no regressions on non-tactical positions.

3) Main-search check extension caps (Priority: MEDIUM)
- Idea: Limit consecutive check extensions in main search (separate from quiescence), e.g., cap to 1–2 in a row or taper with ply.
- Files: `src/search/negamax.cpp` (check extension logic).
- UCI toggle: `MaxConsecutiveCheckExtensions`.
- Tests: Problem FEN + tactical suite; ensure no tactic blindness by running known check-mates tests if available.

4) Light root verification for king moves (Priority: LOW)
- Idea: For root king candidates only, run a narrow verification re-search before accepting as best.
- Files: `src/search/negamax.cpp` root selection.
- UCI toggle: `RootKingVerify` (bool) and `RootKingVerifyWindow` (cp).
- Tests: Time impact, PV correctness; likely beneficial only if 1–2 such moves per iteration.

Phase 3 — TT Interaction Diagnostics (Add Logging Only; Implement Offline)

Goal: Confirm whether early TT stores/hits lock in e8f7.

1) Root TT store/hit tracer
- Add optional logging at root for:
  - First TT store for the eventual best move: `bound, depth, move_type (king/capture/block/quiet)`.
  - Root TT hit that sets bestmove early in iterative deepening.
- Files: `src/search/negamax.cpp` around TT probe/store and root selection.
- UCI toggle: `TraceTTAtRoot` (bool) and `TraceFilterKingMoves` (bool) to reduce noise.

2) Replacement policy sampling
- Log replacement reasons (depth vs gen) for the FEN runs; verify shallow entries don't dominate in check-heavy positions.
- Files: TT interface `src/core/transposition_table.*` (if simple to hook behind a debug toggle).

OpenBench (OB) Candidates and Settings

- Candidate 1: `MaxCheckPly=3` with Option A kept; SEE mode default. Bounds: `[-3.00, 3.00]` (efficiency change). TC: 10+0.1, Threads=1, Hash=16–32MB.
- Candidate 1b (if Phase 1 supports): Candidate 1 + `SEEPruningMode=aggressive`. Bounds: `[-3.00, 3.00]`.
- Candidate 2: Main-search in-check evasion ordering toggle ON (capture/block/king), Option A kept. Bounds: `[0.00, 5.00]`. Consider 60+0.6 for stability.
- Candidate 3: Root quiet re-ranking (stronger root king penalty) toggle ON. Bounds: `[0.00, 5.00]`.

OB Hygiene

- Include bench count in commits (`bench <number>`). Use branch names per Git strategy.
- Describe toggles and defaults in the OB description; ensure dev vs base branch differ by only the intended toggles to isolate effect.

Success Criteria

- FEN behavior: No e8f7 flip at depths 4–10 unless objectively best by deeper verification; PV convergence toward d5d4/c8d7.
- Efficiency: ≥10% node reduction on the FEN and ≥5% on the tactical set (post-Option A baseline), without ELO loss.
- Strength: OB shows neutrality or gain relative to main.

Risks and Safeguards

- Reducing check extensions risks tactic misses. Safeguard with targeted tactical suites and possibly adaptive caps (e.g., reduce at high qply only).
- Changing evasion ordering affects many positions. Use UCI toggle and A/B before adoption.
- Over-aggressive SEE pruning can cause blindness. Start with diagnostics indicating SEE has headroom, then test incrementally.

Execution Timeline (Suggested)

Week 1 (Phase 1):
- Run config sweeps + node explosion mini-suite; aggregate CSV; produce a summary with PV stability and node ratios.

Week 2 (Phase 2 prep):
- Implement toggles offline; run A/B locally on the FEN + tactical set; select 1–2 best candidates.

Week 3 (OB):
- Launch Candidate 1 (and possibly 1b) on OB. If neutral/positive and PV stabilizes, proceed to Candidate 2.

Reporting Templates

- Phase 1 summary entry:
  - `cfg_label: <desc>`
  - `depths: [4,6,8,10]`
  - `bestmove@8: <move>`
  - `nodes@8: <n>` `tthits@8: <n>` `moveeff@8: <pct>` `ebf@8: <x>`
  - `notes: PV stable/unstable; e8f7 persisted?`

- OB report entry:
  - `Candidate: <id>`
  - `Config: <toggles/values>`
  - `SPRT: <bounds>, TC: <tc>, Threads/Hash`
  - `Result: Elo X ± Y (LLR ...)`
  - `Decision: keep/drop`

Next Actions

1) Execute Phase 1 sweeps and share CSV + summary.
2) If Gate A passes, green‑light Candidate 1 for OB; otherwise proceed to Phase 2 A/B toggles offline and report back.




================================================================================
EXECUTION UPDATES
================================================================================

## 2025-09-06 Update

### Phase 1 - COMPLETED
All configuration sweeps completed with following results:
- MaxCheckPly variations: No impact (position not in check)
- SEE pruning: Minimal impact (1.8% improvement with SEE off)
- Aspiration windows: Critical for efficiency (38% node reduction)
- Quiescence: Key differentiator (different moves with/without)
- Decision: Proceed to Phase 2 code changes

### Phase 2.1 - COMPLETED AND FAILED
**Implementation:** In-check evasion ordering
- Branch: bugfix/nodexp/20250906-e8f7-v2
- Commit: 2caec0c (REVERTED after failure)
- UCI Option: InCheckEvasionOrdering (default: false)
- Test results: 17.8% node reduction at depth 8 in test positions

**SPRT Test 459 Results:**
- Elo: -11.68 ± 8.09 (95% CI)
- LLR: -2.97 (failed, crossed lower bound -2.94)
- Games: 3958 (W: 1152, L: 1285, D: 1521)
- **Verdict: FAILED** - Significant strength loss, feature reverted

**Analysis:** Deprioritizing king moves in check positions hurt tactical play. The node reduction came at too high a cost.

### Phase 2.2 - COMPLETED AND TESTED
**Implementation:** Root quiet re-ranking with toggleable king penalty
- Branch: bugfix/nodexp/20250906-e8f7-v2
- Commits: edfc210, 1f9f7ae
- UCI Option: RootKingPenalty (default: 0, range: 0-1000)
- Changes: Made existing hardcoded -200 penalty toggleable, excluded castling from penalty

**SPRT Test Results:**
| Test | Penalty | Elo | LLR | Games | Verdict |
|------|---------|-----|-----|-------|---------|
| 460 | 0 | +0.97 ± 7.61 | -0.21 | 4298 | NEUTRAL |
| 462 | 100 | -2.80 ± 7.86 | -1.11 | 4336 | SLIGHTLY NEGATIVE |

**Local Testing (Problem FEN at depth 8):**
- Penalty=0: Chooses b7b6, 24,759 nodes
- Penalty=200: Chooses e8f7, 30,805 nodes
- Penalty=400: Still chooses e8f7 (move evaluation overcomes penalty)

**Decision:** Set default to 0 (no penalty) as optimal based on SPRT results.

## Key Learnings from Phase 2

1. **Root-level fixes insufficient:** The e8f7 problem persists despite move ordering changes
2. **Position analysis:** The position is NOT in check, limiting effectiveness of check-based solutions
3. **Move quality:** e8f7 may genuinely be a good move that overcomes reasonable penalties
4. **Search depth:** The issue appears deeper in the search tree, not at root level
5. **Strength vs Efficiency tradeoff:** Aggressive ordering changes can hurt tactical accuracy

## Current Status

**Branch:** bugfix/nodexp/20250906-e8f7-v2
**Latest Commit:** 1f9f7ae
**Bench:** 19191913
**UCI Defaults:** RootKingPenalty=0 (no penalty)

## Recommendations for Next Steps

Given that Phase 2.1 failed and Phase 2.2 showed neutral results:

1. **Phase 2.3: Main-search check extension caps** (Priority: MEDIUM)
   - Could help with node explosion in tactical positions
   - Less likely to hurt playing strength than move ordering changes

2. **Phase 3: TT Interaction Diagnostics** (Priority: HIGH)
   - Add logging to understand why e8f7 gets locked in
   - May reveal the root cause of the persistence

3. **Alternative Approach: Deeper search analysis**
   - Focus on search behavior at depths 3-6 where e8f7 gets established
   - Consider position-specific heuristics

The data suggests the problem is not at the root level but deeper in the search tree, possibly in the interaction between TT, quiescence, and move ordering at intermediate depths.