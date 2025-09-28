# Evaluation Overhaul Plan — Post-SE1 Follow-up

## Context & Motivation
- Recent singular-extension work (branches `feature/20250926-singular-extension-se31b` → `feature/20250921-singular-extension-plan`) exposed long-standing evaluation biases.
- Multiple reference FENs in `external/problem_positions.txt` show SeaJay’s static eval disagreeing by 2–6 pawns with top engines (Komodo, Laser, Fritz). Mis-evaluations trigger deeper searches, nullify pruning heuristics, and inflate node counts even before singular verification applies.
- Node explosions observed in `tools/analyze_position.sh` traces stem from the evaluator assigning ±0.3 cp where peers score ±3 cp. Search keeps trying to “prove” the wrong score, so extensions and TT probes thrash.
- Priority: capture these findings now so we can resume after the singular-extension SPRT finishes and merges to `main`.

## Symptom Summary (see `external/problem_positions.txt`)
- **Middlegame king exposure:** Positions `4r1k1/...` and `3r2k1/...` show barely negative scores (−0.4 to −0.6 cp) despite every reference engine reporting −2 cp or worse. `debug eval` reveals: minimal king-safety bonus, PST penalties that overvalue Black’s cramped setup, pawn-structure debits drowning out the attack.
- **Endgame pawn races:** `r1b5/...` and `8/p2r1k1p/...` evaluate near equality (±0.3 cp) while reference engines see ±3 cp. Passed-pawn term dumps +1.2 cp or −2.8 cp without considering blockades, rook placement, or king proximity.
- **Material deficits ignored:** `R3n1k1/...` is only −1.2 cp even though down a bishop; PST/mobility/rook-file terms give back almost half the material deficit.
- **King danger massively undervalued:** In sharp middlegames, `kingSafety` rarely exceeds ±0.2 cp; the existing heuristic doesn’t react to open files or piece pressure.
- **Passed pawn heuristics too optimistic:** Base table hits +180 cp on rank 7 and the “unstoppable” check instantly adds +300 cp when pure king+pawn conditions are misdetected.
- **Pawn penalties uncompensated:** Isolated/doubled pawn penalties fire even when opponent’s pieces are offside; there’s no counter bonus for open files or attacking rook placements.

## Current Tools & References
- **Evaluation tracing:** `setoption name EvalExtended value true` + `debug eval` prints component breakdown (`src/evaluation/eval_trace.h`, `src/evaluation/evaluate.cpp`). Use to quantify suspicious terms.
- **Problem set:** `external/problem_positions.txt` (keep expanding). Each entry lists opponent engine scores and suggested best moves.
- **Singular telemetry & info logs:** `tools/stacked_telemetry.py`, `tests/regression/singular_tests.cpp` remain useful for correlating evaluation flips with search behaviour.
- **Position analyser:** `tools/analyze_position.sh` compares SeaJay vs. other UCI engines at fixed depth/time.
- **Core evaluation modules:**
  - Material & PST: `src/evaluation/evaluate.cpp`, `src/evaluation/pst.cpp`
  - Pawn structure: `src/evaluation/pawn_structure.cpp`
  - King safety: `src/evaluation/king_safety.cpp`
  - Passed-pawn bonuses & unstoppable logic: `src/evaluation/evaluate.cpp` lines ≈260–340 & 360–430

## Proposed Workstream (post-SPRT merge)
1. **Create Evaluation Regression Harness**
   - Add `tests/eval/problem_positions_test.cpp` (or similar) reading `external/problem_positions.txt`.
   - For each FEN, assert SeaJay score falls within reference range (with tolerance, e.g., ±50 cp). Initially mark failing cases to log-only; convert to hard assertions as fixes land.
   - Provide make/ctest target (`ctest -R eval`) for quick runs.

2. **King-Safety Overhaul**
   - Review `king_safety.cpp`; compare with Stockfish’s attack tables.
   - Add weighted contributions for open/half-open files towards the king, attackers counting, pawn shield depletion, and piece proximity.
   - Ensure `debug eval` traces new terms and produce telemetry on attack scores for the problem FENs.

