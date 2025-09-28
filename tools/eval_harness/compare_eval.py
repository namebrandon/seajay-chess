#!/usr/bin/env python3
"""SeaJay evaluation comparison harness.

Runs SeaJay (and optionally a reference UCI engine) across a pack of FEN/EPD
positions, gathers search summaries plus structured `info eval` telemetry, and
exports a JSON report for downstream analysis.
"""

from __future__ import annotations

import argparse
import json
import queue
import subprocess
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional

LINE_PREFIX_EVAL = "info eval "


# ---------------------------------------------------------------------------
# Utility helpers
# ---------------------------------------------------------------------------

def _safe_int(value: str) -> Optional[int]:
    try:
        if value.startswith("0x") or value.startswith("0X"):
            return int(value, 16)
        return int(value)
    except ValueError:
        return None


@dataclass
class Position:
    index: int
    fen: str
    label: Optional[str] = None


# ---------------------------------------------------------------------------
# Position pack loading
# ---------------------------------------------------------------------------

def load_pack(path: Path) -> List[Position]:
    positions: List[Position] = []
    with path.open("r", encoding="ascii") as handle:
        for raw in handle:
            stripped = raw.strip()
            if not stripped or stripped.startswith("#"):
                continue

            fen_part = stripped
            label = None
            if " c0 " in stripped:
                fen_part, label_part = stripped.split(" c0 ", maxsplit=1)
                fen_part = fen_part.rstrip(";")
                label = label_part.strip().rstrip(";")
                if label.startswith('"') and label.endswith('"'):
                    label = label[1:-1]
            else:
                fen_part = fen_part.rstrip(";")

            positions.append(Position(index=len(positions) + 1, fen=fen_part, label=label))

    if not positions:
        raise ValueError(f"No positions found in {path}")
    return positions


# ---------------------------------------------------------------------------
# UCI engine driver
# ---------------------------------------------------------------------------

