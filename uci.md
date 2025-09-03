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

### LMRHistoryThreshold
**Type:** spin  
**Default:** 50  
**Range:** 10-90  
**Purpose:** History score threshold (percentage) for LMR reduction.  
**SPSA Input:** `LMRHistoryThreshold, int, 50.0, 10.0, 90.0, 5.0, 0.002`  

### LMRPvReduction
**Type:** spin  
**Default:** 1  
**Range:** 0-2  
**Purpose:** Reduction amount for PV nodes in LMR.  
**SPSA Input:** `LMRPvReduction, int, 1.0, 0.0, 2.0, 0.5, 0.002`  

### LMRNonImprovingBonus
**Type:** spin  
**Default:** 1  
**Range:** 0-3  
**Purpose:** Extra reduction when position is not improving.  
**SPSA Input:** `LMRNonImprovingBonus, int, 1.0, 0.0, 3.0, 0.5, 0.002`  

---

## Null Move Pruning

### UseNullMove
**Type:** check  
**Default:** true  
**Purpose:** Enables null move pruning.  
**SPSA:** Not tunable - generally beneficial  

### NullMoveStaticMargin ⭐
**Type:** spin  
**Default:** 90  
**Range:** 50-300  
**Purpose:** Static evaluation margin for null move pruning.  
**SPSA Input:** `NullMoveStaticMargin, int, 90.0, 50.0, 300.0, 15.0, 0.002`  

### NullMoveMinDepth
**Type:** spin  
**Default:** 3  
**Range:** 2-5  
**Purpose:** Minimum depth to apply null move pruning.  
**SPSA Input:** `NullMoveMinDepth, int, 3.0, 2.0, 5.0, 1.0, 0.002`  

### NullMoveReductionBase ⭐
**Type:** spin  
**Default:** 2  
**Range:** 1-4  
**Purpose:** Base reduction amount for null move.  
**SPSA Input:** `NullMoveReductionBase, int, 2.0, 1.0, 4.0, 0.5, 0.002`  

### NullMoveReductionDepth6
**Type:** spin  
**Default:** 3  
**Range:** 2-5  
**Purpose:** Reduction at depth 6+ for null move.  
**SPSA Input:** `NullMoveReductionDepth6, int, 3.0, 2.0, 5.0, 0.5, 0.002`  

### NullMoveReductionDepth12
**Type:** spin  
**Default:** 4  
**Range:** 3-6  
**Purpose:** Reduction at depth 12+ for null move.  
**SPSA Input:** `NullMoveReductionDepth12, int, 4.0, 3.0, 6.0, 0.5, 0.002`  

### NullMoveVerifyDepth
**Type:** spin  
**Default:** 10  
**Range:** 6-14  
**Purpose:** Depth at which to verify null move cutoffs.  
**SPSA Input:** `NullMoveVerifyDepth, int, 10.0, 6.0, 14.0, 1.0, 0.002`  

### NullMoveEvalMargin
**Type:** spin  
**Default:** 200  
**Range:** 100-400  
**Purpose:** Evaluation margin required for null move attempt.  
**SPSA Input:** `NullMoveEvalMargin, int, 200.0, 100.0, 400.0, 20.0, 0.002`  

---

## Razoring

### UseRazoring
**Type:** check  
**Default:** true  
**Purpose:** Enables razoring (early pruning at low depths).  
**SPSA:** Not tunable - generally beneficial  

### RazorMargin1 ⭐
**Type:** spin  
**Default:** 300  
**Range:** 100-800  
**Purpose:** Razoring margin for depth 1.  
**SPSA Input:** `RazorMargin1, int, 300.0, 100.0, 800.0, 30.0, 0.002`  

### RazorMargin2 ⭐
**Type:** spin  
**Default:** 500  
**Range:** 200-1200  
**Purpose:** Razoring margin for depth 2.  
**SPSA Input:** `RazorMargin2, int, 500.0, 200.0, 1200.0, 50.0, 0.002`  

---

## Futility Pruning (Enhanced)

### UseFutilityPruning
**Type:** check  
**Default:** true  
**Purpose:** Enables futility pruning at frontier nodes.  
**SPSA:** Not tunable - generally beneficial  

### FutilityPruning
**Type:** check  
**Default:** true  
**Purpose:** Alternative enable flag for futility pruning.  
**SPSA:** Not tunable - generally beneficial  

### FutilityMaxDepth
**Type:** spin  
**Default:** 7  
**Range:** 0-10  
**Purpose:** Maximum depth to apply futility pruning.  
**SPSA Input:** `FutilityMaxDepth, int, 7.0, 4.0, 10.0, 1.0, 0.002`  

