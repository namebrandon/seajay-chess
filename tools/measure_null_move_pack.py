#!/usr/bin/env python3
"""Run depth-vs-time measurements for a FEN pack with and without aggressive null move."""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

DEPTH_RE = re.compile(r"\bdepth (\d+)")
NODES_RE = re.compile(r"\bnodes (\d+)")
SCORE_RE = re.compile(r"score (cp|mate) (-?\d+)")
TT_RE = re.compile(r"tthits ([\d.]+)")
HASHFUL_RE = re.compile(r"hashfull (\d+)")
SEARCHSTATS_RE = re.compile(r"extra\(cand=(\d+),app=(\d+),blk=(\d+),sup=(\d+),cut=(\d+),vpass=(\d+),vfail=(\d+)\)")


def parse_epd(epd_path: Path) -> List[Tuple[str, str]]:
    positions = []
    with epd_path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            tokens = line.split()
            if len(tokens) < 4:
                continue
            fen_core = " ".join(tokens[:4])
            fen = f"{fen_core} 0 1"
            pos_id = "unknown"
            match = re.search(r'id\s+"([^"]+)"', line)
            if match:
                pos_id = match.group(1)
            positions.append((pos_id, fen))
    return positions


def run_search(engine: Path, fen: str, movetime: int, aggressive: bool, common_options: Iterable[str], extra_options: Iterable[str]) -> Dict[str, str]:
    proc = subprocess.Popen(
        [str(engine)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    assert proc.stdin and proc.stdout

    def send(cmd: str) -> None:
        proc.stdin.write(cmd + "\n")
        proc.stdin.flush()

    # UCI handshake
    send("uci")
    while True:
        line = proc.stdout.readline()
        if not line:
            raise RuntimeError("Engine exited during uci")
        if line.strip() == "uciok":
            break

    send("setoption name SearchStats value true")
    send(f"setoption name UseAggressiveNullMove value {'true' if aggressive else 'false'}")
    for opt in common_options:
        send(f"setoption name {opt}")
    for opt in extra_options:
        send(f"setoption name {opt}")

    send("isready")
    while True:
        line = proc.stdout.readline()
        if not line:
            raise RuntimeError("Engine exited waiting for readyok")
        if line.strip() == "readyok":
            break

    send("ucinewgame")
    send("position fen " + fen)
    send(f"go movetime {movetime}")

    max_depth = 0
    nodes_at_depth = 0
    score_type = ""
    score_value = ""
    tt_hits = ""
    hashfull = ""
    stats = {"cand": "0", "app": "0", "blk": "0", "sup": "0", "cut": "0", "vpass": "0", "vfail": "0"}

    while True:
        line = proc.stdout.readline()
        if not line:
            break
        stripped = line.strip()
        if stripped.startswith("info "):
            depth_match = DEPTH_RE.search(stripped)
            if depth_match:
                depth = int(depth_match.group(1))
                if depth >= max_depth:
                    max_depth = depth
                    nodes_match = NODES_RE.search(stripped)
                    if nodes_match:
                        nodes_at_depth = int(nodes_match.group(1))
                    score_match = SCORE_RE.search(stripped)
                    if score_match:
                        score_type = score_match.group(1)
                        score_value = score_match.group(2)
                    tt_match = TT_RE.search(stripped)
                    if tt_match:
                        tt_hits = tt_match.group(1)
                    hash_match = HASHFUL_RE.search(stripped)
                    if hash_match:
                        hashfull = hash_match.group(1)
        elif stripped.startswith("bestmove"):
            break
        stats_match = SEARCHSTATS_RE.search(stripped)
        if stats_match:
            stats = {
                "cand": stats_match.group(1),
                "app": stats_match.group(2),
                "blk": stats_match.group(3),
                "sup": stats_match.group(4),
                "cut": stats_match.group(5),
                "vpass": stats_match.group(6),
                "vfail": stats_match.group(7),
            }

    send("quit")
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()

    return {
        "depth": str(max_depth),
        "nodes": str(nodes_at_depth),
        "score_type": score_type,
        "score_value": score_value,
        "tt_hits": tt_hits,
        "hashfull": hashfull,
        **stats,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Measure aggressive null move impact across a FEN pack")
    parser.add_argument("--engine", required=True, help="Path to engine binary")
    parser.add_argument("--epd", required=True, help="EPD file with candidate positions")
    parser.add_argument("--movetime", type=int, default=1000, help="Movetime in ms (default 1000)")
    parser.add_argument("--out", type=Path, default=Path("null_move_pack_metrics.csv"), help="Output CSV path")
    parser.add_argument("--setoption", action="append", default=[], help="Additional setoption commands applied in both modes (format: 'Name value X')")
    parser.add_argument("--aggressive-setoption", action="append", default=[], help="Additional setoption commands applied only in aggressive mode")
    args = parser.parse_args()

    engine = Path(args.engine)
    positions = parse_epd(Path(args.epd))
    if not positions:
        sys.exit("No positions found in EPD file")

    rows = []
    for pos_id, fen in positions:
        shadow = run_search(engine, fen, args.movetime, aggressive=False, common_options=args.setoption, extra_options=[])
        aggressive = run_search(engine, fen, args.movetime, aggressive=True, common_options=args.setoption, extra_options=args.aggressive_setoption)
        rows.append({
            "id": pos_id,
            "fen": fen,
            "shadow_depth": shadow["depth"],
            "shadow_nodes": shadow["nodes"],
            "shadow_cand": shadow["cand"],
            "shadow_app": shadow["app"],
            "shadow_cut": shadow["cut"],
            "shadow_vpass": shadow["vpass"],
            "shadow_score": shadow["score_value"],
            "aggr_depth": aggressive["depth"],
            "aggr_nodes": aggressive["nodes"],
            "aggr_cand": aggressive["cand"],
            "aggr_app": aggressive["app"],
            "aggr_cut": aggressive["cut"],
            "aggr_vpass": aggressive["vpass"],
            "aggr_score": aggressive["score_value"],
        })
        print(f"Processed {pos_id}: shadow depth {shadow['depth']} nodes {shadow['nodes']} | aggressive depth {aggressive['depth']} nodes {aggressive['nodes']}")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open('w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {len(rows)} rows to {args.out}")


if __name__ == "__main__":
    main()
