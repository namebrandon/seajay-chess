#!/usr/bin/env bash
set -euo pipefail

# King Safety sanity check harness
# Shows King Safety contribution before/after common luft moves
#
# Usage: tools/ks_sanity.sh [--no-build] [--engine ./bin/seajay]

ENGINE="./bin/seajay"
DO_BUILD=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-build) DO_BUILD=0; shift ;;
    --engine) ENGINE="$2"; shift 2 ;;
    -h|--help) echo "Usage: $0 [--no-build] [--engine ./bin/seajay]"; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; exit 2 ;;
  esac
done

if [[ $DO_BUILD -eq 1 ]]; then
  rm -rf build
  ./build.sh Release >/dev/null
fi

if [[ ! -x "$ENGINE" ]]; then
  echo "ERROR: Engine not found: $ENGINE" >&2
  exit 1
fi

run_case() {
  local name="$1"; shift
  local before_moves="$1"; shift
  local after_move="$1"; shift

  echo "== $name =="
  # Before luft
  {
    echo uci
    echo "setoption name ShowPhaseInfo value true"
    echo isready
    echo ucinewgame
    echo position startpos moves ${before_moves}
    echo eval
    echo quit
  } | "$ENGINE" 2>/dev/null | awk '/Evaluation Breakdown/{show=1} show && /King Safety:/{print "Before:",$3} /\+---\+---\+---\+---/{exit}'

  # After luft move
  {
    echo uci
    echo "setoption name ShowPhaseInfo value true"
    echo isready
    echo ucinewgame
    echo position startpos moves ${before_moves} ${after_move}
    echo eval
    echo quit
  } | "$ENGINE" 2>/dev/null | awk '/Evaluation Breakdown/{show=1} show && /King Safety:/{print "After: ",$3} /\+---\+---\+---\+---/{exit}'
  echo
}

# Common luft patterns
run_case "White h2-h3 (kingside luft)" "e2e4 g1f3" "h2h3"
run_case "White g2-g3 (kingside luft)" "e2e4 g1f3" "g2g3"
run_case "White f2-f3 (kingside luft alt)" "e2e4 g1f3" "f2f3"
run_case "White a2-a3 (queenside luft)" "d2d4 c1b2" "a2a3"

run_case "Black h7-h6 (kingside luft)" "e2e4 e7e5 g1f3" "h7h6"
run_case "Black g7-g6 (kingside luft)" "e2e4 e7e5 g1f3" "g7g6"
run_case "Black f7-f6 (kingside luft alt)" "e2e4 e7e5 g1f3" "f7f6"
run_case "Black a7-a6 (queenside luft)" "d2d4 d7d5 c1f4" "a7a6"

echo "Done."