### FutilityBase ⭐
**Type:** spin  
**Default:** 150  
**Range:** 50-500  
**Purpose:** Base futility margin.  
**SPSA Input:** `FutilityBase, int, 150.0, 50.0, 500.0, 20.0, 0.002`  

### FutilityScale ⭐
**Type:** spin  
**Default:** 60  
**Range:** 20-200  
**Purpose:** Futility margin scaling factor per depth.  
**SPSA Input:** `FutilityScale, int, 60.0, 20.0, 200.0, 10.0, 0.002`  

### FutilityMargin1
**Type:** spin  
**Default:** 100  
**Range:** 50-200  
**Purpose:** Futility margin for depth 1.  
**SPSA Input:** `FutilityMargin1, int, 100.0, 50.0, 200.0, 10.0, 0.002`  

### FutilityMargin2
**Type:** spin  
**Default:** 175  
**Range:** 100-300  
**Purpose:** Futility margin for depth 2.  
**SPSA Input:** `FutilityMargin2, int, 175.0, 100.0, 300.0, 15.0, 0.002`  

### FutilityMargin3
**Type:** spin  
**Default:** 250  
**Range:** 150-400  
**Purpose:** Futility margin for depth 3.  
**SPSA Input:** `FutilityMargin3, int, 250.0, 150.0, 400.0, 20.0, 0.002`  

### FutilityMargin4
**Type:** spin  
**Default:** 325  
**Range:** 200-500  
**Purpose:** Futility margin for depth 4.  
**SPSA Input:** `FutilityMargin4, int, 325.0, 200.0, 500.0, 25.0, 0.002`  

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

### EvalExtended
**Type:** check  
**Default:** false  
**Purpose:** Shows detailed evaluation breakdown by component (material, PST, mobility, etc.).  
**SPSA:** Not tunable - debug/visibility option  

---

## Endgame PST Tuning ⭐⭐⭐

These parameters fine-tune piece-square table values in the endgame. They're prime candidates for SPSA tuning.

### Pawn Endgame PST

#### pawn_eg_r3_d
**Type:** spin  
**Default:** 8  
**Range:** 0-30  
**Purpose:** Pawn endgame value on rank 3, d-file.  
**SPSA Input:** `pawn_eg_r3_d, int, 8.0, 0.0, 30.0, 2.0, 0.002`  

#### pawn_eg_r3_e
**Type:** spin  
**Default:** 7  
**Range:** 0-30  
**Purpose:** Pawn endgame value on rank 3, e-file.  
**SPSA Input:** `pawn_eg_r3_e, int, 7.0, 0.0, 30.0, 2.0, 0.002`  

#### pawn_eg_r4_d
**Type:** spin  
**Default:** 18  
**Range:** 10-50  
**Purpose:** Pawn endgame value on rank 4, d-file.  
**SPSA Input:** `pawn_eg_r4_d, int, 18.0, 10.0, 50.0, 3.0, 0.002`  

#### pawn_eg_r4_e
**Type:** spin  
**Default:** 16  
**Range:** 10-50  
**Purpose:** Pawn endgame value on rank 4, e-file.  
**SPSA Input:** `pawn_eg_r4_e, int, 16.0, 10.0, 50.0, 3.0, 0.002`  

#### pawn_eg_r5_d
**Type:** spin  
**Default:** 29  
**Range:** 20-70  
**Purpose:** Pawn endgame value on rank 5, d-file.  
**SPSA Input:** `pawn_eg_r5_d, int, 29.0, 20.0, 70.0, 4.0, 0.002`  

#### pawn_eg_r5_e
**Type:** spin  
**Default:** 27  
**Range:** 20-70  
**Purpose:** Pawn endgame value on rank 5, e-file.  
**SPSA Input:** `pawn_eg_r5_e, int, 27.0, 20.0, 70.0, 4.0, 0.002`  

#### pawn_eg_r6_d
**Type:** spin  
**Default:** 51  
**Range:** 30-100  
**Purpose:** Pawn endgame value on rank 6, d-file.  
**SPSA Input:** `pawn_eg_r6_d, int, 51.0, 30.0, 100.0, 5.0, 0.002`  

#### pawn_eg_r6_e
**Type:** spin  
**Default:** 48  
**Range:** 30-100  
**Purpose:** Pawn endgame value on rank 6, e-file.  
**SPSA Input:** `pawn_eg_r6_e, int, 48.0, 30.0, 100.0, 5.0, 0.002`  

