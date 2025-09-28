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
  --out logs/eval_pawn_focus.json
```

Optional reference comparison:
```
python3 tools/eval_harness/compare_eval.py \
  --engine ./bin/seajay \
  --pack tests/packs/eval_pawn_focus.epd \
  --movetime 100 \
  --out logs/eval_vs_komodo.json \
  --ref-engine /path/to/komodo \
  --ref-name Komodo
```

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
