#!/usr/bin/env python3
"""
Depth vs Time profiler for UCI engines.

Runs a set of FENs against one or more UCI engines for a fixed time per move,
captures the maximum depth reported and nodes searched, and writes a CSV report.

Usage:
  ./tools/depth_vs_time.py --time-ms 1000 \
      --engine ./bin/seajay \
      --engine ./external/engines/stash-bot/stash \
      --fen "startpos" \
      --fen "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" \
      --out depth_vs_time.csv

Notes:
  - Engines must speak UCI.
  - If you pass "startpos" as a FEN, the script will use "position startpos".
  - This script does not modify the codebase; it is purely for measurement.
"""

import argparse
import csv
import os
import re
import shlex
import subprocess
import sys
import time
from datetime import datetime


def uci_run_search(engine_path: str, fen: str, time_ms: int, uci_options: dict = None):
    proc = subprocess.Popen(
        [engine_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1
    )

    def send(cmd: str):
        assert proc.stdin is not None
        proc.stdin.write(cmd + "\n")
        proc.stdin.flush()

    # Initialize UCI
    send("uci")
    # Read until readyok
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        if line.strip() == "uciok":
            break

    # Set UCI options if provided
    if uci_options:
        for name, value in uci_options.items():
            send(f"setoption name {name} value {value}")
    
    send("isready")
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        if line.strip() == "readyok":
            break

    send("ucinewgame")
    send("isready")
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        if line.strip() == "readyok":
            break

    # Set position
    if fen == "startpos":
        send("position startpos")
    else:
        send(f"position fen {fen}")

    # Go
    send(f"go movetime {time_ms}")

    max_depth = 0
    nodes_at_max_depth = 0
    bestmove = None
    score_cp = None
    tt_hits = None
    hashfull = None

    depth_re = re.compile(r"\bdepth (\d+)")
    nodes_re = re.compile(r"\bnodes (\d+)")
    score_cp_re = re.compile(r"score cp (-?\d+)")
    tthits_re = re.compile(r"tthits ([\d.]+)")
    hashfull_re = re.compile(r"hashfull (\d+)")

    while True:
        line = proc.stdout.readline()
        if not line:
            break
        s = line.strip()
        if s.startswith("info "):
            m = depth_re.search(s)
            if m:
                d = int(m.group(1))
                if d >= max_depth:
                    max_depth = d
                    nmatch = nodes_re.search(s)
                    if nmatch:
                        nodes_at_max_depth = int(nmatch.group(1))
                    sc = score_cp_re.search(s)
                    if sc:
                        try:
                            score_cp = int(sc.group(1))
                        except Exception:
                            pass
                    tt = tthits_re.search(s)
                    if tt:
                        try:
                            tt_hits = float(tt.group(1))
                        except Exception:
                            pass
                    hf = hashfull_re.search(s)
                    if hf:
                        try:
                            hashfull = int(hf.group(1))
                        except Exception:
                            pass
        elif s.startswith("bestmove "):
            parts = s.split()
            if len(parts) >= 2:
                bestmove = parts[1]
            break

    # Quit
    try:
        send("quit")
    except Exception:
        pass
    try:
        proc.wait(timeout=2)
    except Exception:
        proc.kill()

    return {
        "depth": max_depth,
        "nodes": nodes_at_max_depth,
        "bestmove": bestmove or "",
        "score_cp": score_cp if score_cp is not None else "",
        "tt_hits": tt_hits if tt_hits is not None else "",
        "hashfull": hashfull if hashfull is not None else "",
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", action="append", required=True, help="Path to UCI engine binary (repeatable)")
    ap.add_argument("--fen", action="append", required=True, help="FEN string or 'startpos' (repeatable)")
    ap.add_argument("--time-ms", type=int, default=1000, help="Time per move in ms (default 1000)")
    ap.add_argument("--out", default="depth_vs_time.csv", help="Output CSV path")
    ap.add_argument("--uci-option", action="append", help="UCI option in format name=value (repeatable)")
    args = ap.parse_args()
    
    # Parse UCI options
    uci_options = {}
    if args.uci_option:
        for opt in args.uci_option:
            if '=' in opt:
                name, value = opt.split('=', 1)
                uci_options[name] = value

    ts = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    out_path = args.out

    rows = []
    for eng in args.engine:
        eng_path = os.path.abspath(eng)
        eng_name = os.path.basename(eng_path)
        for fen in args.fen:
            try:
                res = uci_run_search(eng_path, fen, args.time_ms, uci_options)
            except FileNotFoundError:
                print(f"ERROR: Engine not found: {eng_path}", file=sys.stderr)
                continue
            except Exception as e:
                print(f"ERROR: Engine '{eng_name}' failed on FEN: {fen[:60]}... ({e})", file=sys.stderr)
                continue
            rows.append({
                "timestamp": ts,
                "engine": eng_name,
                "engine_path": eng_path,
                "fen": fen,
                "time_ms": args.time_ms,
                "depth": res["depth"],
                "nodes": res["nodes"],
                "bestmove": res["bestmove"],
                "score_cp": res["score_cp"],
                "tt_hits": res["tt_hits"],
                "hashfull": res["hashfull"],
            })
            print(f"{eng_name}: depth={res['depth']} nodes={res['nodes']} tt_hits={res['tt_hits']}% hashfull={res['hashfull']} move={res['bestmove']} fen={(fen if fen=='startpos' else '...')}\n")

    # Write CSV
    os.makedirs(os.path.dirname(out_path) or '.', exist_ok=True)
    with open(out_path, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=[
            'timestamp','engine','engine_path','fen','time_ms','depth','nodes','bestmove','score_cp','tt_hits','hashfull'
        ])
        w.writeheader()
        for r in rows:
            w.writerow(r)

    print(f"Wrote {len(rows)} rows to {out_path}")


if __name__ == "__main__":
    main()

