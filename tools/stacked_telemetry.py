#!/usr/bin/env python3
"""Collect stacked singular extension telemetry from SeaJay self-search runs."""
from __future__ import annotations

import subprocess
import sys
import argparse
from pathlib import Path
from typing import Any, Dict, Iterable, List, Tuple

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
SINGULAR_SLACK_TAG = "SingularSlack:"

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
    "nodes": "nodes",
    "tt_exact": "tt_exact",
    "expanded": "expanded",
    "extended": "extended",
    "slack_low": "slack_low",
    "slack_high": "slack_high",
    "cacheHits": "cache_hits",
    "maxDepth": "max_depth",
}


def _accumulate_buckets(dest: List[int] | None, source: Iterable[int]) -> List[int]:
    values = list(source)
    if dest is None:
        return values
    if len(dest) < len(values):
        dest.extend([0] * (len(values) - len(dest)))
    for index, value in enumerate(values):
        dest[index] += value
    return dest


def _bucket_percentiles(counts: Iterable[int], bucket_width: int, percentiles: Iterable[float]) -> List[int]:
    counts_list = list(counts)
    total = sum(counts_list)
    if total == 0:
        return [0 for _ in percentiles]
    results: List[int] = []
    for pct in percentiles:
        threshold = pct * total
        cumulative = 0
        bucket_value = bucket_width * len(counts_list)
        for idx, count in enumerate(counts_list):
            cumulative += count
            if cumulative >= threshold:
                bucket_value = bucket_width * (idx + 1)
                break
        results.append(bucket_value)
    return results


def _read_until(proc: subprocess.Popen[bytes], token: bytes) -> None:
    while True:
        line = proc.stdout.readline()
        if not line:
            raise RuntimeError("engine terminated early while waiting for token")
        if token in line:
            return


def _handshake(proc: subprocess.Popen[bytes], bypass_tt_exact: bool) -> None:
    proc.stdin.write(b"uci\n")
    proc.stdin.flush()
    _read_until(proc, b"uciok")

    options = [
        "setoption name UseSearchNodeAPIRefactor value true",
        "setoption name EnableExcludedMoveParam value true",
        "setoption name UseSingularExtensions value true",
        "setoption name AllowStackedExtensions value true",
        "setoption name SearchStats value true",
    ]
    if bypass_tt_exact:
        options.append("setoption name BypassSingularTTExact value true")

    for option in options:
        proc.stdin.write(option.encode() + b"\n")
    proc.stdin.write(b"isready\n")
    proc.stdin.flush()
    _read_until(proc, b"readyok")
    proc.stdin.write(b"ucinewgame\n")
    proc.stdin.flush()


def _reset_engine(proc: subprocess.Popen[bytes]) -> None:
    proc.stdin.write(b"ucinewgame\n")
    proc.stdin.flush()
    proc.stdin.write(b"isready\n")
    proc.stdin.flush()
    _read_until(proc, b"readyok")


