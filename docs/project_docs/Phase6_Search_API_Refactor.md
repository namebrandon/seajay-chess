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

