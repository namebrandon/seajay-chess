#!/usr/bin/env python3
"""Probe SeaJay selectivity toggles against Komodo references."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional

import chess

TRACKER_DEFAULT = Path("docs/issues/eval_bias_tracker.json")
OUTPUT_JSON_DEFAULT = Path("docs/project_docs/telemetry/eval_bias/selectivity_probe_results.json")
OUTPUT_MD_DEFAULT = Path("docs/project_docs/telemetry/eval_bias/selectivity_probe_results.md")
ENGINE_PATH_DEFAULT = Path("bin/seajay")

UCI_DISABLE_SELECTIVITY = [
    ("LMREnabled", "false"),
    ("SEEPruning", "off"),
    ("QSEEPruning", "off"),
    ("UseNullMove", "false"),
    ("FutilityPruning", "false"),
    ("UseFutilityPruning", "false"),
]

BESTMOVE_RE = re.compile(r"bestmove\s+(\S+)")
SCORE_CP_RE = re.compile(r"info .*?score cp (-?\d+)")
DEPTH_RE = re.compile(r"info depth (\d+)")


@dataclass
class EngineRun:
    bestmove_uci: Optional[str]
    bestmove_san: Optional[str]
    score_cp: Optional[int]
    depth: Optional[int]
    raw: str


@dataclass
class ProbeResult:
    fen: str
    komodo_move: Optional[str]
    komodo_score: Optional[int]
    baseline: EngineRun
    relaxed: EngineRun

    def to_dict(self) -> dict:
        return {
            "fen": self.fen,
            "komodo_move": self.komodo_move,
            "komodo_score": self.komodo_score,
            "baseline": self._run_dict(self.baseline),
            "relaxed": self._run_dict(self.relaxed),
        }

    @staticmethod
    def _run_dict(run: EngineRun) -> dict:
        return {
            "bestmove_uci": run.bestmove_uci,
            "bestmove_san": run.bestmove_san,
            "score_cp": run.score_cp,
            "depth": run.depth,
        }


def parse_engine_output(output: str, fen: str) -> EngineRun:
    best_match = BESTMOVE_RE.search(output)
    score_matches = SCORE_CP_RE.findall(output)
    depth_matches = DEPTH_RE.findall(output)
    bestmove = best_match.group(1) if best_match else None
    bestmove_san = None
    if bestmove:
        try:
            board = chess.Board(fen)
            move = chess.Move.from_uci(bestmove)
            if move in board.legal_moves:
                bestmove_san = board.san(move)
        except Exception:
            bestmove_san = None
    score_cp = int(score_matches[-1]) if score_matches else None
    depth = int(depth_matches[-1]) if depth_matches else None
    return EngineRun(bestmove_uci=bestmove, bestmove_san=bestmove_san, score_cp=score_cp, depth=depth, raw=output)


def run_engine(engine: Path, fen: str, movetime: int, toggles: Optional[list[tuple[str, str]]] = None) -> EngineRun:
    commands = ["uci", "isready"]
    if toggles:
        for name, value in toggles:
            commands.append(f"setoption name {name} value {value}")
    commands.append(f"position fen {fen}")
    commands.append(f"go movetime {movetime}")
    commands.append("quit")
    payload = "\n".join(commands) + "\n"
    completed = subprocess.run(
        [str(engine)],
        input=payload,
        text=True,
        capture_output=True,
        check=True,
    )
    return parse_engine_output(completed.stdout, fen)


def load_tracker(path: Path) -> Dict[str, dict]:
    data = json.loads(path.read_text())
    mapping: Dict[str, dict] = {}
    for entry in data:
        fen = entry["fen"]
        mapping[fen] = entry["evaluations"]
    return mapping


def reference_for_fen(evals: dict, prefer_depth: int = 18) -> tuple[Optional[str], Optional[int], Optional[int]]:
    depth_key = str(prefer_depth)
    if depth_key in evals:
        ref = evals[depth_key]
        return (
            ref.get("komodo", {}).get("bestmove"),
            ref.get("komodo", {}).get("score_cp"),
            prefer_depth,
        )
    # fallback to highest available depth
    available = sorted((int(k), v) for k, v in evals.items())
    if not available:
        return (None, None, None)
    depth_key, ref = available[-1]
    return (
        ref.get("komodo", {}).get("bestmove"),
        ref.get("komodo", {}).get("score_cp"),
        depth_key,
    )


def build_markdown(results: list[ProbeResult]) -> str:
    lines = [
        "# Selectivity Probe Results",
        "",
        "| FEN | Komodo Move | Baseline Move | Relaxed Move | Baseline Δcp | Relaxed Δcp |",
        "| --- | --- | --- | --- | ---: | ---: |",
    ]
    for res in results:
        kmove = res.komodo_move or ""
        bmove = res.baseline.bestmove_san or res.baseline.bestmove_uci or ""
        rmove = res.relaxed.bestmove_san or res.relaxed.bestmove_uci or ""
        def delta(run: EngineRun) -> str:
            if res.komodo_score is None or run.score_cp is None:
                return ""
            return str(run.score_cp - res.komodo_score)
        lines.append(
            f"| `{res.fen}` | {kmove} | {bmove} | {rmove} | {delta(res.baseline)} | {delta(res.relaxed)} |"
        )
    lines.append("")
    # summary counts
    baseline_matches = sum(
        1
        for r in results
        if r.komodo_move and r.baseline.bestmove_san == r.komodo_move
    )
    relaxed_matches = sum(
        1
        for r in results
        if r.komodo_move and r.relaxed.bestmove_san == r.komodo_move
    )
    total_with_ref = sum(1 for r in results if r.komodo_move)
    lines.append(f"Baseline matches: {baseline_matches}/{total_with_ref}")
    lines.append(f"Relaxed matches: {relaxed_matches}/{total_with_ref}")
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description="Probe selectivity toggles against Komodo references.")
    parser.add_argument("--tracker", type=Path, default=TRACKER_DEFAULT)
    parser.add_argument("--engine", type=Path, default=ENGINE_PATH_DEFAULT)
    parser.add_argument("--movetime", type=int, default=2000)
    parser.add_argument("--output-json", type=Path, default=OUTPUT_JSON_DEFAULT)
    parser.add_argument("--output-md", type=Path, default=OUTPUT_MD_DEFAULT)
    parser.add_argument("--limit", type=int, default=0, help="Limit number of FENs (0 = all)")
    args = parser.parse_args()

    evals = load_tracker(args.tracker)
    fens = list(evals.keys())
    if args.limit:
        fens = fens[: args.limit]

    results: list[ProbeResult] = []
    for fen in fens:
        komodo_move, komodo_score, depth_used = reference_for_fen(evals[fen])
        baseline = run_engine(args.engine, fen, args.movetime)
        relaxed = run_engine(args.engine, fen, args.movetime, toggles=UCI_DISABLE_SELECTIVITY)
        results.append(
            ProbeResult(
                fen=fen,
                komodo_move=komodo_move,
                komodo_score=komodo_score,
                baseline=baseline,
                relaxed=relaxed,
            )
        )
        print(f"Processed FEN (depth ref {depth_used}): {fen}")

    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(json.dumps([r.to_dict() for r in results], indent=2))
    args.output_md.parent.mkdir(parents=True, exist_ok=True)
    args.output_md.write_text(build_markdown(results))


if __name__ == "__main__":
    main()