def _run_position(
    proc: subprocess.Popen[bytes],
    name: str,
    fen: str,
    go_cmd: str,
    aggregate: Dict[str, Any],
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
                    aggregate[label] = aggregate.get(label, 0) + int(value)
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
                    aggregate[label] = aggregate.get(label, 0) + int(value)
                except ValueError:
                    continue
        if SINGULAR_SLACK_TAG in line:
            try:
                payload = line.split(SINGULAR_SLACK_TAG, 1)[1].strip()
            except IndexError:
                payload = ""
            parts = payload.split()
            bucket_width = None
            low_values: List[int] | None = None
            high_values: List[int] | None = None
            for part in parts:
                if part.startswith("bucket="):
                    try:
                        bucket_width = int(part.split("=", 1)[1])
                    except ValueError:
                        bucket_width = None
                elif part.startswith("low="):
                    try:
                        low_values = [int(v) for v in part.split("=", 1)[1].split(",") if v]
                    except ValueError:
                        low_values = None
                elif part.startswith("high="):
                    try:
                        high_values = [int(v) for v in part.split("=", 1)[1].split(",") if v]
                    except ValueError:
                        high_values = None
            if bucket_width is not None:
                existing_width = aggregate.get("slack_bucket_width")
                if existing_width is None:
                    aggregate["slack_bucket_width"] = bucket_width
                elif existing_width != bucket_width:
                    raise RuntimeError(
                        f"inconsistent bucket width {bucket_width}; expected {existing_width}"
                    )
            if low_values is not None:
                aggregate["slack_low_buckets"] = _accumulate_buckets(
                    aggregate.get("slack_low_buckets"), low_values
                )
            if high_values is not None:
                aggregate["slack_high_buckets"] = _accumulate_buckets(
                    aggregate.get("slack_high_buckets"), high_values
                )
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
                    aggregate[label] = aggregate.get(label, 0) + int(value)
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


_STACK_SUMMARY_KEYS: Tuple[Tuple[str, str], ...] = (
    ("stack_candidates", "candidates"),
    ("stack_applied", "applied"),
    ("stack_rej_depth", "rej_depth"),
    ("stack_rej_eval", "rej_eval"),
    ("stack_rej_tt", "rej_tt"),
    ("stack_clamped", "clamped"),
    ("stack_extra_depth", "extra_depth"),
)

_SINGULAR_SUMMARY_KEYS: Tuple[Tuple[str, str], ...] = (
    ("examined", "examined"),
    ("qualified", "qualified"),
    ("rej_illegal", "rej_illegal"),
    ("rej_tactical", "rej_tactical"),
    ("verified", "verified"),
    ("fail_low", "fail_low"),
    ("fail_high", "fail_high"),
    ("nodes", "nodes"),
    ("tt_exact", "tt_exact"),
    ("expanded", "expanded"),
    ("extended", "extended"),
    ("slack_low", "slack_low"),
    ("slack_high", "slack_high"),
)


def _merge_aggregates(destination: Dict[str, Any], source: Dict[str, Any]) -> Dict[str, Any]:
    for key, value in source.items():
        if key in {"slack_low_buckets", "slack_high_buckets"}:
            destination[key] = _accumulate_buckets(destination.get(key), value)
        elif key == "slack_bucket_width":
            existing = destination.get(key)
            if existing is None:
                destination[key] = value
            elif existing != value:
                raise RuntimeError(
                    f"inconsistent slack bucket width detected: {value} vs {existing}"
                )
        else:
            destination[key] = destination.get(key, 0) + value
    return destination


def _print_summary(aggregate: Dict[str, Any]) -> None:
    print("Stacked Extension Telemetry Summary")
    print("=================================")
    for key, label in _STACK_SUMMARY_KEYS:
        print(f"{label:>13}: {aggregate.get(key, 0)}")

    print("\nCore Singular Stats")
    print("--------------------")
    for key, label in _SINGULAR_SUMMARY_KEYS:
        print(f"{label:>13}: {aggregate.get(key, 0)}")

    bucket_width = aggregate.get("slack_bucket_width")
    slack_low = aggregate.get("slack_low_buckets")
    slack_high = aggregate.get("slack_high_buckets")
    if bucket_width and slack_low:
        percentiles = _bucket_percentiles(slack_low, bucket_width, (0.5, 0.9, 0.95))
        print("\nFail-low Slack Percentiles (cp)")
        print("------------------------------")
        for label, value in zip(("p50", "p90", "p95"), percentiles):
            print(f"{label:>8}: {value}")
    if bucket_width and slack_high:
        percentiles = _bucket_percentiles(slack_high, bucket_width, (0.5, 0.9, 0.95))
        print("\nFail-high Slack Percentiles (cp)")
        print("-------------------------------")
        for label, value in zip(("p50", "p90", "p95"), percentiles):
            print(f"{label:>8}: {value}")


def _print_transcripts(transcripts: List[Tuple[str, List[str]]]) -> None:
    if not transcripts:
        return
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


def _print_report(
    title: str,
    aggregate: Dict[str, Any],
    transcripts: List[Tuple[str, List[str]]] | None,
) -> None:
    print(f"\n{title}")
    print("=" * (len(title) if title else 1))
    _print_summary(aggregate)
    if transcripts is not None:
        _print_transcripts(transcripts)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--epd", type=Path, help="EPD file to draw test positions from")
    parser.add_argument("--limit", type=int, help="Maximum positions to evaluate after offset is applied")
    parser.add_argument("--movetime", type=int, default=1000, help="Search time per position in milliseconds")
    parser.add_argument("--depth", type=int, help="Optional fixed depth to search instead of movetime")
    parser.add_argument("--no-default", action="store_true", help="Do not include the built-in test positions")
    parser.add_argument(
        "--bypass-tt-exact",
        action="store_true",
        help="Enable the BypassSingularTTExact UCI toggle during telemetry runs",
    )
    parser.add_argument(
        "--offset",
        type=int,
        default=0,
        help="Skip the first N positions after combining defaults and EPD entries",
    )
    parser.add_argument(
        "--chunk-size",
        type=int,
        help="Process positions in batches of this size (default: all positions in one chunk)",
    )
    parser.add_argument(
        "--max-chunks",
        type=int,
        help="Stop after processing this many chunks (use with --offset to resume later)",
    )
    parser.add_argument(
        "--passes",
        type=int,
        default=1,
        help="Repeat the position set this many times (names are suffixed per pass)",
    )
    args = parser.parse_args()

    if not ENGINE_PATH.exists():
        print(f"error: engine binary not found at {ENGINE_PATH}", file=sys.stderr)
        return 1

    positions: List[Tuple[str, str]] = []
    if not args.no_default:
        positions.extend(DEFAULT_POSITIONS)
    if args.epd:
        epd_limit = None if args.limit is None else args.limit + args.offset
        positions.extend(_load_epd_positions(args.epd, epd_limit))

    passes = args.passes if args.passes and args.passes > 0 else 1
    if passes > 1 and positions:
        repeated: List[Tuple[str, str]] = []
        for pass_index in range(passes):
            suffix = f"#p{pass_index + 1}"
            for name, fen in positions:
                repeated.append((f"{name}{suffix}", fen))
        positions = repeated

    if args.offset:
        if args.offset >= len(positions):
            positions = []
        else:
            positions = positions[args.offset :]
    if args.limit is not None:
        positions = positions[: args.limit]
    if not positions:
        print("error: no positions to evaluate", file=sys.stderr)
        return 1

    if args.depth is not None:
        go_cmd = f"go depth {args.depth}"
    else:
        go_cmd = f"go movetime {args.movetime}"

    chunk_size = args.chunk_size if args.chunk_size and args.chunk_size > 0 else len(positions)
    chunk_count = max(1, (len(positions) + chunk_size - 1) // chunk_size)

    proc = subprocess.Popen(
        [str(ENGINE_PATH)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    assert proc.stdin is not None and proc.stdout is not None

    overall: Dict[str, Any] = {}
    chunks_executed = 0

    try:
        _handshake(proc, bypass_tt_exact=args.bypass_tt_exact)
        for chunk_index in range(chunk_count):
            if args.max_chunks is not None and chunk_index >= args.max_chunks:
                break
            start = chunk_index * chunk_size
            end = min(len(positions), start + chunk_size)
            chunk_positions = positions[start:end]
            if not chunk_positions:
                continue

            _reset_engine(proc)

            chunk_aggregate: Dict[str, Any] = {}
            chunk_transcripts: List[Tuple[str, List[str]]] = []
            for name, fen in chunk_positions:
                lines = _run_position(proc, name, fen, go_cmd=go_cmd, aggregate=chunk_aggregate)
                chunk_transcripts.append((name, lines))

            chunks_executed += 1
            _print_report(
                f"Chunk {chunks_executed}/{chunk_count}",
                chunk_aggregate,
                chunk_transcripts,
            )
            _merge_aggregates(overall, chunk_aggregate)

        proc.stdin.write(b"quit\n")
        proc.stdin.flush()
    finally:
        proc.wait(timeout=5)

    if chunks_executed == 0:
        print("warning: no chunks were processed", file=sys.stderr)
        return 1

    if chunks_executed > 1:
        _print_report("Overall Totals", overall, transcripts=None)
    elif chunks_executed == 1:
        # Single chunk already printed detailed report; echo aggregate summary for clarity.
        _print_report("Overall Totals", overall, transcripts=None)

    return 0


if __name__ == "__main__":
    sys.exit(main())