class UCIEngineProcess:
    def __init__(
        self,
        binary: Path,
        threads: int = 1,
        enable_eval_extended: bool = False,
        eval_log_file: Optional[Path] = None,
    ) -> None:
        self.binary = binary
        self.threads = threads
        self.enable_eval_extended = enable_eval_extended
        self.eval_log_file = eval_log_file
        self._queue: "queue.Queue[tuple[str, str | None]]" = queue.Queue()
        self._proc = subprocess.Popen(
            [str(self.binary)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        assert self._proc.stdin
        assert self._proc.stdout
        assert self._proc.stderr

        self._stdout_thread = threading.Thread(
            target=self._drain_stream,
            args=("stdout", self._proc.stdout),
            daemon=True,
        )
        self._stderr_thread = threading.Thread(
            target=self._drain_stream,
            args=("stderr", self._proc.stderr),
            daemon=True,
        )
        self._stdout_thread.start()
        self._stderr_thread.start()

        self.id_name: Optional[str] = None
        self.id_author: Optional[str] = None

        self._initialize()

    # -------------------
    def _drain_stream(self, name: str, stream) -> None:
        for raw in iter(stream.readline, ""):
            self._queue.put((name, raw.rstrip("\r\n")))
        self._queue.put((name, None))

    # -------------------
    def _write(self, command: str) -> None:
        assert self._proc.stdin
        self._proc.stdin.write(command + "\n")
        self._proc.stdin.flush()

    # -------------------
    def _wait_ready(self, timeout: float = 10.0) -> List[str]:
        deadline = time.time() + timeout
        lines: List[str] = []
        while True:
            remaining = deadline - time.time()
            if remaining <= 0:
                raise TimeoutError("timeout waiting for readyok")
            try:
                stream, line = self._queue.get(timeout=remaining)
            except queue.Empty:
                continue
            if line is None:
                continue
            if line == "readyok":
                return lines
            lines.append(line)

    # -------------------
    def _wait_for_token(self, token: str, timeout: float = 10.0) -> List[str]:
        deadline = time.time() + timeout
        lines: List[str] = []
        while True:
            remaining = deadline - time.time()
            if remaining <= 0:
                raise TimeoutError(f"timeout waiting for {token}")
            try:
                stream, line = self._queue.get(timeout=remaining)
            except queue.Empty:
                continue
            if line is None:
                continue
            lines.append(line)
            if line == token:
                return lines

    # -------------------
    def _initialize(self) -> None:
        self._write("uci")
        lines = self._wait_for_token("uciok")
        for line in lines:
            if line.startswith("id name "):
                self.id_name = line[len("id name "):]
            elif line.startswith("id author "):
                self.id_author = line[len("id author "):]
        self._write("isready")
        self._wait_ready()

        if self.threads:
            self._write(f"setoption name Threads value {self.threads}")
        if self.enable_eval_extended:
            self._write("setoption name EvalExtended value true")
            if self.eval_log_file:
                self._write(f"setoption name EvalLogFile value {self.eval_log_file}")

        # Flush any info strings emitted while setting options
        self._write("isready")
        self._wait_ready()

    # -------------------
    def position(self, fen: str) -> None:
        self._write(f"position fen {fen}")

    # -------------------
    def go(self, movetime: Optional[int], depth: Optional[int]) -> List[str]:
        cmd = ["go"]
        if depth is not None:
            cmd += ["depth", str(depth)]
        if movetime is not None:
            cmd += ["movetime", str(movetime)]
        self._write(" ".join(cmd))
        self._write("isready")
        return self._wait_ready()

    # -------------------
    def debug_eval(self) -> List[str]:
        self._write("debug eval")
        self._write("isready")
        return self._wait_ready()

    # -------------------
    def stop(self) -> None:
        try:
            self._write("quit")
        except BrokenPipeError:
            pass
        self._proc.wait(timeout=5)

    # -------------------
    def __del__(self) -> None:
        if self._proc.poll() is None:
            try:
                self.stop()
            except Exception:
                pass


# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

def parse_search_output(lines: List[str]) -> Dict[str, object]:
    summary: Dict[str, object] = {
        "info": [line for line in lines if line.startswith("info ")],
    }

    bestmove_line = next((line for line in lines if line.startswith("bestmove ")), None)
    if bestmove_line:
        parts = bestmove_line.split()
        summary["bestmove"] = parts[1]
        if len(parts) >= 4 and parts[2] == "ponder":
            summary["ponder"] = parts[3]

    score_line = None
    for line in summary["info"]:
        if " score " in f" {line} ":
            score_line = line
    if score_line:
        parts = score_line.split()
        it = iter(range(len(parts)))
        for idx in it:
            token = parts[idx]
            if token == "depth" and idx + 1 < len(parts):
                summary["depth"] = _safe_int(parts[idx + 1])
            elif token == "seldepth" and idx + 1 < len(parts):
                summary["seldepth"] = _safe_int(parts[idx + 1])
            elif token == "nodes" and idx + 1 < len(parts):
                summary["nodes"] = _safe_int(parts[idx + 1])
            elif token == "time" and idx + 1 < len(parts):
                summary["time_ms"] = _safe_int(parts[idx + 1])
            elif token == "nps" and idx + 1 < len(parts):
                summary["nps"] = _safe_int(parts[idx + 1])
            elif token == "score" and idx + 2 < len(parts):
                score_type = parts[idx + 1]
                score_value = parts[idx + 2]
                summary["score_type"] = score_type
                if score_type == "cp":
                    summary["score_cp"] = _safe_int(score_value)
                elif score_type == "mate":
                    summary["score_mate"] = _safe_int(score_value)
                # Skip potential flags like lowerbound/upperbound
    return summary


def parse_eval_output(lines: List[str]) -> Optional[Dict[str, object]]:
    eval_lines = [line for line in lines if line.startswith(LINE_PREFIX_EVAL)]
    if not eval_lines:
        return None

    result: Dict[str, object] = {
        "terms": {}
    }

    for line in eval_lines:
        payload = line[len(LINE_PREFIX_EVAL):]
        parts = payload.split()
        if not parts:
            continue
        kind = parts[0]
        kv: Dict[str, str] = {}
        for token in parts[1:]:
            if "=" not in token:
                continue
            key, value = token.split("=", maxsplit=1)
            kv[key] = value

        if kind == "header":
            result["header"] = {k: _safe_int(v) or v for k, v in kv.items()}
        elif kind == "summary":
            converted = {}
            for k, v in kv.items():
                converted[k] = _safe_int(v) if k != "side" else v
            result["summary"] = converted
        elif kind == "phase":
            result["phase"] = {k: _safe_int(v) for k, v in kv.items()}
        elif kind == "pawn_cache":
            cache_entry = dict(kv)
            cache_entry["hit"] = kv.get("hit") == "1"
            cache_key = kv.get("key")
            if cache_key is not None:
                cache_entry["key_int"] = _safe_int(cache_key)
            result["pawn_cache"] = cache_entry
        elif kind == "term":
            name = kv.get("name", "unknown")
            term_data: Dict[str, object] = {}
            for k, v in kv.items():
                if k == "name":
                    continue
                maybe = _safe_int(v)
                term_data[k] = maybe if maybe is not None else v
            result["terms"][name] = term_data
        elif kind == "total":
            result["total"] = {k: _safe_int(v) for k, v in kv.items()}
        else:
            # Preserve any future payloads generically
            result.setdefault("extras", []).append({"kind": kind, "data": kv})

    return result


# ---------------------------------------------------------------------------
# Harness workflow
# ---------------------------------------------------------------------------

def run_engine_on_positions(
    engine: UCIEngineProcess,
    positions: List[Position],
    movetime: Optional[int],
    depth: Optional[int],
    collect_eval: bool,
) -> List[Dict[str, object]]:
    results: List[Dict[str, object]] = []
    for pos in positions:
        engine.position(pos.fen)
        start = time.time()
        search_lines = engine.go(movetime=movetime, depth=depth)
        elapsed_ms = int(round((time.time() - start) * 1000))
        search_summary = parse_search_output(search_lines)
        search_summary["elapsed_ms"] = elapsed_ms
        search_summary["raw"] = search_lines

        eval_summary: Optional[Dict[str, object]] = None
        if collect_eval:
            eval_lines = engine.debug_eval()
            eval_summary = parse_eval_output(eval_lines)
            if eval_summary is not None:
                eval_summary["raw"] = eval_lines

        results.append(
            {
                "search": search_summary,
                "evaluation": eval_summary,
            }
        )
    return results


# ---------------------------------------------------------------------------
# CLI entrypoint
# ---------------------------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="SeaJay evaluation comparison harness")
    parser.add_argument("--engine", type=Path, required=True, help="Path to SeaJay binary")
    parser.add_argument("--pack", type=Path, required=True, help="FEN/EPD pack to evaluate")
    parser.add_argument("--out", type=Path, required=True, help="Output JSON report path")
    parser.add_argument("--movetime", type=int, help="Per-position movetime in ms")
    parser.add_argument("--depth", type=int, help="Fixed search depth")
    parser.add_argument("--threads", type=int, default=1, help="Threads to request via UCI")
    parser.add_argument("--eval-log", type=Path, help="Optional EvalLogFile path for SeaJay")
    parser.add_argument("--ref-engine", type=Path, help="Optional reference engine binary")
    parser.add_argument("--ref-name", type=str, help="Override display name for reference engine")
    return parser


def main(argv: Optional[Iterable[str]] = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    positions = load_pack(args.pack)
    print(f"Loaded {len(positions)} positions from {args.pack}")

    metadata: Dict[str, object] = {
        "pack": str(args.pack),
        "options": {
            "movetime": args.movetime,
            "depth": args.depth,
            "threads": args.threads,
        },
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }

    seajay_engine = UCIEngineProcess(
        binary=args.engine,
        threads=args.threads,
        enable_eval_extended=True,
        eval_log_file=args.eval_log,
    )
    metadata["engine"] = {
        "path": str(args.engine),
        "id": seajay_engine.id_name,
        "author": seajay_engine.id_author,
    }

    try:
        seajay_results = run_engine_on_positions(
            engine=seajay_engine,
            positions=positions,
            movetime=args.movetime,
            depth=args.depth,
            collect_eval=True,
        )
    finally:
        seajay_engine.stop()

    report_positions: List[Dict[str, object]] = []
    for base, pos in zip(seajay_results, positions):
        entry: Dict[str, object] = {
            "index": pos.index,
            "fen": pos.fen,
            "label": pos.label,
            "seajay": base,
        }
        report_positions.append(entry)

    if args.ref_engine:
        ref_engine = UCIEngineProcess(
            binary=args.ref_engine,
            threads=args.threads,
            enable_eval_extended=False,
        )
        metadata["reference_engine"] = {
            "path": str(args.ref_engine),
            "id": args.ref_name or ref_engine.id_name,
            "author": ref_engine.id_author,
        }
        try:
            ref_results = run_engine_on_positions(
                engine=ref_engine,
                positions=positions,
                movetime=args.movetime,
                depth=args.depth,
                collect_eval=False,
            )
        finally:
            ref_engine.stop()

        for entry, ref in zip(report_positions, ref_results):
            entry["reference"] = ref

    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open("w", encoding="utf-8") as handle:
        json.dump({"metadata": metadata, "positions": report_positions}, handle, indent=2)

    print(f"Report written to {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