#### pawn_eg_r7_center
**Type:** spin  
**Default:** 75  
**Range:** 50-150  
**Purpose:** Pawn endgame value on rank 7, center files.  
**SPSA Input:** `pawn_eg_r7_center, int, 75.0, 50.0, 150.0, 8.0, 0.002`  

### Knight Endgame PST

#### knight_eg_center ⭐
**Type:** spin  
**Default:** 15  
**Range:** 5-25  
**Purpose:** Knight bonus in center squares.  
**SPSA Input:** `knight_eg_center, int, 15.0, 5.0, 25.0, 2.0, 0.002`  

#### knight_eg_extended
**Type:** spin  
**Default:** 10  
**Range:** 0-20  
**Purpose:** Knight bonus in extended center.  
**SPSA Input:** `knight_eg_extended, int, 10.0, 0.0, 20.0, 2.0, 0.002`  

#### knight_eg_edge
**Type:** spin  
**Default:** -25  
**Range:** -40 to -10  
**Purpose:** Knight penalty on edge squares.  
**SPSA Input:** `knight_eg_edge, int, -25.0, -40.0, -10.0, 3.0, 0.002`  

#### knight_eg_corner
**Type:** spin  
**Default:** -40  
**Range:** -50 to -20  
**Purpose:** Knight penalty in corner squares.  
**SPSA Input:** `knight_eg_corner, int, -40.0, -50.0, -20.0, 3.0, 0.002`  

### Bishop Endgame PST

#### bishop_eg_long_diag ⭐
**Type:** spin  
**Default:** 19  
**Range:** 10-35  
**Purpose:** Bishop bonus on long diagonals.  
**SPSA Input:** `bishop_eg_long_diag, int, 19.0, 10.0, 35.0, 2.0, 0.002`  

#### bishop_eg_center
**Type:** spin  
**Default:** 14  
**Range:** 5-25  
**Purpose:** Bishop bonus in center.  
**SPSA Input:** `bishop_eg_center, int, 14.0, 5.0, 25.0, 2.0, 0.002`  

#### bishop_eg_edge
**Type:** spin  
**Default:** -5  
**Range:** -15 to 5  
**Purpose:** Bishop penalty on edges.  
**SPSA Input:** `bishop_eg_edge, int, -5.0, -15.0, 5.0, 2.0, 0.002`  

### Rook Endgame PST

#### rook_eg_7th ⭐
**Type:** spin  
**Default:** 20  
**Range:** 15-40  
**Purpose:** Rook bonus on 7th rank.  
**SPSA Input:** `rook_eg_7th, int, 20.0, 15.0, 40.0, 2.0, 0.002`  

#### rook_eg_active
**Type:** spin  
**Default:** 12  
**Range:** 5-20  
**Purpose:** Active rook bonus.  
**SPSA Input:** `rook_eg_active, int, 12.0, 5.0, 20.0, 2.0, 0.002`  

#### rook_eg_passive
**Type:** spin  
**Default:** 5  
**Range:** 0-15  
**Purpose:** Passive rook value.  
**SPSA Input:** `rook_eg_passive, int, 5.0, 0.0, 15.0, 1.0, 0.002`  

### Queen Endgame PST

#### queen_eg_center
**Type:** spin  
**Default:** 9  
**Range:** 5-20  
**Purpose:** Queen bonus in center.  
**SPSA Input:** `queen_eg_center, int, 9.0, 5.0, 20.0, 1.0, 0.002`  

#### queen_eg_active
**Type:** spin  
**Default:** 7  
**Range:** 0-20  
**Purpose:** Active queen bonus.  
**SPSA Input:** `queen_eg_active, int, 7.0, 0.0, 20.0, 1.0, 0.002`  

#### queen_eg_back
**Type:** spin  
**Default:** -5  
**Range:** -10 to 5  
**Purpose:** Queen penalty on back rank.  
**SPSA Input:** `queen_eg_back, int, -5.0, -10.0, 5.0, 1.0, 0.002`  

---

## King Safety Tuning ⭐⭐

### KingSafetyDirectShieldMg ⭐
**Type:** spin  
**Default:** 16  
**Range:** 0-50  
**Purpose:** Bonus for pawns directly shielding the king in middlegame.  
**SPSA Input:** `KingSafetyDirectShieldMg, int, 16.0, 0.0, 50.0, 3.0, 0.002`  
**Note:** Uses proper SPSA float rounding (16.6 → 17)

### KingSafetyAdvancedShieldMg ⭐
**Type:** spin  
**Default:** 12  
**Range:** 0-40  
**Purpose:** Bonus for advanced shield pawns (2 ranks ahead) in middlegame.  
**SPSA Input:** `KingSafetyAdvancedShieldMg, int, 12.0, 0.0, 40.0, 2.0, 0.002`  
**Note:** Uses proper SPSA float rounding

