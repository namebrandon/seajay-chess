#!/usr/bin/env python3
"""Collect stacked singular extension telemetry from SeaJay self-search runs."""
from __future__ import annotations

import subprocess
import sys
import argparse
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

ENGINE_PATH = Path(__file__).resolve().parent.parent / "bin" / "seajay"

DEFAULT_POSITIONS: List[Tuple[str, str]] = [
    (
        "Karpov-Kasparov 1985 G16",
        "8/8/4kpp1/3p1b2/p6P/2B5/6P1/6K1 b - - 0 47",
    ),
    (
        "Deep defensive resource",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    ),
    (
        "Endgame pawn race",
        "8/5pk1/4p1p1/7P/5P2/6K1/8/8 w - - 0 1",
    ),
    (
        "Complex middlegame",
        "r1bqkb1r/pp3ppp/2n1pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R b KQkq - 0 6",
    ),
    (
        "Singular exchange sac",
        "3r1rk1/p1q2pbp/1np1p1p1/8/2PNP3/1P3P2/PB3QPP/3R1RK1 w - - 0 1",
    ),
]

INFO_PREFIX = b"info "
SINGULAR_STACK_TAG = "SingularStack:"

STACK_KEYS = {
    "se_stack_c": "stack_candidates",
    "se_stack_a": "stack_applied",
    "se_stack_rd": "stack_rej_depth",
    "se_stack_re": "stack_rej_eval",
    "se_stack_rt": "stack_rej_tt",
    "se_stack_cl": "stack_clamped",
    "se_stack_x": "stack_extra_depth",
}

SINGULAR_KEYS = {
    "examined": "examined",
    "qualified": "qualified",
    "rej_illegal": "rej_illegal",
    "rej_tactical": "rej_tactical",
    "fail_low": "fail_low",
    "fail_high": "fail_high",
    "verified": "verified",
    "extended": "extended",
    "cacheHits": "cache_hits",
    "maxDepth": "max_depth",
}


def _read_until(proc: subprocess.Popen[bytes], token: bytes) -> None:
    while True:
        line = proc.stdout.readline()
        if not line:
            raise RuntimeError("engine terminated early while waiting for token")
        if token in line:
            return


def _handshake(proc: subprocess.Popen[bytes]) -> None:
    proc.stdin.write(b"uci\n")
    proc.stdin.flush()
    _read_until(proc, b"uciok")

    for option in (
        "setoption name UseSearchNodeAPIRefactor value true",
        "setoption name EnableExcludedMoveParam value true",
        "setoption name UseSingularExtensions value true",
        "setoption name AllowStackedExtensions value true",
        "setoption name SearchStats value true",
    ):
        proc.stdin.write(option.encode() + b"\n")
    proc.stdin.write(b"isready\n")
    proc.stdin.flush()
    _read_until(proc, b"readyok")
    proc.stdin.write(b"ucinewgame\n")
    proc.stdin.flush()


def _run_position(
    proc: subprocess.Popen[bytes],
    name: str,
    fen: str,
    go_cmd: str,
    aggregate: Dict[str, int],
) -> List[str]:
    proc.stdin.write(f"position fen {fen}\n".encode())
    proc.stdin.write(go_cmd.encode() + b"\n")
    proc.stdin.flush()

    captured_lines: List[str] = []
    while True:
        raw = proc.stdout.readline()
        if not raw:
            raise RuntimeError(f"engine terminated early during '{name}' search")
        text = raw.decode(errors="replace").strip()
        if text:
            captured_lines.append(text)
        if text.startswith("bestmove"):
            break

    for line in captured_lines:
        if "SingularStats:" in line:
            try:
                payload = line.split("SingularStats:", 1)[1].strip()
            except IndexError:
                payload = ""
            for entry in payload.split():
                if "=" not in entry:
                    continue
                key, value = entry.split("=", 1)
                label = STACK_KEYS.get(key)
                if label is None:
                    label = SINGULAR_KEYS.get(key)
                if label is None:
                    continue
                try:
                    aggregate[label] += int(value)
                except ValueError:
                    continue
        if SINGULAR_STACK_TAG in line:
            # Example: info string SingularStack: cand=12 app=3 ...
            try:
                payload = line.split(SINGULAR_STACK_TAG, 1)[1].strip()
            except IndexError:
                continue
            for entry in payload.split():
                if "=" not in entry:
                    continue
                key, value = entry.split("=", 1)
                label = STACK_KEYS.get(key, key)
                try:
                    aggregate[label] += int(value)
                except ValueError:
                    continue
        if INFO_PREFIX in line.encode():
            # parse se_stack_* fields in compact info lines
            parts = line.split()
            for part in parts:
                if "=" not in part:
                    continue
                key, value = part.split("=", 1)
                label = STACK_KEYS.get(key)
                if label is None:
                    continue
                try:
                    aggregate[label] += int(value)
                except ValueError:
                    continue
    return captured_lines


