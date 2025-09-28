# Eval Framework Telemetry Notes

## Structured EvalExtended Output
- Enable via `setoption name EvalExtended value true` in UCI.
- Output now uses `info eval` lines with key/value pairs for machine parsing.
- Includes: summary (side, total white CP, final CP), phase blend, pawn hash metadata, per-term scores with MG/EG splits for material/PST, and mobility move counts.

Example:
```
info eval header version=1
info eval summary side=white total_white_cp=42 final_cp=42
info eval phase value=172 mg_pct=67 eg_pct=33
info eval pawn_cache key=0x123456789abcdef0 hit=1
info eval term name=material cp=35 mg=120 eg=-49
info eval term name=pst cp=18 mg=22 eg=10
...
info eval total white_cp=42 final_cp=42
```

## Eval Log File Toggle
- Configure optional persistent logging with `setoption name EvalLogFile value <path>`.
- When set, every EvalExtended dump appends the structured lines to the file.
- Clearing: `setoption name EvalLogFile value`.
- Failed opens are reported once per session; fix path/permissions then re-run the command.

## Recommended Workflow (Phase P1)
1. `setoption name EvalExtended value true`
2. `setoption name EvalLogFile value tmp_logs/eval_debug.log`
3. Issue `debug eval` (or run search with EvalExtended enabled) on target FENs.
4. Parse `info eval` lines from stdout or the log file to feed the external harness.

Store captured logs per experiment under this directory (e.g., `logs/2025-09-19-pawn-pack.txt`).
