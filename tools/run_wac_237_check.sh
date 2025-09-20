#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

ENGINE_PATH="${ENGINE_PATH:-${ROOT_DIR}/bin/seajay}"
EPD_PATH="${EPD_PATH:-${ROOT_DIR}/tests/positions/wac_failures_20250918.epd}"
HARNESS_TIMEOUT="${HARNESS_TIMEOUT:-300}"
INVOCATION_TIMEOUT_MS="${INVOCATION_TIMEOUT_MS:-120000}"

LOG_DIR="${ROOT_DIR}/docs/project_docs/telemetry/tactical"
mkdir -p "${LOG_DIR}"

timestamp="$(date +"%Y%m%d_%H%M%S")"
csv_out="${LOG_DIR}/WAC.237_mt_run_${timestamp}.csv"
txt_out="${LOG_DIR}/WAC.237_mt_run_${timestamp}.log"

cmd=(
  python3 "${ROOT_DIR}/tools/tactical_investigation.py"
  --engine "${ENGINE_PATH}"
  --epd "${EPD_PATH}"
  --ids WAC.237
  --time-ms 100 850 2000
  --timeout-ms "${INVOCATION_TIMEOUT_MS}"
  --output "${csv_out}"
  --verbose
)

echo "[run_wac_237_check] Starting tactical harness (log: ${txt_out})"
(
  cd "${ROOT_DIR}"
  timeout --preserve-status "${HARNESS_TIMEOUT}" "${cmd[@]}"
) | tee "${txt_out}"

echo "[run_wac_237_check] Completed. CSV saved to ${csv_out}"
