#!/usr/bin/env bash
set -euo pipefail

ENGINE=${ENGINE:-./bin/seajay}
PACK=${PACK:-tests/packs/eval_pawn_focus.epd}
REF_ENGINE=${REF_ENGINE:-}
MOVETIME=${MOVETIME:-100}
DEPTH=${DEPTH:-}
THREADS=${THREADS:-1}
OUTDIR=${OUTDIR:-tmp/eval_reports}
TIMESTAMP=$(date -u +"%Y%m%dT%H%M%SZ")
REPORT=${REPORT:-$OUTDIR/eval_${TIMESTAMP}.json}
SUMMARY=${SUMMARY:-$OUTDIR/eval_${TIMESTAMP}_summary.json}

mkdir -p "$OUTDIR"

CMD=(python3 tools/eval_harness/compare_eval.py
    --engine "$ENGINE"
    --pack "$PACK"
    --movetime "$MOVETIME"
    --threads "$THREADS"
    --out "$REPORT"
    --summary-json "$SUMMARY"
    --summary-top 10)

if [[ -n "$DEPTH" ]]; then
    CMD+=(--depth "$DEPTH")
fi

if [[ -n "$REF_ENGINE" ]]; then
    CMD+=(--ref-engine "$REF_ENGINE")
fi

if [[ -n "${ENGINE_OPTIONS:-}" ]]; then
    for opt in $ENGINE_OPTIONS; do
        CMD+=(--engine-option "$opt")
    done
fi

exec "${CMD[@]}"
