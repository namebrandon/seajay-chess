# Shared Bitboard Evaluation Refactor Plan

## Purpose
Centralize attack/feature computation inside SeaJay's evaluator so every subsystem (pawns, pieces, threats, king safety, telemetry) consumes a shared bitboard context instead of repeatedly calling `MoveGenerator::get*Attacks` / `isSquareAttacked`. This aligns with the Perseus-style pipeline, trims the 10–12% hotspot we observe in `isSquareAttacked`, and creates a foundation for future evaluation features.

## Motivation & Current Pain Points
- **Performance:** Callgrind (2025-10-05) shows `computeKingDangerComponents` + `isSquareAttacked` driving ~1.9B + 1.0B instructions when `EvalKingDangerIndex` is enabled. Many calls are redundant because each feature recomputes attacks.
- **Fragmented data flow:** Pawn, king, threat, and mobility code all rebuild similar masks, making correctness changes risky and telemetry noisy.
- **Telemetry cost:** Enabling logs often forces extra loops (e.g., queen safe-check telemetry), harming SPRT runs.
- **Extensibility:** Future work (king storms, threat blends, SPSA sweeps) will be easier if attack data is precomputed once per evaluation.

## High-Level Architecture
1. **EvalContext Backbone**
   - Build a struct populated at the top of `evaluateImpl`.
   - Contents: per-color attack bitboards by piece type, pawn attack spans, multi-attack masks, defended/undefended sets, king rings/outer rings/flanks, pinned masks, mobility areas, pawn span metadata, threat masks, precomputed `kingCheckers` arrays, etc.
   - Provide lightweight accessors for features/telemetry without exposing raw board.

2. **Modular Feature Passes**
   - Refactor evaluation into modules (e.g. `computePawnFeatures`, `computePieceMobility`, `computeThreats`, `computeKingDanger`). Each receives the `EvalContext` and returns a `Score` + optional telemetry struct.
   - Modules avoid direct calls to `MoveGenerator::isSquareAttacked` except where legality is truly needed (e.g., SEE, quiescence).

3. **Telemetry Integration**
   - Record intermediate counters inside the context so toggling telemetry only serializes existing data. No extra loops when logging is disabled.

4. **Feature Table / Mask Driven Ops**
   - Replace per-piece loops with bitboard intersections + popcounts wherever possible (e.g., rook on open file, pawn phalanx counts, king flank pressure).
   - Maintain small helper functions for repeated patterns (mask & popcount, attack accumulation, safe-check filtering).

## Refactor Phases
1. **Context Prototype (Phase A)**
   - Introduce `EvalContext` built in `evaluateImpl` without changing feature logic; populate with existing data we already compute (material, occupancy, king squares, pawn spans).
   - Wire it into a single module (likely pawn struct) as a proof of concept.

2. **Attack Aggregation (Phase B)**
   - Populate context with per-piece attack bitboards, multi-attack masks, pawn attack fields, pinned masks. Leverage `MoveGenerator::get*Attacks` centrally and cache results.
   - Add unit/bench tests to confirm no behaviour drift when context is unused.

3. **King Danger Rewrite (Phase C)**
   - Port `computeKingDangerComponents` and telemetry to use context bitboards (ring/outer/flank counts, pawn storms, safe-check popcounts).
   - Remove `isSquareAttacked` loops from king danger.

4. **Threat & Mobility Migration (Phase D)**
   - Update threat scoring, rook/queen mobility, pawns-on-files, outpost checks, etc., to read the shared masks.
   - Eliminate duplicated attack recomputation in evaluation modules.

5. **Cleanup & Telemetry Pass (Phase E)**
   - Ensure telemetry only consumes stored counters.
   - Document new context schema in evaluator README.

6. **Optimization & Validation (Phase F)**
   - Re-run bench, Callgrind, and targeted SPRTs (index off/on).
   - Track `isSquareAttacked` instruction count and evaluate compile-time vs runtime toggles.

## Dependencies & Supporting Work
- Finalize current king-danger feature set and SPSA tuning so the baseline is stable (no mixing with refactor).
- Capture before/after profiling data (bench, callgrind) for regression detection.
- Update `eval_trace` structures to pull data from context.
- Ensure unit tests / regression suites cover edge cases (pinned pieces, promotions, discovered attacks).

## Risks & Mitigations
- **Behavioural drift:** Introduce phased migration and cross-check evaluation outputs on curated FEN packs.
- **Complexity:** Limit context size to essential masks; document invariants (e.g., all masks are white-from-perspective or side-specific?).
- **Telemetry mismatch:** Validate traces by comparing current logs with new context-based values before removing old paths.
- **Integration overhead:** Keep `EvalContext` construction O(#pieces) by caching occupancy and avoiding repeated board queries.

## Metrics / Success Criteria
- `isSquareAttacked` instruction share drops significantly (<4% target during bench).
- With `EvalKingDangerIndex=true`, bench NPS regression ≤2% compared to index off.
- Telemetry-enabled runs show negligible extra cost (<1% additional instructions).
- Evaluation modules become composable (e.g., ability to turn features on/off for experiments without touching movegen).

## Notes for Future Implementers
- Review Perseus’ `pestoEval` for template patterns around mobility and threat aggregation.
- Keep per-feature commits small (align with existing Git strategy). Each phase should produce 0 ELO expectation documentation and include `bench <nodes>` per guidelines.
- Coordinate with tooling: update eval harness scripts to display context-derived telemetry fields.
- Consider introducing micro-benchmarks to guard `EvalContext` construction cost.

## Next Steps Prior to Kickoff
1. Finish king-danger search improvements and SPSA tuning to stabilize baseline.
2. Archive callgrind/bench results (`callgrind.index.{on,off}`, bench NPS) for comparison.
3. Draft implementation tickets per phase, referencing this document.
