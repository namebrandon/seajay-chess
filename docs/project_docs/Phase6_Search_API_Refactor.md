Title: Phase 6 — Search API Refactor (Excluded-Move Ready)

Objective
- Prepare the search to support singular extensions, multi-cut, and probcut without changing current behavior or depth-parity results. Introduce an explicit, testable Search Node API that clarifies node context (PV/non-PV/root), and cleanly supports an optional excluded-move parameter.

Non-Goals (Phase 6)
- Do not enable singular extensions, multi-cut, or probcut yet.
- Do not change default pruning/reduction behavior or evaluation features.
- Do not alter root or quiescence behavior.

Success Criteria
- All unit tests and perft unchanged.
- Bench within noise; depth/time parity unchanged vs baseline with Phase 6 toggles off.
- With toggles on (NoOp mode), behavior remains identical; telemetry shows no excluded-move usage.
- Clear API that allows adding singular/multicut/probcut in a later phase without further plumbing changes.

Guardrails
- Keep features behind UCI options/compile flags; default OFF.
- Add DEBUG asserts to ensure excluded state never leaks across nodes.
- Ensure TT stores remain hygienic (avoid polluting good entries with shallow NO_MOVE heuristics).

UCI/Build Toggles
- UseSearchNodeAPIRefactor (bool, default false): switches negamax signature to use NodeContext and excludes stack-based ad-hoc flags. No behavior change.
- EnableExcludedMoveParam (bool, default false): threads an excluded move parameter; still disabled by default at call sites.
- DEV_ASSERT_SearchAPI (bool, default true in DEBUG): turns on additional verification logging/asserts.

Sub‑Phases (each SPRT vs current main with toggles OFF; code paths compile but NoOp by default)

6a — Introduce NodeContext and Negamax Signature (NoOp)
- Add struct NodeContext { bool isPv; bool isRoot; Move excluded = NO_MOVE; }.
- Overload or update `negamax` to accept NodeContext in addition to existing parameters; map current call sites to equivalent context.
- Maintain existing behavior: pass excluded = NO_MOVE everywhere; isPv/isRoot passed explicitly instead of inferred from stack.
- Tests: build, unit tests, perft; bench parity; confirm UCI still works.

6b — Thread Context through Call Graph (NoOp)
- Thread NodeContext through all recursive calls, qsearch wrapper, and helper paths that need PV/non-PV knowledge.
- Keep current PV determination semantics identical (PV only for first legal move at PV parent) but expressed via NodeContext.
- Add DEBUG asserts to check consistency (e.g., child.isPv equals (parent.isPv && firstLegal)).
- Tests: same as 6a; add targeted unit tests for context propagation in synthetic tree.

6c — Replace Stack-Based Excluded-Move Checks with Parameter (NoOp)
- Replace `SearchInfo::isExcluded(ply, move)` usage at search loop with direct check against `ctx.excluded`.
- Keep SearchInfo’s excluded fields present but unused (deprecated) for now; add TODO to remove after rollout.
- Ensure no behavior change by leaving `ctx.excluded = NO_MOVE` from all call sites.
- Tests: confirm zero diff in node counts on a fixed suite; unit test ensures excluded move is respected when set (dev-only test harness).

6d — Add Verification Helper (Disabled)
- Implement `verify_exclusion(Board&, ctx, depth, alpha, beta, limits, tt)` that performs a reduced-depth, narrow-window search intended for singular/multicut verification.
- Not called anywhere by default; provide minimal telemetry counters (compiled out in Release).
- Tests: compile-only; a DEV test can call it directly with excluded move to verify windowing and depth reductions, but keep out of normal search.

6e — TT Probe/Store Hygiene Review (NoOp Behavior)
- Audit TT store paths to ensure correct bounds for: static-null prune, null-move cutoff, and future excluded verification.
- Add comments and small guards to prevent overwriting deeper move-carrying entries with shallow NO_MOVE heuristic entries.
- No default behavior changes; toggles allow turning on stricter guards for A/B.
- Tests: unit tests for TT replacement behavior remain green; bench parity.