### KingSafetyEnableScoring
**Type:** spin  
**Default:** 1  
**Range:** 0-1  
**Purpose:** Enable (1) or disable (0) king safety scoring entirely.  
**SPSA:** Not recommended for tuning - use for testing only  

---

## King PST Middlegame (Castling Incentives) ⭐⭐⭐

These parameters control the middlegame piece-square table values for the king on rank 1, directly affecting castling incentives. Default values create a 50-60 centipawn incentive to castle.

### king_mg_e1 ⭐
**Type:** spin  
**Default:** -30  
**Range:** -50 to 50  
**Purpose:** King on starting square (e1). Negative value encourages moving.  
**SPSA Input:** `king_mg_e1, int, -30.0, -50.0, 50.0, 5.0, 0.002`  

### king_mg_b1 ⭐⭐
**Type:** spin  
**Default:** 30  
**Range:** -50 to 50  
**Purpose:** Queenside castled position. High value encourages O-O-O.  
**SPSA Input:** `king_mg_b1, int, 30.0, -50.0, 50.0, 5.0, 0.002`  

### king_mg_g1 ⭐⭐
**Type:** spin  
**Default:** 20  
**Range:** -50 to 50  
**Purpose:** Kingside castled position. Positive value encourages O-O.  
**SPSA Input:** `king_mg_g1, int, 20.0, -50.0, 50.0, 5.0, 0.002`  

### king_mg_a1
**Type:** spin  
**Default:** 20  
**Range:** -50 to 50  
**Purpose:** Corner square (a1).  
**SPSA Input:** `king_mg_a1, int, 20.0, -50.0, 50.0, 5.0, 0.002`  

### king_mg_h1
**Type:** spin  
**Default:** 20  
**Range:** -50 to 50  
**Purpose:** Corner square (h1).  
**SPSA Input:** `king_mg_h1, int, 20.0, -50.0, 50.0, 5.0, 0.002`  

### king_mg_c1
**Type:** spin  
**Default:** 10  
**Range:** -50 to 50  
**Purpose:** Near queenside castle (c1).  
**SPSA Input:** `king_mg_c1, int, 10.0, -50.0, 50.0, 5.0, 0.002`  

### king_mg_d1
**Type:** spin  
**Default:** -20  
**Range:** -50 to 50  
**Purpose:** Center-ish square (d1). Negative to discourage.  
**SPSA Input:** `king_mg_d1, int, -20.0, -50.0, 50.0, 5.0, 0.002`  

### king_mg_f1
**Type:** spin  
**Default:** -30  
**Range:** -50 to 50  
**Purpose:** Center-ish square (f1). Negative to discourage.  
**SPSA Input:** `king_mg_f1, int, -30.0, -50.0, 50.0, 5.0, 0.002`  

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

### Highest Priority (Core Search Parameters)
1. **MaxCheckPly** - Controls tactical depth (400-600 ELO impact)
2. **FutilityBase & FutilityScale** - Core pruning margins
3. **RazorMargin1 & RazorMargin2** - Early pruning thresholds
4. **NullMoveReductionBase** - Null move core reduction

### High Priority (Search Efficiency)
5. **LMRMinMoveNumber & LMRMinDepth** - LMR activation points
6. **NullMoveStaticMargin** - Null move pruning threshold
7. **FutilityMargin1-4** - Depth-specific pruning (tune as group)
8. **CountermoveBonus** - Move ordering quality

### Medium Priority (Endgame PST Tuning)
9. **Pawn endgame PST values** - Critical endgame evaluation
10. **Knight centralization values** - knight_eg_center, edge, corner
11. **Bishop diagonals** - bishop_eg_long_diag
12. **Rook 7th rank** - rook_eg_7th

### Medium-Low Priority (Fine Tuning)
13. **MoveCountLimit3-8** - Tune as a group for consistency
14. **AspirationWindow** - Search efficiency
15. **LMRHistoryThreshold** - History impact on LMR
16. **NullMoveVerifyDepth** - Verification threshold

### Low Priority (Minor Adjustments)
17. **StabilityThreshold** - Time management
18. **Phase-specific stability** - Opening/Middle/Endgame
19. **LMRPvReduction/NonImprovingBonus** - LMR variations
20. **Queen/Rook passive positions** - Minor PST adjustments

---

## Example SPSA Workload for OpenBench

