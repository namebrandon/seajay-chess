# SeaJay UCI Options Documentation

## Overview
This document lists all UCI options available in SeaJay, their purpose, and SPSA tuning recommendations for OpenBench.

---

## Core Engine Options

### Hash
**Type:** spin  
**Default:** 16  
**Range:** 1-16384  
**Purpose:** Sets the size of the transposition table in MB. Larger tables store more positions, improving search efficiency.  
**SPSA:** Not tunable - hardware dependent  

### Threads
**Type:** spin  
**Default:** 1  
**Range:** 1-1024  
**Purpose:** Number of search threads (currently single-threaded implementation).  
**SPSA:** Not tunable - hardware dependent  

### UseMagicBitboards
**Type:** check  
**Default:** true  
**Purpose:** Enables magic bitboard move generation (79x speedup). Should always be true.  
**SPSA:** Not tunable - critical for performance  

### UseTranspositionTable
**Type:** check  
**Default:** true  
**Purpose:** Enables transposition table for position caching.  
**SPSA:** Not tunable - critical for performance  

---

## Quiescence Search Options

### UseQuiescence
**Type:** check  
**Default:** true  
**Purpose:** Enables quiescence search to resolve tactical sequences.  
**SPSA:** Not tunable - provides 400-600 ELO  

### QSearchNodeLimit
**Type:** spin  
**Default:** 0  
**Range:** 0-10000000  
**Purpose:** Limits nodes per quiescence search entry (0 = unlimited).  
**SPSA:** Not recommended - use for debugging only  

### MaxCheckPly ⭐ (High Impact)
**Type:** spin  
**Default:** 6  
**Range:** 0-10  
**Purpose:** Maximum check extension depth in quiescence search. Higher values catch deeper tactics but cost more nodes.  
**SPSA Input:** `MaxCheckPly, int, 6.0, 0.0, 10.0, 1.0, 0.002`  
**Expected Impact:** Very high (current implementation generates all moves in check)  

---

## Search Options

### UseAspirationWindows
**Type:** check  
**Default:** true  
**Purpose:** Uses aspiration windows to narrow alpha-beta bounds.  
**SPSA:** Not recommended - generally beneficial  

### AspirationWindow ⭐
**Type:** spin  
**Default:** 16  
**Range:** 5-50  
**Purpose:** Initial aspiration window size in centipawns.  
**SPSA Input:** `AspirationWindow, int, 16.0, 5.0, 50.0, 3.0, 0.002`  

### AspirationMaxAttempts
**Type:** spin  
**Default:** 5  
**Range:** 3-10  
**Purpose:** Maximum re-search attempts before infinite window.  
**SPSA Input:** `AspirationMaxAttempts, int, 5.0, 3.0, 10.0, 1.0, 0.002`  

### AspirationGrowth
**Type:** combo  
**Default:** exponential  
**Values:** linear, moderate, exponential, adaptive  
**Purpose:** Window growth strategy on fail high/low.  
**SPSA:** Not tunable - combo option  

### ShowPVSStats
**Type:** check  
**Default:** false  
**Purpose:** Shows Principal Variation Search statistics.  
**SPSA:** Not tunable - debug option  

### SearchStats
**Type:** check  
**Default:** false  
**Purpose:** Prints a one‑shot summary at the end of `go`, including TT probes/hits/hit%, TT cutoffs/stores/collisions, PVS scout/re‑search rate, null‑move attempts/cutoffs/cut%, pruning counts (futility/move‑count), and lazy‑legality rejections (before first legal/total). Visibility only; does not affect search.  
**SPSA:** Not tunable - debug/visibility option  

---

## Late Move Reductions (LMR)

### LMREnabled
**Type:** check  
**Default:** true  
**Purpose:** Enables Late Move Reductions to prune late moves.  
**SPSA:** Not tunable - generally beneficial  

### LMRMinDepth ⭐
**Type:** spin  
**Default:** 3  
**Range:** 0-10  
**Purpose:** Minimum depth to apply LMR.  
**SPSA Input:** `LMRMinDepth, int, 3.0, 1.0, 6.0, 1.0, 0.002`  

### LMRMinMoveNumber ⭐
**Type:** spin  
**Default:** 6  
**Range:** 0-20  
**Purpose:** Start reducing after this many moves.  
**SPSA Input:** `LMRMinMoveNumber, int, 6.0, 3.0, 12.0, 1.0, 0.002`  

### LMRBaseReduction
**Type:** spin  
**Default:** 1  
**Range:** 0-3  
**Purpose:** Base reduction amount in plies.  
**SPSA Input:** `LMRBaseReduction, int, 1.0, 0.0, 3.0, 0.5, 0.002`  