3. **Passed-Pawn Sanity Checks**
   - Cap base bonuses at sensible values (e.g., ≤80 cp before additional modifiers).
   - Add blockers/blockade detection: no bonus when the square in front is occupied or controlled by enemy pieces/king.
   - Integrate rook-behind-pawn and king-support requirements before granting +300 cp “unstoppable” bonus.

4. **Pawn-Structure Compensation & PST Refresh**
   - Modify isolated/doubled penalties to scale with opponent activity: reduce penalty when rooks/queens are passive or when our pieces dominate open files.
   - Re-tune PST tables (SPSA or manual) once king safety and pawn heuristics are fixed.
   - Consider storing separate MG/EG PST blends for open vs. closed positions (phase detection may be insufficient).

5. **Material Deficit Awareness & Piece Activity**
   - Add penalties for undeveloped/poorly placed minor pieces (e.g., knights on rim, bishops blocked by own pawns).
   - Encourage rooks on 7th rank, queens on active squares to compensate for structural weaknesses, closing the gap with reference engines.

6. **Search Interaction Checks**
   - After evaluation adjustments, rerun node-count comparisons (with/without singular extensions, with `tools/analyze_position.sh`) to verify reductions.
   - Revisit null-move, futility margins, and singular verification thresholds if evaluation shifts change fail-high frequency.

## Execution Plan
1. **Branching**
   - Wait for current SPRT (`debf9d1182d8e4e031f36b62be39715dedbbab87` vs. `main`) to finish.
   - Merge singular-extension work to `main` (branch `feature/20250921-singular-extension-plan`).
   - Create new branch `feature/202510XX-eval-overhaul` off the updated `main`.

2. **Phase 0 — Harness & Baseline**
   - Implement evaluation regression test harness and log baseline SeaJay scores vs. references.
   - Capture current `debug eval` outputs for each problem FEN (store in `external/eval_baselines/` for diffing).

3. **Phase 1 — King Safety & Pawns**
   - Land king-safety overhaul; validate on problem FENs and a small tactics/endgame suite.
   - Fix passed-pawn bonuses and add blockade detection.
   - Update harness expectations where improvements materialise.

4. **Phase 2 — PST & Compensation**
   - Rework PST tables / add tuning hooks. Consider re-running SPSA with new evaluation model.
   - Adjust pawn-structure penalties to lean on open-file compensation.

5. **Phase 3 — Cleanup & SPRT**
   - Re-run node-count comparisons (with `tools/analyze_position.sh` and `bench`).
   - Launch SPRT(s) at 10+0.1 to ensure evaluation changes are at least neutral.
   - Merge back to `main` once stable.

## Helpful Commands & Snippets
```bash
# Detailed evaluation breakdown
./bin/seajay <<'EOF'
uci
setoption name EvalExtended value true
position fen <FEN>
debug eval
quit
EOF

# Compare engines on a position
tools/analyze_position.sh -fen "<FEN>" -depth 18 -engines "seajay,komodo,laser"

# Run problem-position harness (once added)
ctest -R eval

# Profiling (post-eval fixes)
# intel VTune: vtune -collect hotspots ./bin/seajay ...
```

## References & Prior Work
- Singular extension plan/investigation: `docs/project_docs/Phase_SE1_Singular_Extension_Plan.md`, `docs/project_docs/SE1_Singular_Extension_Investigation.md`.
- Evaluation modules: `src/evaluation/*`, especially `evaluate.cpp`, `king_safety.cpp`, `pawn_structure.cpp`.
- Problem FEN catalogue: `external/problem_positions.txt` (update as new cases appear).
- Telemetry scripts: `tools/stacked_telemetry.py`, `tools/analyze_position.sh`.

---
*Document prepared 2025‑09‑27 while waiting for singular-extension SPRT completion.*