6f — PV Clarity and Root/QS Safety (NoOp)
- Centralize PV handling via NodeContext; ensure qsearch never receives excluded moves and root remains unchanged.
- Add DEBUG asserts that rank-aware gates and pruning never trigger when ctx.excluded != NO_MOVE (future-proofing) and never at PV/root nodes when not intended.
- Tests: sanity checks over search traces; verify no asserts in DEBUG on standard suites.

6g — Integration + Rollout Steps
- Keep 6a–6f under `UseSearchNodeAPIRefactor=false` by default.
- Provide a DEV mode where it is true but ctx.excluded = NO_MOVE everywhere, to validate the new API under real search.
- Acceptance: once DEV runs show parity and no stability issues, set default to false and proceed to the next feature phase that uses the API.

Risks & Mitigations
- Risk: hidden behavior drift from PV propagation differences → Mitigate with asserts and A/B logs of PV move counts and PVS re-search rate.
- Risk: TT performance impact due to extra parameter threading → Keep inline-friendly APIs; verify bench parity.
- Risk: API churn affecting quiescence → Keep QS path explicitly orthogonal; asserts + tests.

Validation & SPRT Guidance
- For phases 6a–6f, SPRT not required when toggles OFF; run non-regression CI (tests, perft, bench). For sanity, optional brief OB with toggles ON (NoOp mode) and bounds [-3.00, 3.00].

Follow-ups (post Phase 6)
- Phase SE1 (later): Implement true singular extensions using 6d helper.
- Phase MC1/PC1 (later): Multi-cut and ProbCut built on the same API with their own toggles and SPRTs.


Appendix — QSearch Integration Notes (2025‑09‑13)
- TT eval coupling
  - Observation: Main search relies heavily on TT.eval written from qsearch (depth 0) for static‑null/futility gates. Any path that stores eval=TT_EVAL_NONE in qsearch degrades pruning globally.
  - Action in 6e (TT hygiene): Specify invariant “non‑check qsearch stores must include a valid static eval; never store TT_EVAL_NONE unless in check”. Add DEBUG asserts and counters. Avoid overwriting deeper/move‑carrying entries with shallow NO_MOVE heuristic entries.

- Full eval usage inside qsearch
  - Observation: qsearch computes full eval for multiple decisions (stand‑pat, coarse delta, per‑move delta, equal‑exchange SEE) and before TT stores. This makes QS expensive and central to engine behavior.
  - Action in 6b/6f: Thread a per‑node staticEval cache via NodeContext or a small helper so each node computes full eval at most once and shares it among decisions and TT store paths. Add a helper ensureStaticEval(ctx, board) used at all sites that require it.

- Stand‑pat confirmation and deferred eval
  - Observation: When deferring full eval after a low fast‑eval, we must re‑check the stand‑pat beta cutoff once full eval is computed to avoid missing easy cutoffs.
  - Action in 6f: Centralize stand‑pat handling into a small routine that enforces: beta confirmation requires full eval; any later ensureStaticEval must re‑check stand‑pat cutoff before proceeding to pruning.

- Fast‑eval usage roadmap (ties to Phase 3)
  - Action for later phases (not 6 default): Prepare API seams so static checks (coarse/delta/null‑move) can take an EvalProvider (full vs fast) selected by toggles, with parity sampling in DEBUG. In 6, define the interface but keep behavior NoOp.

- Telemetry to add (DEBUG‑only, compiled out in Release)
  - qsearchFullEvalCalls, qsearchNodesWithEvalDeferred, standPatReturns, standPatRechecksAfterDefer.
  - TT stores from qsearch: counts by bound type and with/without eval present.
  - Delta/SEE prune rates by qply; equal‑exchange prune rates by qply.

- Move ordering and scope in qsearch
  - Observation: QS currently performs non‑trivial ordering (discovered‑check prioritization, queen‑promotion front‑loading, in‑check reordering). This adds overhead and sensitivity.
  - Action in 6f: Keep QS path orthogonal and light; document intended ordering (captures/promotions only when not in check; simple, deterministic ordering), and move complex heuristics to main search where possible. Provide toggles/hooks but default to current behavior (NoOp) in Phase 6.
