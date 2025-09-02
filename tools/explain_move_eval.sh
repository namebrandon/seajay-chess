#!/usr/bin/env bash

set -euo pipefail

# Explain eval impact of a single move from a FEN.
# - Runs SeaJay's UCI eval on the base position and after applying the move
# - Prints Piece-Square Tables and Total deltas (White's perspective)
#
# Usage:
#   tools/explain_move_eval.sh "<FEN>" <uci-move> [--engine ./bin/seajay] [--no-build]

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 \"<FEN>\" <uci-move> [--engine ./bin/seajay] [--no-build]" >&2
  exit 2
fi

FEN="$1"
MOVE="$2"
shift 2 || true

ENGINE="./bin/seajay"
DO_BUILD=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --engine)
      ENGINE="$2"; shift 2 ;;
    --no-build)
      DO_BUILD=0; shift ;;
    *)
      echo "Unknown argument: $1" >&2; exit 2 ;;
  esac
done

if [[ $DO_BUILD -eq 1 ]]; then
  if [[ ! -x "$ENGINE" ]]; then
    echo "[build] ./build.sh Release" >&2
    ./build.sh Release >/dev/null
  fi
fi

if [[ ! -x "$ENGINE" ]]; then
  echo "ERROR: Engine not found: $ENGINE" >&2
  exit 1
fi

tmp=$(mktemp)
cleanup() { rm -f "$tmp"; }
trap cleanup EXIT

# Drive UCI to gather eval breakdowns
{
  echo "uci"
  echo "isready"
  echo "position fen ${FEN}"
  echo "eval"
  echo "position fen ${FEN} moves ${MOVE}"
  echo "eval"
  echo "quit"
} | "$ENGINE" >"$tmp" 2>&1 || true

extract_eval() {
  awk '
    BEGIN{show=0}
    /\+--- Evaluation Breakdown/ {show=1}
    show && /Piece-Square Tables:/ {print $NF}
    show && /Total:/ {print $NF; exit}
  ' "$tmp"
}

readarray -t vals < <(extract_eval)
if [[ ${#vals[@]} -ne 4 ]]; then
  echo "ERROR: Failed to parse eval output" >&2
  exit 1
fi

base_pst=${vals[0]}
base_total=${vals[1]}
after_pst=${vals[2]}
after_total=${vals[3]}

to_int() {
  local s="$1"
  # strip leading +
  echo "${s#+}"
}

bp=$(to_int "$base_pst")
bt=$(to_int "$base_total")
ap=$(to_int "$after_pst")
at=$(to_int "$after_total")

dp=$(( ap - bp ))
dt=$(( at - bt ))

echo "FEN:   $FEN"
echo "Move:  $MOVE"
echo
printf "Piece-Square Tables:  base=%4s  after=%4s  delta=%+d cp\n" "$base_pst" "$after_pst" "$dp"
printf "Total (White persp):  base=%4s  after=%4s  delta=%+d cp\n" "$base_total" "$after_total" "$dt"

