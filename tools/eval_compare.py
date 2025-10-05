#!/usr/bin/env python3
"""Compare SeaJay vs Komodo on one or many FENs, tracking multiple depths."""

from __future__ import annotations

import argparse
import json
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable

import chess
import chess.engine

CLAMP_DEPTH = 18
DEFAULT_TIMEOUT = 240.0


@dataclass
class EngineResult:
    depth: int | None
    score_cp: int | None
    bestmove: str | None


def analyse(engine_path: Path, fen: str, depth: int, timeout: float) -> EngineResult:
    board = chess.Board(fen)
    limit = chess.engine.Limit(depth=depth)
    engine = chess.engine.SimpleEngine.popen_uci(str(engine_path))
    result: EngineResult | None = None
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
            result = EngineResult(
                depth=info.get("depth"),
                score_cp=pov,
                bestmove=bestmove,
            )
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


def load_entries(path: Path) -> Dict[str, dict]:
    if not path.exists():
        return {}
    raw = json.loads(path.read_text())
    entries: Dict[str, dict] = {}
    for item in raw:
        fen = item["fen"]
        if "evaluations" in item:
            entries[fen] = {
                "evaluations": item["evaluations"],
                **({"notes": item["notes"]} if "notes" in item else {}),
            }
            continue
        depth = item.get("depth")
        evals = {
            str(depth): {
                "target_depth": depth,
                "seajay": item.get("seajay", {}),
                "komodo": item.get("komodo", {}),
            }
        }
        entries[fen] = {"evaluations": evals}
    return entries


def dump_entries(path: Path, entries: Dict[str, dict]) -> None:
    serialisable = []
    for fen in sorted(entries.keys()):
        entry = {"fen": fen, "evaluations": entries[fen]["evaluations"]}
        if "notes" in entries[fen]:
            entry["notes"] = entries[fen]["notes"]
        serialisable.append(entry)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(serialisable, indent=2))


def write_markdown(path: Path, entries: Dict[str, dict]) -> None:
    lines = [
        "# Evaluation Bias Tracker",
        "",
        "| FEN | Depth | SeaJay (cp) | Komodo (cp) | Delta | SeaJay Best | Komodo Best |",
        "| --- | ---: | ---: | ---: | ---: | --- | --- |",
    ]
    for fen in sorted(entries.keys()):
        evals = entries[fen]["evaluations"]
        for depth_key in sorted(evals.keys(), key=lambda d: int(d)):
            data = evals[depth_key]
            target_depth = data.get("target_depth") or depth_key
            seajay = data.get("seajay", {})
            komodo = data.get("komodo", {})
            sj_score = seajay.get("score_cp")
            kd_score = komodo.get("score_cp")
            delta = None
            if sj_score is not None and kd_score is not None:
                delta = sj_score - kd_score
            depth_display = seajay.get("depth") or komodo.get("depth") or target_depth
            lines.append(
                "| `{fen}` | {depth} | {sj} | {kd} | {delta} | {sj_move} | {kd_move} |".format(
                    fen=fen,
                    depth=depth_display,
                    sj=sj_score if sj_score is not None else "",
                    kd=kd_score if kd_score is not None else "",
                    delta=delta if delta is not None else "",
                    sj_move=seajay.get("bestmove", "") or "",
                    kd_move=komodo.get("bestmove", "") or "",
                )
            )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n")


def run_for_fen(
    entries: Dict[str, dict],
    fen: str,
    depth: int,
    timeout: float,
    seajay_path: Path,
    komodo_path: Path,
) -> None:
    entry = entries.setdefault(fen, {"evaluations": {}})
    evals = entry["evaluations"]
    depth_key = str(depth)

    komodo = analyse(komodo_path, fen, depth, timeout)
    seajay = analyse(seajay_path, fen, depth, timeout)
    evals[depth_key] = {
        "target_depth": depth,
        "seajay": seajay.__dict__,
        "komodo": komodo.__dict__,
    }


def iter_fens(entries: Dict[str, dict], explicit_fen: str | None) -> Iterable[str]:
    if explicit_fen:
        yield explicit_fen
    else:
        for fen in sorted(entries.keys()):
            yield fen


def main() -> None:
    parser = argparse.ArgumentParser(description="SeaJay vs Komodo evaluation comparison")
    parser.add_argument("fen", nargs="?", help="FEN string (omit with --all)")
    parser.add_argument("--depth", type=int, default=CLAMP_DEPTH, help="Target depth (max 18)")
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT, help="Per-engine timeout in seconds")
    parser.add_argument("--seajay", type=Path, default=Path("bin/seajay"))
    parser.add_argument("--komodo", type=Path, default=Path("external/engines/komodo/komodo-14.1-linux"))
    parser.add_argument("--output", type=Path, default=Path("docs/issues/eval_bias_tracker.json"))
    parser.add_argument("--markdown", type=Path, default=Path("docs/issues/eval_bias_tracker.md"))
    parser.add_argument("--all", action="store_true", help="Run against every FEN already tracked in the output file")
    parser.add_argument("--force", action="store_true", help="Recompute even if data for the requested depth already exists")
    parser.add_argument("--convert-only", action="store_true", help="Rewrite output/markdown without running engines")
    args = parser.parse_args()

    depth = min(args.depth, CLAMP_DEPTH)
    entries = load_entries(args.output)

    if args.convert_only:
        dump_entries(args.output, entries)
        write_markdown(args.markdown, entries)
        return

    if not args.all and not args.fen:
        parser.error("provide a FEN or use --all")

    fens = list(iter_fens(entries, args.fen))
    if args.all and not fens:
        parser.error("no existing FENs to process; add one via positional argument first")

    for fen in fens:
        depth_key = str(depth)
        existing = entries.get(fen, {}).get("evaluations", {})
        if not args.force and depth_key in existing:
            print(f"Skipping FEN at depth {depth} (already recorded): {fen}")
            continue
        print(f"Analysing depth {depth}: {fen}")
        try:
            run_for_fen(entries, fen, depth, args.timeout, args.seajay, args.komodo)
        except TimeoutError as exc:
            print(f"Timeout: {exc}")
            continue
        except Exception as exc:  # pragma: no cover - defensive
            print(f"Error analysing {fen}: {exc}")
            continue
        dump_entries(args.output, entries)
        write_markdown(args.markdown, entries)

    dump_entries(args.output, entries)
    write_markdown(args.markdown, entries)


if __name__ == "__main__":
    main()