### LMRDepthFactor
**Type:** spin  
**Default:** 3  
**Range:** 1-10  
**Purpose:** Depth scaling factor for reduction formula.  
**SPSA Input:** `LMRDepthFactor, int, 3.0, 1.0, 10.0, 1.0, 0.002`  

---

## Null Move Pruning

### UseNullMove
**Type:** check  
**Default:** true  
**Purpose:** Enables null move pruning.  
**SPSA:** Not tunable - generally beneficial  

### NullMoveStaticMargin ⭐
**Type:** spin  
**Default:** 120  
**Range:** 50-300  
**Purpose:** Static evaluation margin for null move pruning.  
**SPSA Input:** `NullMoveStaticMargin, int, 120.0, 50.0, 300.0, 15.0, 0.002`  

---

## Evaluation Options

### UsePSTInterpolation
**Type:** check  
**Default:** true  
**Purpose:** Enables phase-based interpolation between middlegame and endgame PST values. Provides smooth evaluation transitions as material comes off the board.  
**SPSA:** Not tunable - generally beneficial (provides 5-15 ELO)  
**Note:** Phase calculation uses material weights: Knight=1, Bishop=1, Rook=2, Queen=4  

### ShowPhaseInfo
**Type:** check  
**Default:** true  
**Purpose:** Prints phase details during `uci -> eval`: continuous phase (0–256) and coarse GamePhase (OPENING/MIDDLEGAME/ENDGAME). Visibility only; does not affect evaluation.  
**SPSA:** Not tunable - debug/visibility option  

---

## Move Ordering

### CountermoveBonus ⭐
**Type:** spin  
**Default:** 8000  
**Range:** 0-20000  
**Purpose:** History bonus for countermoves that cause cutoffs.  
**SPSA Input:** `CountermoveBonus, int, 8000.0, 4000.0, 16000.0, 500.0, 0.002`  

### CounterMoveHistoryWeight
**Type:** float  
**Default:** 0.0  
**Range:** 0.0-2.0  
**Purpose:** Weight for counter-move history in move ordering. Currently disabled (0.0) as testing showed no benefit at fast time controls.  
**SPSA:** Not recommended at current strength level  
**Note:** May provide benefit at longer time controls (60+0) or when engine reaches 2400+ ELO  

---

## Move Count Pruning

### MoveCountPruning
**Type:** check  
**Default:** true  
**Purpose:** Enables move count pruning (futility at high depths).  
**SPSA:** Not tunable - generally beneficial  

### MoveCountLimit3-8 ⭐ (Tune as Group)
**Type:** spin  
**Purpose:** Move limit for each depth (3-8).  
**SPSA Batch Input:**
```
MoveCountLimit3, int, 12.0, 3.0, 30.0, 2.0, 0.002
MoveCountLimit4, int, 18.0, 5.0, 40.0, 2.0, 0.002
MoveCountLimit5, int, 24.0, 8.0, 50.0, 3.0, 0.002
MoveCountLimit6, int, 30.0, 10.0, 60.0, 3.0, 0.002
MoveCountLimit7, int, 36.0, 12.0, 70.0, 4.0, 0.002
MoveCountLimit8, int, 42.0, 15.0, 80.0, 4.0, 0.002
```

### MoveCountHistoryThreshold ⭐
**Type:** spin  
**Default:** 1500  
**Range:** 0-5000  
**Purpose:** History score threshold for bonus moves.  
**SPSA Input:** `MoveCountHistoryThreshold, int, 1500.0, 500.0, 3000.0, 100.0, 0.002`  

### MoveCountHistoryBonus
**Type:** spin  
**Default:** 6  
**Range:** 0-20  
**Purpose:** Extra moves allowed for good history.  
**SPSA Input:** `MoveCountHistoryBonus, int, 6.0, 2.0, 12.0, 1.0, 0.002`  

### MoveCountImprovingRatio
**Type:** spin  
**Default:** 75  
**Range:** 50-100  
**Purpose:** Percentage of moves to search when not improving (75 = 75%).  
**SPSA Input:** `MoveCountImprovingRatio, int, 75.0, 50.0, 100.0, 5.0, 0.002`  

---

## Game Phase Stability

### UsePhaseStability
**Type:** check  
**Default:** true  
**Purpose:** Adjusts stability requirements based on game phase.  
**SPSA:** Not tunable - generally beneficial  

### StabilityThreshold
**Type:** spin  
**Default:** 6  
**Range:** 3-12  
**Purpose:** Default iterations needed for move stability.  
**SPSA Input:** `StabilityThreshold, int, 6.0, 3.0, 12.0, 1.0, 0.002`  

