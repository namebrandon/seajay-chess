#!/usr/bin/env python3
"""Targeted tactical investigation harness for SeaJay.

Allows running a subset of WAC positions (or custom EPD files) at
multiple time controls to determine whether the engine ever plays – or
at least considers – the expected move.
"""
from __future__ import annotations

import argparse
import csv
import re
import subprocess
import sys
import time
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

try:
    import chess  # type: ignore
except ImportError as exc:  # pragma: no cover - this script requires python-chess
    raise SystemExit("python-chess is required: pip install chess") from exc

INFO_RE = re.compile(r"depth (\\d+).*?nodes (\\d+).*?nps (\\d+)")
SCORE_RE = re.compile(r"score (cp|mate) (-?\\d+)")


@dataclass
class EngineResult:
    bestmove: str
    info_lines: List[str]
    raw_output: str

    def summary(self) -> Dict[str, Optional[str]]:
        depth = nodes = nps = score = None
        score_type = None
        if self.info_lines:
            for line in reversed(self.info_lines):
                if depth is None:
                    m = INFO_RE.search(line)
                    if m:
                        depth, nodes, nps = m.groups()
                if score is None:
                    s = SCORE_RE.search(line)
                    if s:
                        score_type, score = s.groups()
                if depth and score:
                    break
        return {
            "depth": depth,
            "nodes": nodes,
            "nps": nps,
            "score_type": score_type,
            "score": score,
        }


def parse_epd(epd_path: Path) -> Dict[str, Tuple[str, List[str]]]:
    """Return mapping id -> (fen, [best moves in SAN])."""
    mapping: Dict[str, Tuple[str, List[str]]] = {}
    with epd_path.open() as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            id_match = re.search(r'id "([^"]+)"', line)
            bm_match = re.search(r'bm ([^;]+);', line)
            if not id_match or not bm_match:
                continue
            pos_id = id_match.group(1)
            fen = line[: line.index(" bm ")].strip()
            if len(fen.split()) == 4:
                fen = f"{fen} 0 1"
            bm_moves = bm_match.group(1).strip().split()
            mapping[pos_id] = (fen, bm_moves)
    return mapping