### Core Pruning Test (Futility & Razoring) ⭐⭐⭐
```
FutilityBase, int, 150.0, 50.0, 500.0, 20.0, 0.002
FutilityScale, int, 60.0, 20.0, 200.0, 10.0, 0.002
RazorMargin1, int, 300.0, 100.0, 800.0, 30.0, 0.002
RazorMargin2, int, 500.0, 200.0, 1200.0, 50.0, 0.002
```
Games: 50000  
Time Control: 10+0.1  
**Expected Impact:** High - these control core pruning behavior

### Null Move Suite ⭐⭐
```
NullMoveStaticMargin, int, 90.0, 50.0, 300.0, 15.0, 0.002
NullMoveReductionBase, int, 2.0, 1.0, 4.0, 0.5, 0.002
NullMoveReductionDepth6, int, 3.0, 2.0, 5.0, 0.5, 0.002
NullMoveReductionDepth12, int, 4.0, 3.0, 6.0, 0.5, 0.002
NullMoveEvalMargin, int, 200.0, 100.0, 400.0, 20.0, 0.002
```
Games: 60000  
Time Control: 10+0.1  

### Enhanced LMR Suite ⭐⭐
```
LMRMinDepth, int, 3.0, 1.0, 6.0, 1.0, 0.002
LMRMinMoveNumber, int, 6.0, 3.0, 12.0, 1.0, 0.002
LMRBaseReduction, int, 1.0, 0.0, 3.0, 0.5, 0.002
LMRDepthFactor, int, 3.0, 1.0, 10.0, 1.0, 0.002
LMRHistoryThreshold, int, 50.0, 10.0, 90.0, 5.0, 0.002
LMRPvReduction, int, 1.0, 0.0, 2.0, 0.5, 0.002
LMRNonImprovingBonus, int, 1.0, 0.0, 3.0, 0.5, 0.002
```
Games: 70000  
Time Control: 10+0.1  

### Endgame PST Pawn Suite ⭐⭐
```
pawn_eg_r3_d, int, 8.0, 0.0, 30.0, 2.0, 0.002
pawn_eg_r3_e, int, 7.0, 0.0, 30.0, 2.0, 0.002
pawn_eg_r4_d, int, 18.0, 10.0, 50.0, 3.0, 0.002
pawn_eg_r4_e, int, 16.0, 10.0, 50.0, 3.0, 0.002
pawn_eg_r5_d, int, 29.0, 20.0, 70.0, 4.0, 0.002
pawn_eg_r5_e, int, 27.0, 20.0, 70.0, 4.0, 0.002
pawn_eg_r6_d, int, 51.0, 30.0, 100.0, 5.0, 0.002
pawn_eg_r6_e, int, 48.0, 30.0, 100.0, 5.0, 0.002
pawn_eg_r7_center, int, 75.0, 50.0, 150.0, 8.0, 0.002
```
Games: 80000  
Time Control: 10+0.1  

### Knight & Bishop Endgame Suite ⭐
```
knight_eg_center, int, 15.0, 5.0, 25.0, 2.0, 0.002
knight_eg_extended, int, 10.0, 0.0, 20.0, 2.0, 0.002
knight_eg_edge, int, -25.0, -40.0, -10.0, 3.0, 0.002
knight_eg_corner, int, -40.0, -50.0, -20.0, 3.0, 0.002
bishop_eg_long_diag, int, 19.0, 10.0, 35.0, 2.0, 0.002
bishop_eg_center, int, 14.0, 5.0, 25.0, 2.0, 0.002
bishop_eg_edge, int, -5.0, -15.0, 5.0, 2.0, 0.002
```
Games: 70000  
Time Control: 10+0.1  

### Move Count Pruning Suite
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

### Mega-Tune (All High-Priority Parameters) ⭐⭐⭐⭐
```
MaxCheckPly, int, 6.0, 0.0, 10.0, 1.0, 0.002
FutilityBase, int, 150.0, 50.0, 500.0, 20.0, 0.002
FutilityScale, int, 60.0, 20.0, 200.0, 10.0, 0.002
RazorMargin1, int, 300.0, 100.0, 800.0, 30.0, 0.002
RazorMargin2, int, 500.0, 200.0, 1200.0, 50.0, 0.002
NullMoveReductionBase, int, 2.0, 1.0, 4.0, 0.5, 0.002
NullMoveStaticMargin, int, 90.0, 50.0, 300.0, 15.0, 0.002
LMRMinMoveNumber, int, 6.0, 3.0, 12.0, 1.0, 0.002
LMRMinDepth, int, 3.0, 1.0, 6.0, 1.0, 0.002
CountermoveBonus, int, 8000.0, 4000.0, 16000.0, 500.0, 0.002
```
Games: 120000  
Time Control: 10+0.1  
**Note:** This is a comprehensive tune but requires significant compute time  

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