### OpeningStability
**Type:** spin  
**Default:** 4  
**Range:** 2-8  
**Purpose:** Stability threshold in opening phase.  
**SPSA Input:** `OpeningStability, int, 4.0, 2.0, 8.0, 1.0, 0.002`  

### MiddlegameStability
**Type:** spin  
**Default:** 6  
**Range:** 3-10  
**Purpose:** Stability threshold in middlegame.  
**SPSA Input:** `MiddlegameStability, int, 6.0, 3.0, 10.0, 1.0, 0.002`  

### EndgameStability
**Type:** spin  
**Default:** 8  
**Range:** 4-12  
**Purpose:** Stability threshold in endgame.  
**SPSA Input:** `EndgameStability, int, 8.0, 4.0, 12.0, 1.0, 0.002`  

---

## Static Exchange Evaluation (SEE)

### SEEMode
**Type:** combo  
**Default:** off  
**Values:** off, testing, shadow, production  
**Purpose:** SEE integration mode for move ordering.  
**SPSA:** Not tunable - combo option  
**Note:** SPRT testing shows -7 ELO with production mode. SEE calculation overhead exceeds ordering benefit at current implementation.

### SEEPruning
**Type:** combo  
**Default:** conservative  
**Values:** off, conservative, aggressive  
**Purpose:** SEE-based pruning aggressiveness in quiescence.  
**SPSA:** Not tunable - combo option  
**Note:** SPRT testing shows +28.6 ELO for conservative vs off. Aggressive mode shows no additional benefit  

---

## SPSA Tuning Priority Recommendations

### High Priority (Large Impact Expected)
1. **MaxCheckPly** - Controls tactical depth (400-600 ELO impact)
2. **LMRMinMoveNumber** - When to start reductions
3. **NullMoveStaticMargin** - Null move pruning threshold
4. **CountermoveBonus** - Move ordering quality

### Medium Priority (Moderate Impact)
5. **MoveCountLimit3-8** - Tune as a group for consistency
6. **AspirationWindow** - Search efficiency
7. **LMRMinDepth** - LMR application depth
8. **MoveCountHistoryThreshold** - History heuristic impact

### Low Priority (Fine Tuning)
9. **StabilityThreshold** - Time management
10. **Phase-specific stability** - Opening/Middle/Endgame
11. **LMRBaseReduction/DepthFactor** - LMR formula tuning
12. **MoveCountHistoryBonus/ImprovingRatio** - Move count details

---

## Example SPSA Workload for OpenBench

### Single Parameter Test (MaxCheckPly)
```
MaxCheckPly, int, 6.0, 0.0, 10.0, 1.0, 0.002
```
Games: 30000  
Time Control: 10+0.1  

### Multi-Parameter Test (LMR Suite)
```
LMRMinDepth, int, 3.0, 1.0, 6.0, 1.0, 0.002
LMRMinMoveNumber, int, 6.0, 3.0, 12.0, 1.0, 0.002
LMRBaseReduction, int, 1.0, 0.0, 3.0, 0.5, 0.002
LMRDepthFactor, int, 3.0, 1.0, 10.0, 1.0, 0.002
```
Games: 50000  
Time Control: 10+0.1  

### Large Suite Test (Move Count Pruning)
```
MoveCountLimit3, int, 12.0, 3.0, 30.0, 2.0, 0.002
MoveCountLimit4, int, 18.0, 5.0, 40.0, 2.0, 0.002
MoveCountLimit5, int, 24.0, 8.0, 50.0, 3.0, 0.002
MoveCountLimit6, int, 30.0, 10.0, 60.0, 3.0, 0.002
MoveCountLimit7, int, 36.0, 12.0, 70.0, 4.0, 0.002
MoveCountLimit8, int, 42.0, 15.0, 80.0, 4.0, 0.002
MoveCountHistoryThreshold, int, 1500.0, 500.0, 3000.0, 100.0, 0.002
MoveCountHistoryBonus, int, 6.0, 2.0, 12.0, 1.0, 0.002
```
Games: 80000  
Time Control: 10+0.1  

---

## Notes on SPSA Parameters

### C_end (Step Size)
- **Integers with small range (0-10):** Use 0.5-1.0
- **Integers with medium range (0-100):** Use 2.0-5.0  
- **Large integers (100+):** Use 10.0-50.0

### R_end (Learning Rate)
- **Conservative:** 0.002 (recommended default)
- **Moderate:** 0.01
- **Aggressive:** 0.02 (faster but less stable)

### Games Required
- **Single parameter:** 20000-30000
- **2-4 parameters:** 40000-50000
- **5+ parameters:** 60000-100000

⭐ = Recommended for SPSA tuning