def run_engine(engine_path: Path, fen: str, time_ms: int, depth: int = 0) -> EngineResult:
    go_cmd = f"go depth {depth}" if depth > 0 else f"go movetime {time_ms}"
    proc = subprocess.Popen(
        [str(engine_path)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    assert proc.stdin is not None and proc.stdout is not None

    def send(cmd: str) -> None:
        proc.stdin.write(cmd + "\n")
        proc.stdin.flush()

    stdout_lines: List[str] = []
    info_lines: List[str] = []

    def readline_with_timeout(timeout: float) -> Optional[str]:
        start = time.time()
        while True:
            line = proc.stdout.readline()
            if line:
                stdout_lines.append(line)
                if line.startswith("info "):
                    info_lines.append(line.strip())
                return line
            if time.time() - start > timeout:
                return None

    send("uci")
    while True:
        line = readline_with_timeout(5)
        if not line:
            raise RuntimeError("Engine did not respond to UCI")
        if line.strip() == "uciok":
            break

    send("isready")
    while True:
        line = readline_with_timeout(5)
        if not line:
            raise RuntimeError("Engine did not become ready")
        if line.strip() == "readyok":
            break

    send(f"position fen {fen}")
    send(go_cmd)

    timeout = (time_ms / 1000) + 10 if depth == 0 else 60
    start = time.time()
    bestmove: Optional[str] = None
    while True:
        line = readline_with_timeout(timeout)
        if line is None:
            raise RuntimeError("Engine search timed out")
        if line.startswith("bestmove"):
            parts = line.split()
            if len(parts) >= 2:
                bestmove = parts[1]
            break
        if time.time() - start > timeout:
            raise RuntimeError("Engine search exceeded timeout")

    send("quit")
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()

    if not bestmove:
        raise RuntimeError("Engine failed to report bestmove")

    return EngineResult(bestmove=bestmove, info_lines=info_lines, raw_output="".join(stdout_lines))


def build_expected_moves(fen: str, san_moves: Sequence[str]) -> Tuple[List[str], Dict[str, str]]:
    """Return list of expected UCI moves and mapping back to SAN."""
    board = chess.Board(fen)
    uci_moves: List[str] = []
    san_by_uci: Dict[str, str] = {}
    for san in san_moves:
        candidate = san.split()[0]
        try:
            move = board.parse_san(candidate)
        except ValueError:
            continue
        uci = move.uci()
        uci_moves.append(uci)
        san_by_uci[uci] = board.san(move)
    return uci_moves, san_by_uci


def analyse_position(
    engine_path: Path,
    fen: str,
    san_moves: Sequence[str],
    time_controls: Sequence[int],
    depth: int = 0,
) -> List[Dict[str, object]]:
    expected_uci, _ = build_expected_moves(fen, san_moves)
    results: List[Dict[str, object]] = []

    for time_ms in time_controls:
        engine_result = run_engine(engine_path, fen, time_ms, depth)
        summary = engine_result.summary()
        bestmove = engine_result.bestmove
        board_for_san = chess.Board(fen)
        try:
            san_best = board_for_san.san(chess.Move.from_uci(bestmove))
        except ValueError:
            san_best = None

        seen_in_pv = False
        if expected_uci:
            for line in engine_result.info_lines:
                if " pv " not in line:
                    continue
                pv_moves = line.split(" pv ", 1)[1].split()
                if any(expected in pv_moves for expected in expected_uci):
                    seen_in_pv = True
                    break

        results.append(
            {
                "time_ms": time_ms,
                "bestmove": bestmove,
                "bestmove_san": san_best,
                "matches_expected": bestmove in expected_uci,
                "expected_moves_uci": " ".join(expected_uci),
                "info_depth": summary["depth"],
                "info_nodes": summary["nodes"],
                "info_nps": summary["nps"],
                "score_type": summary["score_type"],
                "score": summary["score"],
                "seen_in_pv": seen_in_pv,
            }
        )

    return results


def load_ids_from_csv(csv_path: Path) -> List[str]:
    with csv_path.open() as handle:
        reader = csv.DictReader(handle)
        return [row["position_id"] for row in reader]


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Targeted tactical investigation harness")
    parser.add_argument("--engine", default="bin/seajay", help="Path to SeaJay binary")
    parser.add_argument("--epd", default="tests/positions/wac_failures_20250918.epd", help="EPD file with positions")
    parser.add_argument("--ids", nargs="*", help="Specific position IDs to run (defaults to all in EPD)")
    parser.add_argument("--ids-from-csv", dest="ids_csv", help="CSV file with position_id column to filter")
    parser.add_argument("--time-ms", type=int, nargs="+", default=[850], help="List of movetime values in ms")
    parser.add_argument("--depth", type=int, default=0, help="Fixed search depth (overrides movetime when > 0)")
    parser.add_argument("--output", help="Optional CSV output path")
    parser.add_argument("--verbose", action="store_true", help="Print detailed per-run info")

    args = parser.parse_args(argv)

    engine_path = Path(args.engine)
    if not engine_path.exists():
        parser.error(f"engine not found: {engine_path}")

    epd_path = Path(args.epd)
    if not epd_path.exists():
        parser.error(f"EPD file not found: {epd_path}")

    data = parse_epd(epd_path)

    if args.ids_csv:
        ids = load_ids_from_csv(Path(args.ids_csv))
    elif args.ids:
        ids = args.ids
    else:
        ids = list(data.keys())

    missing = [pid for pid in ids if pid not in data]
    if missing:
        parser.error(f"positions not found in {epd_path}: {', '.join(missing)}")

    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    output_path = Path(args.output) if args.output else Path(f"tools/tactical_investigation_{timestamp}.csv")

    rows: List[Dict[str, object]] = []

    for pos_id in ids:
        fen, san_moves = data[pos_id]
        analyses = analyse_position(engine_path, fen, san_moves, args.time_ms, args.depth)
        for entry in analyses:
            row = {
                "position_id": pos_id,
                "fen": fen,
                "expected_moves_san": " ".join(san_moves),
                **entry,
            }
            rows.append(row)
            if args.verbose:
                tc = entry['time_ms'] if args.depth == 0 else f"depth {args.depth}"
                match = "✅" if entry["matches_expected"] else "❌"
                print(f"{pos_id} @ {tc}: {match} best {entry['bestmove_san']} ({entry['bestmove']}) seen_in_pv={entry['seen_in_pv']}")

    fieldnames = [
        "position_id",
        "fen",
        "expected_moves_san",
        "expected_moves_uci",
        "time_ms",
        "bestmove",
        "bestmove_san",
        "matches_expected",
        "seen_in_pv",
        "info_depth",
        "info_nodes",
        "info_nps",
        "score_type",
        "score",
    ]

    with output_path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

    if args.verbose:
        print(f"Results saved to {output_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
