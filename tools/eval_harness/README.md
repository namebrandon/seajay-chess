# Evaluation Harness

CLI located at `tools/eval_harness/compare_eval.py` drives SeaJay (and optional
reference engines) across curated evaluation packs and emits structured JSON
summaries.

## Usage
```
python3 tools/eval_harness/compare_eval.py \
  --engine ./bin/seajay \
  --pack tests/packs/eval_pawn_focus.epd \
  --movetime 100 \
  --threads 1 \
  --out logs/eval_pawn_focus.json \
  --summary-top 10 \
  --summary-json logs/eval_pawn_focus_summary.json
```

Optional reference comparison (Komodo auto-detected when present):
```
python3 tools/eval_harness/compare_eval.py \
  --engine ./bin/seajay \
  --pack tests/packs/eval_pawn_focus.epd \
  --movetime 100 \
  --out logs/eval_vs_komodo.json
```
If `external/engines/komodo/komodo-14.1-linux` exists and is executable, the
harness selects it automatically as the reference engine. Provide
`--ref-engine` to override or disable.

### Convenience wrapper

`tools/eval_harness/run_eval_pack.sh` runs the default pack with sensible
defaults and writes both the full report and summary into `tmp/eval_reports/`.
Environment overrides:

- `ENGINE`, `PACK`, `REF_ENGINE`, `MOVETIME`, `DEPTH`, `THREADS`
- `OUTDIR`, `REPORT`, `SUMMARY`

Example:
```
./tools/eval_harness/run_eval_pack.sh
```

### Summary output

### Key Behaviors
- Enables `EvalExtended` automatically and captures structured `info eval` lines
  for each position (including per-term contributions and pawn-hash metadata).
- Executes a `go` command per position (honoring `--movetime` and/or
  `--depth`), collecting depth, score, nodes, and best move.
- Invokes `debug eval` after the search to gather the evaluation trace.
- Optionally drives an additional UCI engine for comparison (best move, score,
  depth). Reference engines are not expected to emit `info eval` telemetry.
- Results are stored as JSON with raw `info` lines preserved for deeper audits.

### Output Sketch
```
{
  "metadata": {
    "pack": "tests/packs/eval_pawn_focus.epd",
    "engine": {"path": "./bin/seajay", "id": "SeaJay v20250920"},
    "reference_engine": {"path": "./engines/komodo", "id": "Komodo"},
    "options": {"movetime": 100, "depth": null, "threads": 1}
  },
  "positions": [
    {
      "index": 1,
      "fen": "r7/2p2pp1/...",
      "label": "Theme: advanced a-pawn...",
      "seajay": {
        "search": {
          "bestmove": "rb8",
          "depth": 21,
          "score_cp": -106,
          "nodes": 348219,
          "elapsed_ms": 102,
          "raw": ["info depth 21 ...", "bestmove rb8"]
        },
        "evaluation": {
          "summary": {"side": "black", "final_cp": -106},
          "terms": {"passed_pawns": {"cp": -12, "white": 0, "black": 1}},
          "raw": ["info eval header ...", ...]
        }
      },
      "reference": {
        "search": {"bestmove": "bd4", "depth": 30, "score_cp": -373}
      },
      "comparison": {
        "score_delta_cp": 267,
        "bestmove_match": false,
        "reference_bestmove": "bd4"
      }
    }
  ]
}
```

### Notes
- Provide `--eval-log` if you also want SeaJay to mirror the telemetry into a
  file via `EvalLogFile`.
- The harness exits with non-zero status on pack parsing errors or if the
  engine handshake times out.
- JSON reports can be diffed across engine revisions to observe evaluation term
  changes per position.
