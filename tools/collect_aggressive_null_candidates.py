#!/usr/bin/env python3
"""Probe a set of EPD positions and record where the aggressive null-move
instrumentation triggers.

Outputs a CSV listing positions where extra null-move reductions were eligible
or applied, along with the captured counters from SearchStats.
"""

import argparse
import csv
import re
import subprocess
import sys
import time
from pathlib import Path
from typing import List, Optional, Tuple

SEARCHSTATS_RE = re.compile(
    r"extra\(cand=(\d+),app=(\d+),blk=(\d+),sup=(\d+),cut=(\d+),vpass=(\d+),vfail=(\d+)\)"
)
SCORE_RE = re.compile(r"score\s+(mate|cp)\s+(-?\d+)")


def parse_epd_line(line: str) -> Optional[Tuple[str, str]]:
    line = line.strip()
    if not line or line.startswith("#"):
        return None

    tokens = line.split()
    if len(tokens) < 4:
        return None

    fen_core = " ".join(tokens[:4])
    # EPD omits move counters; append placeholders for engine compatibility.
    fen = f"{fen_core} 0 1"

    id_match = re.search(r'id\s+"([^"]+)"', line)
    pos_id = id_match.group(1) if id_match else "unknown"

    return fen, pos_id


def send(proc: subprocess.Popen, cmd: str) -> None:
    assert proc.stdin is not None
    proc.stdin.write(cmd + "\n")
    proc.stdin.flush()


def read_until(proc: subprocess.Popen, predicate, timeout: float, lines: List[str]) -> bool:
    assert proc.stdout is not None
    end_time = time.time() + timeout
    while time.time() < end_time:
        line = proc.stdout.readline()
        if not line:
            return False
        lines.append(line)
        if predicate(line):
            return True
    return False


def probe_fen(engine: str, fen: str, depth: Optional[int], movetime: Optional[int],
              null_margin: Optional[int], extra_options: List[str], timeout: float = 30.0):
    proc = subprocess.Popen(
        [engine],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    captured: List[str] = []

    try:
        send(proc, "uci")
        if not read_until(proc, lambda l: l.strip() == "uciok", timeout, captured):
            raise RuntimeError("engine did not report uciok")

        send(proc, "setoption name SearchStats value true")
        send(proc, "setoption name UseAggressiveNullMove value true")
        if null_margin is not None:
            send(proc, f"setoption name NullMoveEvalMargin value {null_margin}")
        for opt in extra_options:
            send(proc, opt)

        send(proc, "isready")
        if not read_until(proc, lambda l: l.strip() == "readyok", timeout, captured):
            raise RuntimeError("engine did not report readyok")

        send(proc, f"position fen {fen}")
        if depth is not None:
            send(proc, f"go depth {depth}")
        else:
            assert movetime is not None
            send(proc, f"go movetime {movetime}")

        # Read until bestmove appears.
        read_until(proc, lambda l: l.startswith("bestmove"), timeout, captured)

        send(proc, "quit")
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()

    finally:
        if proc.poll() is None:
            proc.kill()

    # Parse statistics
    stats = {
        "cand": 0,
        "app": 0,
        "blk": 0,
        "sup": 0,
        "cut": 0,
        "vpass": 0,
        "vfail": 0,
        "score_type": None,
        "score_value": None,
    }

    for line in captured:
        match = SEARCHSTATS_RE.search(line)
        if match:
            stats.update({
                "cand": int(match.group(1)),
                "app": int(match.group(2)),
                "blk": int(match.group(3)),
                "sup": int(match.group(4)),
                "cut": int(match.group(5)),
                "vpass": int(match.group(6)),
                "vfail": int(match.group(7)),
            })
        score_match = SCORE_RE.search(line)
        if score_match:
            kind, value = score_match.groups()
            stats["score_type"] = kind
            stats["score_value"] = int(value)

    return stats, captured


def main() -> None:
    parser = argparse.ArgumentParser(description="Collect aggressive null-move candidate positions")
    parser.add_argument("--engine", required=True, help="Path to UCI engine")
    parser.add_argument("--epd", required=True, help="EPD file containing test positions")
    parser.add_argument("--depth", type=int, default=10, help="Search depth to probe (default 10)")
    parser.add_argument("--movetime", type=int, default=0, help="Alternative movetime in ms (0 to ignore)")
    parser.add_argument("--null-margin", type=int, default=None, help="Override NullMoveEvalMargin")
    parser.add_argument("--extra-option", action="append", default=[], help="Additional raw setoption commands")
    parser.add_argument("--out", type=Path, default=Path("aggressive_null_candidates.csv"), help="Output CSV path")
    parser.add_argument("--min-candidates", type=int, default=1, help="Minimum candidate count to record")

    args = parser.parse_args()

    if args.depth <= 0 and args.movetime <= 0:
        parser.error("Provide either --depth > 0 or --movetime > 0")

    use_depth = args.depth > 0
    depth = args.depth if use_depth else None
    movetime = None if use_depth else args.movetime

    epd_path = Path(args.epd)
    if not epd_path.is_file():
        sys.exit(f"EPD file not found: {epd_path}")

    out_rows = []

    with epd_path.open() as f:
        for idx, raw_line in enumerate(f, 1):
            parsed = parse_epd_line(raw_line)
            if not parsed:
                continue
            fen, pos_id = parsed
            try:
                stats, _ = probe_fen(
                    engine=args.engine,
                    fen=fen,
                    depth=depth,
                    movetime=movetime,
                    null_margin=args.null_margin,
                    extra_options=args.extra_option,
                )
            except Exception as exc:  # noqa: BLE001
                print(f"[WARN] Failed to probe {pos_id} (line {idx}): {exc}", file=sys.stderr)
                continue

            if stats["cand"] >= args.min_candidates or stats["app"] > 0:
                out_rows.append({
                    "id": pos_id,
                    "fen": fen,
                    **stats,
                })

    if not out_rows:
        print("No positions met the criteria; nothing written." )
        return

    out_path = args.out
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="") as csvfile:
        writer = csv.DictWriter(
            csvfile,
            fieldnames=[
                "id",
                "fen",
                "cand",
                "app",
                "blk",
                "sup",
                "cut",
                "vpass",
                "vfail",
                "score_type",
                "score_value",
            ],
        )
        writer.writeheader()
        writer.writerows(out_rows)

    print(f"Wrote {len(out_rows)} positions to {out_path}")


if __name__ == "__main__":
    main()
