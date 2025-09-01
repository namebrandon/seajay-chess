#!/usr/bin/env bash

set -euo pipefail

# Eval compare harness
# - Builds Release (unless --no-build)
# - Runs UCI 'eval' breakdown on reference FENs
# - Prints concise summaries for quick human review
#
# Usage:
#   tools/eval_compare.sh [--no-build] [--engine ./bin/seajay] \
#       [--fen1 "<FEN>"] [--fen2 "<FEN>"] [--fen3 "<FEN>"]
#
# Notes:
# - Build process follows docs/BUILD_SYSTEM.md (CMake via build.sh Release)
# - For OpenBench/production, use Makefile (not invoked here)

ENGINE="./bin/seajay"
DO_BUILD=1

# Defaults from the remediation plan
FEN1='r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/1R2R1K1 w kq - 2 17'
FEN2='8/5p2/2R2Pk1/5r1p/5P1P/5KP1/8/8 b - - 26 82'
FEN3='rnbqkbnr/ppppp3/8/8/8/5N2/PPPPPPPP/RNBQ1BKR w KQ - 0 1'

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-build)
      DO_BUILD=0
      shift
      ;;
    --engine)
      ENGINE="$2"
      shift 2
      ;;
    --fen1)
      FEN1="$2"
      shift 2
      ;;
    --fen2)
      FEN2="$2"
      shift 2
      ;;
    --fen3)
      FEN3="$2"
      shift 2
      ;;
    -h|--help)
      grep -E '^#( |$)' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if [[ $DO_BUILD -eq 1 ]]; then
  echo "[build] rm -rf build && ./build.sh Release"
  rm -rf build
  ./build.sh Release
fi

if [[ ! -x "$ENGINE" ]]; then
  echo "ERROR: Engine binary not found or not executable at: $ENGINE" >&2
  exit 1
fi

run_eval() {
  local fen="$1"
  local tmp
  tmp=$(mktemp)
  # Drive UCI: set position, request eval breakdown, quit
  {
    echo "uci"
    echo "isready"
    echo "position fen ${fen}"
    echo "eval"
    echo "quit"
  } | "$ENGINE" >"$tmp" 2>&1 || true

  echo "---"
  echo "FEN: ${fen}"
  # Extract the breakdown block and summarize key lines
  awk '
    BEGIN{show=0}
    /Evaluation Breakdown/ {show=1}
    show==1 {print}
    /\+---\+---\+---\+---\+---\+---\+---\+---\+/ && NR>1 && prev=="footer" {exit}
    {prev = (match($0,/\+---\+---\+---\+---\+---\+---\+---\+---\+/)?"footer":"")}
  ' "$tmp" | sed '/^$/d'

  # Also show bestmove if printed (not expected in eval path, but harmless)
  grep -E "^bestmove|^info score|^Total:|^Side to move:" -n "$tmp" || true

  rm -f "$tmp"
}

echo "== Eval Compare: Example Game 2 Focus =="
run_eval "$FEN1"

echo
echo "== Eval Compare: Example Game 1 Endgame Choice =="
run_eval "$FEN2"

echo
echo "== Eval Compare: King Safety Sanity (expect KS ~ +48 for White) =="
run_eval "$FEN3"

echo
echo "Done. Use --no-build to skip rebuild; customize with --engine/--fen1/--fen2/--fen3."
