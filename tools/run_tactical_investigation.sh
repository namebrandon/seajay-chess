#!/bin/bash
# Wrapper for tactical investigation harness.
# Usage: run_tactical_investigation.sh [engine] [time_ms...]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ENGINE="${1:-$WORKSPACE_DIR/bin/seajay}"
shift || true

if [ "$#" -eq 0 ]; then
  TIME_ARGS=(850)
else
  TIME_ARGS=("$@")
fi

python3 "$SCRIPT_DIR/tactical_investigation.py" \
  --engine "$ENGINE" \
  --time-ms "${TIME_ARGS[@]}" \
  --verbose
