e8f7 e8f7_issue.txt – Independent Review (Consultant Report)

Summary

- Conclusion: I agree with the assessment that quiescence search is the proximate cause of the instability and move flip (to e8f7) around depth 3–4 and is a likely contributor to the node explosion pattern you are observing.
- Mechanism: In-check quiescence behavior plus its move-ordering biases and check extensions combine to make king-evasion moves look artificially “safe” at shallow depths. This causes a PV switch to e8f7 that later gets refuted at deeper depths, while also inflating nodes searched.

What I Reviewed

- Position and outcomes described in e8f7_issue.txt, including toggling UseQuiescence and observing PV and node differences.
- Quiescence implementation src/search/quiescence.cpp and header src/search/quiescence.h.
- Integration points in src/search/negamax.cpp (entry to qsearch, options), and related move-ordering and pruning knobs (SEE, delta pruning, check-ply limit, TT usage).
- Project/process docs you referenced for context.

Key Observations From Code

- In-check handling in quiescence:
  - When in check, quiescence generates all legal evasions (not just captures), as expected.
  - Evasion move ordering explicitly prioritizes king moves first, then captures, then blocks (src/search/quiescence.cpp). This creates a strong bias toward king moves at q-nodes under check.
  - Stand-pat is not available when in check (again standard), which removes a stabilizing alpha bound and forces exploration of evasions.
- Check extension limiting:
  - Quiescence tracks consecutive check plies and extends up to `limits.maxCheckPly` (default 6). In tactical positions, this permits deeper qsearch trees precisely where checks proliferate, amplifying the impact of the king-move-first bias.
- Ordering and pruning when not in check:
  - Uses MVV-LVA or SEE ordering for captures, prioritizes queen promotions and discovered checks; delta pruning and SEE pruning are enabled with margins that are more conservative when in check (delta pruning not applied while in check).
- TT interaction:
  - Quiescence probes TT but only accepts depth==0 entries; stores LOWER/EXACT/UPPER with depth 0 and the best move found. This is generally sound and avoids poisoning from main search, but it can still reinforce the local bias within qsearch lines.
- Root quiet ordering (negamax):
  - Root-specific heuristic penalizes king moves in quiet ordering, but e8f7 still becomes preferred due to the score produced by qsearch lines.

Why This Fits the Symptom Pattern

- e8f7_issue.txt shows: with quiescence disabled, the engine gravitates toward c8d7 and d5d4 (reasonable choices), whereas with quiescence enabled it switches to e8f7 starting around depth 4 and sticks there.
- Static evaluations at depth 1 are not the problem; the “flip” comes from tactical exploration at the leaves. In this position, king-on-f7 lines are avoiding the immediate checking/capture sequences that quiescence explores first when other moves are tried.
- The combination of:
  - No stand-pat in check,
  - Evasion ordering bias (king moves first),
  - Check extensions up to 6 plies,
  - Limited pruning when in check,
leads to a systematic preference to try king moves earlier with higher initial alpha improvements at shallow depths. That looks better at depth 3–4, but at higher depth these lines get refuted (horizon effect). Meanwhile, node counts inflate because these lines are expensive to resolve in qsearch.

Alternative Hypotheses Considered (and discounted)

- Evaluation bug: Depth-1 evals and the comparative ranking in the report align with other engines; static eval deltas are small and directionally consistent.
- Root move ordering: Penalizes root king moves; doesn’t explain the depth-dependent flip driven by leaf tactics.
- TT poisoning from main search: Guarded by depth==0 acceptance in qsearch probe; unlikely to drive this specific flip.

Contributing Factors That Likely Exacerbate Nodes

- `maxCheckPly = 6` for qsearch is on the permissive side for tactics-heavy positions.
- Evasion ordering prioritizing king moves at q-nodes under check biases the search frontier toward evasive king steps.
- Discovered-check prioritization and general ordering for capture-heavy positions may create deep tactical tails in the non-king-move branches that are only “fixed” at higher depth.
- In-check nodes skip delta pruning and stand-pat, further broadening the tree when other moves invite checks.

Next Steps (No Code Changes Yet)

Goal: Empirically validate the mechanism and identify the most targeted mitigation with the least global risk.

1) Reproduce and quantify with existing knobs
   - Run the provided FEN with `UseQuiescence=true/false` across depths 2–10; record PVs, nodes, tthits. Confirm the flip depth and node inflation.
   - Sweep `maxCheckPly` via UCI (e.g., 2, 3, 4, 6) and observe PV stability and nodes. Expect that 2–3 reduces the e8f7 bias and node growth in this case.
   - Toggle SEE pruning mode to `aggressive` and monitor whether in-check branches shrink meaningfully without harming correctness on this position.
   - Use `tools/node_explosion_diagnostic.sh` focused on the problem FEN to capture ratios vs peer engines (if available locally).

2) Targeted behavior experiments (behind options if added later)
   - Evasion ordering variants for in-check quiescence:
     - “Capture the checker first, then blocks, then king moves.”
     - Interleave instead of forcing all king moves first; apply SEE on evasions to prefer cost-effective responses.
     - Only consider non-capturing king evasions after confirming no safe capture or block exists (by SEE or a light heuristic).
   - Check extension limit in qsearch:
     - Temporarily reduce `maxCheckPly` to 3 and retest; if it cures the flip without widespread tactical harm, this is a low-risk lever.
   - Equal-exchange pruning deep in qsearch:
     - Already present for SEE aggressive at qply>=3–5; validate it’s active in the failing lines and whether tuning thresholds helps.

3) Broader validation
   - Run tactical suites and a micro-SPRT vs current main to ensure any tweak does not reduce tactic-finding strength.
   - Track “moveeff” and “ebf” from logs to see ordering and branching improvements at depths 3–6.

Prioritized Recommendation Path

1. Configuration-only trial: Lower `maxCheckPly` to 3 and rerun the problem FEN and a small tactical suite. If PV stabilizes (no e8f7 flip) and node counts drop, pursue as a candidate default or adaptive policy (e.g., scale with qply or game phase).
2. If needed, adjust in-check evasion ordering in quiescence to prefer capturing the checker and blocks ahead of king moves (or apply SEE-based ordering to evasions). Gate behind a temporary UCI for quick A/B testing.
3. If still needed, increase pruning aggressiveness for deep qsearch levels (equal exchanges, SEE thresholds) and/or slightly reduce allowed captures per q-node in panic/deep qply.

Risks and Mitigations

- Reducing check extensions can miss deep tactics; mitigate with suite testing, adaptive caps (e.g., reduce only at high qply), or PV safeguards.
- Changing evasion ordering affects a wide tactical class; gate behind a UCI flag for controlled A/B.
- Over-aggressive pruning can cause tactical blindness; prefer changes that primarily affect in-check q-nodes or deep qply.

Bottom Line

- I agree with the diagnosis: Quiescence search, specifically the in-check evasion ordering and permissive check extension depth, is the most likely cause of the e8f7 preference at depth 4 and the associated node inflation.
- The least invasive and highest-signal experiments are: (1) `maxCheckPly` reduction to 3, (2) SEE-based evasion ordering that doesn’t force king moves first, and (3) validating aggressive SEE pruning/equal-exchange pruning efficacy at higher qply.

If you’d like, I can set up a minimal experiment plan (scripts + run matrix) to collect the above metrics without changing core code paths, or prepare a temporary UCI toggle design so these behaviors can be A/B tested cleanly when you’re ready to implement.

