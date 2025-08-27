# SeaJay Critical Bugs Investigation Plan
## Date: 2025-08-27
## Purpose: Systematic debugging of critical evaluation and tactical bugs

---

# Bug #1: Catastrophic Material Counting Failure

## Problem Statement
SeaJay evaluates positions where it is down significant material (full rook) as winning by large margins (+4.87 pawns). This indicates a fundamental failure in material counting or piece detection.

## Test Position
```
FEN: r2qk2r/ppp1bppp/3p1n2/4p3/2B1P3/2NP2QP/PPn2PP1/R1B2RK1 w kq - 0 11
```

## Reproduction Steps
1. Set up position after Black has played 10...Nxc2 (knight forking Queen on g3 and Rook on a1)
2. Run SeaJay evaluation at depth 10-12
3. Observe evaluation output

## What Happens (Bug)
```bash
# SeaJay's evaluation after 10...Nxc2 (knight forking Q and R):
echo -e "position fen r2qk2r/ppp1bppp/3p1n2/4p3/2B1P3/2NP2QP/PPn2PP1/R1B2RK1 w kq - 0 11\neval\ngo depth 10\nquit" | ./bin/seajay

# Expected SeaJay output:
# Evaluation: +1.42 pawns (or similar positive value)
# Best move suggested: Bh6 or Qxg7 (ignoring the fork)

# After 11.Bh6 Nxa1 (rook captured):
echo -e "position fen r2qk2r/ppp1bppp/3p1n1B/4p3/2B1P3/2NP2QP/PP3PP1/n1B2RK1 w kq - 0 12\neval\ngo depth 10\nquit" | ./bin/seajay

# Expected SeaJay output:
# Evaluation: +4.87 pawns (thinks it's winning by nearly 5 pawns!)
```

## What Should Happen (Correct)
```bash
# Stockfish evaluation of same position:
echo -e "position fen r2qk2r/ppp1bppp/3p1n2/4p3/2B1P3/2NP2QP/PPn2PP1/R1B2RK1 w kq - 0 11\neval\ngo depth 15\nquit" | ./external/engines/stockfish/stockfish

# Expected output:
# Evaluation: -3.5 to -4.0 pawns (White is losing, Black's knight wins the rook)
# Best move: Qf3 or Qd1 (saving the queen first)
```

## Investigation Focus Areas
1. **Material counting function**: Check how pieces are counted
2. **Piece detection**: Verify hanging piece detection
3. **Fork detection**: Check if engine recognizes forks
4. **Evaluation function**: Trace through evaluation with this specific position
5. **Search pruning**: Check if search is being cut off prematurely

## Key Questions to Answer
- Why does SeaJay think it's winning when down a rook?
- Is the rook on a1 being counted as still present after Nxa1?
- Is the knight on c2 (forking) being properly evaluated as attacking both pieces?
- Is there a bug in the material balance calculation?

---

# Bug #2: Starting Position Evaluation Bias

## Problem Statement
SeaJay evaluates the completely symmetric starting position as -289 centipawns (favoring Black by nearly 3 pawns), when it should evaluate as 0.00 (equal).

## Test Position
```
FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
(or simply: position startpos)
```

## Reproduction Steps
1. Set up starting position
2. Run SeaJay evaluation
3. Compare with expected value of 0

## What Happens (Bug)
```bash
# SeaJay's evaluation:
echo -e "position startpos\neval\ngo depth 10\nquit" | ./bin/seajay

# Expected SeaJay output:
# Evaluation: -289 cp (or -2.89 pawns)
# This means SeaJay thinks White is losing by almost 3 pawns at the start!
```

## What Should Happen (Correct)
```bash
# Stockfish evaluation:
echo -e "position startpos\neval\ngo depth 15\nquit" | ./external/engines/stockfish/stockfish

# Expected output:
# Evaluation: 0.00 (completely equal)
# Or very close to 0 (±0.10 pawns maximum)
```

## Investigation Focus Areas
1. **Piece-Square Tables (PST)**: Check for asymmetry in PST values
2. **Tempo bonus**: Verify side-to-move bonus is applied correctly
3. **Material values**: Ensure piece values are symmetric
4. **Initialization**: Check if evaluation components are initialized properly
5. **Color-dependent code**: Look for any White vs Black asymmetry

## Key Questions to Answer
- Are piece-square tables symmetric for White and Black?
- Is there a hardcoded bonus/penalty for one side?
- Is the tempo bonus being applied incorrectly?
- Are all evaluation terms properly mirrored for both colors?
- Is there an initialization bug affecting the first evaluation?