def _load_epd_positions(epd_path: Path, limit: int | None) -> List[Tuple[str, str]]:
    positions: List[Tuple[str, str]] = []
    with epd_path.open("r", encoding="utf-8") as handle:
        for idx, line in enumerate(handle, start=1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = line.split()
            if len(fields) < 4:
                continue
            fen = " ".join(fields[:6])
            positions.append((f"EPD-{idx}", fen))
            if limit is not None and len(positions) >= limit:
                break
    return positions


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--epd", type=Path, help="EPD file to draw test positions from")
    parser.add_argument("--limit", type=int, default=20, help="Maximum positions to run from the EPD file")
    parser.add_argument("--movetime", type=int, default=1000, help="Search time per position in milliseconds")
    parser.add_argument("--depth", type=int, help="Optional fixed depth to search instead of movetime")
    parser.add_argument("--no-default", action="store_true", help="Do not include the built-in test positions")
    args = parser.parse_args()

    if not ENGINE_PATH.exists():
        print(f"error: engine binary not found at {ENGINE_PATH}", file=sys.stderr)
        return 1

    positions: List[Tuple[str, str]] = []
    if not args.no_default:
        positions.extend(DEFAULT_POSITIONS)
    if args.epd:
        positions.extend(_load_epd_positions(args.epd, args.limit))
    if not positions:
        print("error: no positions to evaluate", file=sys.stderr)
        return 1

    if args.depth is not None:
        go_cmd = f"go depth {args.depth}"
    else:
        go_cmd = f"go movetime {args.movetime}"

    proc = subprocess.Popen(
        [str(ENGINE_PATH)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    assert proc.stdin is not None and proc.stdout is not None

    try:
        _handshake(proc)
        aggregate: Dict[str, int] = defaultdict(int)
        transcripts: List[Tuple[str, List[str]]] = []
        for name, fen in positions:
            lines = _run_position(proc, name, fen, go_cmd=go_cmd, aggregate=aggregate)
            transcripts.append((name, lines))

        proc.stdin.write(b"quit\n")
        proc.stdin.flush()
    finally:
        proc.wait(timeout=5)

    print("Stacked Extension Telemetry Summary")
    print("=================================")
    for key in (
        "stack_candidates",
        "stack_applied",
        "stack_rej_depth",
        "stack_rej_eval",
        "stack_rej_tt",
        "stack_clamped",
        "stack_extra_depth",
    ):
        print(f"{key.replace('stack_', ''):>13}: {aggregate.get(key, 0)}")

    print("\nCore Singular Stats")
    print("--------------------")
    for key in (
        "examined",
        "qualified",
        "rej_illegal",
        "rej_tactical",
        "verified",
        "fail_low",
        "fail_high",
        "extended",
    ):
        print(f"{key:>13}: {aggregate.get(key, 0)}")

    print("\nDetailed Logs (per position)")
    print("------------------------------")
    for name, lines in transcripts:
        print(f"[{name}]")
        for line in lines:
            if (
                SINGULAR_STACK_TAG in line
                or "SingularStats:" in line
                or any(field in line for field in STACK_KEYS)
            ):
                print(f"  {line}")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
