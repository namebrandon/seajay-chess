#!/usr/bin/env python3
"""Compare SeaJay vs Komodo (single FEN, depth ≤ 14) with per-engine 60s timeout."""

from __future__ import annotations

import argparse
import json
import threading
from pathlib import Path

import chess
import chess.engine

CLAMP_DEPTH = 14
PER_ENGINE_TIMEOUT = 60.0


def analyse(engine_path: Path, fen: str, depth: int, timeout: float) -> dict:
    board = chess.Board(fen)
    limit = chess.engine.Limit(depth=depth)
    engine = chess.engine.SimpleEngine.popen_uci(str(engine_path))
    result: dict | None = None
    exc: Exception | None = None

    def worker() -> None:
        nonlocal result, exc
        try:
            info = engine.analyse(board, limit)
            score = info.get("score")
            pov = score.pov(board.turn).score(mate_score=32000) if score else None
            pv = info.get("pv")
            bestmove = None
            if pv:
                temp_board = board.copy()
                bestmove = temp_board.san(pv[0])
            result = {
                "depth": info.get("depth"),
                "score_cp": pov,
                "bestmove": bestmove,
            }
        except Exception as e:
            exc = e

    thread = threading.Thread(target=worker, daemon=True)
    thread.start()
    thread.join(timeout)
    if thread.is_alive():
        try:
            engine.kill()
        except Exception:
            pass
        thread.join(1)
        raise TimeoutError(f"Engine {engine_path} exceeded {timeout}s at depth {depth}")
    try:
        engine.quit()
    except Exception:
        pass
    if exc:
        raise exc
    if result is None:
        raise RuntimeError(f"Engine {engine_path} returned no result")
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description="Single-position SeaJay vs Komodo comparison")
    parser.add_argument("fen", help="FEN string")
    parser.add_argument("--depth", type=int, default=CLAMP_DEPTH, help="Target depth (max 14)")
    parser.add_argument("--seajay", type=Path, default=Path("bin/seajay"))
    parser.add_argument("--komodo", type=Path, default=Path("external/engines/komodo/komodo-14.1-linux"))
    parser.add_argument("--output", type=Path, default=Path("docs/issues/eval_bias_tracker.json"))
    args = parser.parse_args()

    depth = min(args.depth, CLAMP_DEPTH)
    fen = args.fen

    komodo = analyse(args.komodo, fen, depth, PER_ENGINE_TIMEOUT)
    seajay = analyse(args.seajay, fen, depth, PER_ENGINE_TIMEOUT)

    entry = {
        "fen": fen,
        "depth": depth,
        "komodo": komodo,
        "seajay": seajay,
    }

    # Append to JSON list (create if absent)
    if args.output.exists():
        data = json.loads(args.output.read_text())
    else:
        data = []
    data = [item for item in data if item.get("fen") != fen]
    data.append(entry)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(data, indent=2))

    # Print simple markdown row for convenience
    delta = None
    if seajay["score_cp"] is not None and komodo["score_cp"] is not None:
        delta = seajay["score_cp"] - komodo["score_cp"]
    print(f"| `{fen}` | {depth} | {seajay['score_cp']} | {komodo['score_cp']} | {delta} | {seajay['bestmove']} | {komodo['bestmove']} |")


if __name__ == "__main__":
    main()