## Debug Commands
```bash
# Test with colors reversed:
echo -e "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1\neval\nquit" | ./bin/seajay
# Should be +289 cp if the bug is consistent

# Test after one move each:
echo -e "position startpos moves e2e4 e7e5\neval\nquit" | ./bin/seajay
# Check if evaluation is still biased
```

---

# Bug #3: King Safety Evaluation Failure

## Problem Statement
SeaJay voluntarily moves its king into extreme danger, failing to recognize basic king safety threats. Most critically, it played 17...Kf7 walking into a devastating attack.

## Test Position
```
FEN: r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17
```
Position after White's 17.Rbc1, Black to move.

## Reproduction Steps
1. Set up position after 17.Rbc1
2. Run SeaJay evaluation and get best move
3. Observe SeaJay's evaluation and move choice

## What Happens (Bug)
```bash
# SeaJay's evaluation and move:
echo -e "position fen r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17\neval\ngo depth 12\nquit" | ./bin/seajay

# Expected SeaJay output:
# Evaluation: +11.67 pawns (thinks Black is completely winning)
# Suggested moves: Often includes unsafe moves
# In the actual game: SeaJay played Kf7?! exposing the king

# Position after 17...Kf7:
echo -e "position fen r1b4r/pp3kpp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 w - - 6 18\neval\ngo depth 10\nquit" | ./bin/seajay
# SeaJay still thinks Black is better despite exposed king
```

## What Should Happen (Correct)
```bash
# Stockfish evaluation:
echo -e "position fen r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17\neval\ngo depth 20\nquit" | ./external/engines/stockfish/stockfish

# Expected output:
# Evaluation: Close to equal (0.00 to -0.50)
# Best moves: Defensive moves like Bd7, a5, or Qc4
# NOT Kf7 which exposes the king
```

## Investigation Focus Areas
1. **King safety evaluation**: Check king safety scoring function
2. **King exposure penalties**: Verify penalties for exposed kings
3. **Attack detection**: Check if attacks near king are detected
4. **Pawn shield evaluation**: Verify pawn shield bonus/penalties
5. **Piece proximity to king**: Check if enemy pieces near king are penalized

## Key Questions to Answer
- How much weight does king safety have in the evaluation?
- Is king exposure in the center properly penalized?
- Are enemy pieces near the king counted as threats?
- Is there a bonus for keeping the king castled?
- Why does SeaJay think exposing the king is acceptable?

## Additional Test Positions
```bash
# Test king in center vs castled:
# King in center (should be penalized):
echo -e "position fen r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1\neval\nquit" | ./bin/seajay

# After castling (should be better):
echo -e "position fen r4rk1/pppppppp/8/8/8/8/PPPPPPPP/R4RK1 w - - 0 1\neval\nquit" | ./bin/seajay
```

---

# Investigation Methodology

## For Each Bug:

### Step 1: Reproduce and Confirm
1. Run the exact test positions provided
2. Confirm the evaluation matches the bug description
3. Document exact output values

### Step 2: Code Tracing
1. Set breakpoints in evaluation function
2. Step through evaluation of test position
3. Log all component scores
4. Identify where incorrect values originate

### Step 3: Component Testing
1. Test each evaluation component in isolation
2. Verify material counting separately
3. Check each positional factor
4. Validate search is reaching expected depth

### Step 4: Fix Development
1. Identify minimal code change needed
2. Test fix with problematic position
3. Verify fix doesn't break other positions
4. Run regression tests

### Step 5: Validation
1. Re-test all positions from both games
2. Verify evaluations are reasonable
3. Check that moves suggested are sound
4. Compare with Stockfish for sanity check

## Success Criteria

### Bug #1 (Material Counting)
- Position with knight fork must evaluate as losing for White (-3 to -4 pawns)
- After rook capture, must show White is down material
- No positive evaluation when down significant material

### Bug #2 (Starting Position)
- Starting position must evaluate between -10 and +10 centipawns
- Ideally exactly 0.00
- Both colors must have symmetric evaluation

### Bug #3 (King Safety)
- Must not suggest moves that expose king to danger
- King in center must be penalized vs castled king
- Must recognize when king is under attack

## Branch Naming Convention
```
bugfix/20250827-material-counting
bugfix/20250827-startpos-evaluation  
bugfix/20250827-king-safety
```

## Testing After Fixes
Run these positions to verify all fixes work together:
1. Starting position (should be ~0.00)
2. Position with Nxc2 fork (should be -3.5 or worse for White)
3. Position before 17...Kf7 (should suggest safe moves)
4. Full game replay to ensure reasonable play throughout

---

# Priority Order
1. **Bug #1** - Material counting (most severe - engine thinks it's winning when losing)
2. **Bug #2** - Starting position (affects every game from move 1)
3. **Bug #3** - King safety (causes tactical disasters but less fundamental)

Each bug should be fixed and tested independently before moving to the next.