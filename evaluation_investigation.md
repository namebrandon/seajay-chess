# SeaJay Evaluation and Search Investigation
## Date: 2025-08-27
## Position: Bug #3 - Tactical Blindness

---

## Executive Summary

SeaJay demonstrates catastrophic move selection despite reasonable static evaluation. In the test position, SeaJay's static evaluation (+221 cp) is close to correct (+300 cp), but the moves it selects (Kf7, Kd7, Qg2+, d5d4) all lose immediately or severely worsen the position.

---

## Test Position

```
FEN: r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17
```

**Position characteristics:**
- Black to move
- Material: Black up 3 pawns (Q+2R+B+7P vs Q+2R+B+4P)
- Key feature: White bishop on d6 deep in Black's position
- Black king still in center (e8)
- Black queen active on g4

---

## Evaluation Analysis

### Static Evaluation Comparison

| Engine | Evaluation | Notes |
|--------|------------|-------|
| Independent engines | +300 cp (±100) | White is better despite material deficit |
| SeaJay | +221 cp | Reasonably close to correct |
| Stockfish 17 | +46 cp | Surprisingly low, possible NNUE issue |

### SeaJay's Evaluation Breakdown

```
Material: -300 cp (Black up 3 pawns - correct)
PST: +50 cp (middlegame)
Other factors: +471 cp (!)
Total: +221 cp for White
```

**Key finding**: SeaJay adds +471 cp beyond material and PST, suggesting evaluation components (passed pawns, king safety, mobility, etc.) are heavily favoring White. While this seems to land us in the same evaluation range as independent engines, we should not assume SeaJay has arrived to the value with correct or similar logic. 

---

## Move Selection Catastrophe

### Moves SeaJay Suggests (by depth)

| Depth | Move | SeaJay's Eval | Actual Result |
|-------|------|---------------|---------------|
| 2 | d5d4 | +230 cp | Leads to a lost rook - After d5d4.. 18.Rxc8+ Kf7 19.Qxb7+ Kg6 20.Qxa8 Rxc8 21.Qxc8 |
| 4 | d5d4 | +216 cp | Same as above. Evaluations from other engines range from +8.00 to white to +16.00 for white. |
| 6 | e8f7 (Kf7) | +227 cp | Walks into Rc7+ attack (Komodo engine shows evaluation as +5.85 for white) |
| 8 | e8d7 (Kd7) | +203 cp | Position becomes +900 cp for White (Komodo engine shows evaluation as +9.2 for white) |
| 10 | e8f7 (Kf7) | +203 cp |  |
| 12 | e8f7 (Kf7) | Not tested |  |

### Critical Position Evaluations

| Position | SeaJay Eval | Correct Eval | Delta |
|----------|-------------|--------------|-------|
| Original (Black to move) | +221 cp | +300 cp | -79 cp (acceptable) |
| After Kd7 (White to move) | -205 cp | +900 cp | -1105 cp (!) |
| After Kf7 (White to move) | -227 cp | +900+ cp | -1127 cp (!) |
| After Qg2+ Kxg2 | +550 cp | M12 | Loses queen, enters into forced mate sequence |

---

## Specific Move Analysis

### 1. **Kf7 (e8f7)** - The Main Blunder
- **SeaJay's view**: Reasonable move, maintains evaluation
- **Reality**: Walks into immediate Rc7+ check
- **Consequence**: White gets attack with rook on 7th rank
- **Why SeaJay misses it**: Search doesn't see tactical consequences at reasonable depths

### 2. **Kd7 (e8d7)** - Also Terrible
- **SeaJay's view**: Safe king move
- **Reality**: Position deteriorates from +300 to +900 for White
- **Why**: King blocks own pieces, bishop on d6 dominates

### 3. **Qg2+ (g4g2)** - Loses Queen
- **SeaJay's static eval**: Thinks this gains material (+403 cp)
- **Reality**: Kg1xg2 simply captures the queen, leading to forced mate in 12.
- **Why SeaJay misses it**: Fundamental search failure

### 4. **d5d4** - Loses a Rook
- **SeaJay's view**: Advancing passed pawn
- **Reality**: Evaluation shows +12.00 for white after d5d4. White's next move of Rxc8+, leading to the king evading check (Kf7), then white's Qxb7+ force the king to evade again, allowing the white queen to capture black's rook on a8.
- **Stockfish also suggests this**: Possible evaluation blind spot in modern engines

---

## Root Cause Analysis

### What's Working
1. **Static evaluation**: Reasonably accurate (+221 vs +300 actual)
2. **Tactical detection after the fact**: Correctly finds Rc7+ after Kf7 is played
3. **Material counting**: Correctly counts pieces

### What's Broken
1. **Search evaluation of king moves**: Thinks positions after Kd7/Kf7 are good for Black when they're disasters
2. **Tactical lookahead**: Doesn't see immediate tactical shots (Rc7+, Qxd4+, Kxg2)
3. **Move ordering**: Best defensive moves may not be searched deeply enough
4. **Quiescence search**: Not catching critical tactics despite MaxCheckPly=6

### The Core Problem
**SeaJay's search function incorrectly evaluates positions after moves are made**. The discrepancy between:
- Static eval of position: +221 cp (reasonable)
- Eval after Kd7 in search: -205 cp (should be +900 cp)

This 1100 centipawn error indicates the search is fundamentally broken when evaluating certain types of moves, particularly king moves that expose the king to tactics.

---

## Testing Methodology

### Commands Used
```bash
# Static evaluation
echo -e "uci\nposition fen r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17\neval\nquit" | ./bin/seajay

# Search at various depths
echo -e "uci\nposition fen r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17\ngo depth 8\nquit" | ./bin/seajay

# Position after moves
echo -e "uci\nposition fen r1b4r/pp1k2pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 w - - 4 18\neval\nquit" | ./bin/seajay
```

### Build Configuration
- Built with: `./build.sh` (NOT make/OpenBench makefile which causes crashes)
- Version: Stage14-Remediated with eval command added
- Settings: Default UCI options, MaxCheckPly=6

---

## Implications

1. **SeaJay cannot play tactical positions**: Will blunder in any sharp position
2. **Search depth doesn't help**: Even at depth 10-12, still plays Kf7
3. **Evaluation components may be over-tuned**: +471 cp from non-material factors seems excessive, though we did arrive at roughly a correct evaluation when compared to other independent engines for this position with the +471 cp.
4. **Quiescence search not working**: Despite hybrid implementation with check extensions

---

## Recommended Next Steps

1. **Debug search function**: Trace why positions after Kd7/Kf7 are evaluated incorrectly
2. **Review quiescence search**: Why doesn't it catch Rc7+ or Qxd4+?
3. **Check move ordering**: Are good defensive moves being pruned?
4. **Validate evaluation components**: The +471 cp from other factors needs investigation to ensure they are correct in their impact to our evaluation.
5. **Test simpler tactical positions**: Isolate if this is king-safety specific or general tactical blindness

---

## Additional Notes

- The crash issue (assertion failure) was due to using make instead of build.sh. ALWAYS USE build.sh
- UCI eval command was successfully implemented and helps debugging
- The issue is does not appear to be in static evaluation but in search/move selection
- Both Kf7 and Kd7 are very bad, turning a difficult position into a worse one
- **SEAJAY USES NEGAMAX SCORING** and will always return the score from the perspective of the side to move.