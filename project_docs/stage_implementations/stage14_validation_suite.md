# Stage 14: Quiescence Search - Validation Test Suite

## Quick Validation Commands

### After Each Code Change:
```bash
# Compile check
make -j

# Quick perft test (should be unchanged)
./bin/seajay perft 4

# Run unit tests
./bin/test_seajay --filter="*Quiescence*"
```

### After Each Deliverable:
```bash
# Full compile from clean
make clean && make -j

# Memory check (if suspicious)
valgrind --leak-check=full ./bin/seajay perft 3

# Tactical position test
echo "position fen r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 4 4" | ./bin/seajay
echo "go depth 6" | ./bin/seajay
```

---

## Critical Test Positions

### Test 1: Perpetual Check (MUST NOT LOOP)
```bash
# This position MUST terminate quickly
echo "position fen 3Q4/8/3K4/8/8/3k4/8/3q4 b - - 0 1" | ./bin/seajay
echo "go depth 10" | ./bin/seajay

# Expected: Should return draw score (0) quickly
# FAIL if: Infinite loop or crash
```

### Test 2: Stand-Pat Basic Test
```bash
# Quiet position - should use stand-pat
echo "position startpos" | ./bin/seajay
echo "go depth 1" | ./bin/seajay

# Before quiescence: [record score]
# After quiescence: Should be same or very similar
```

### Test 3: Simple Tactics
```bash
# Knight fork position
echo "position fen r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 4 4" | ./bin/seajay
echo "go depth 6" | ./bin/seajay

# Expected: Should find Nxe5 winning a pawn
```

### Test 4: Check Evasion
```bash
# King in check
echo "position fen rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3" | ./bin/seajay
echo "go depth 6" | ./bin/seajay

# Expected: Must find legal move to escape check
```

### Test 5: Stalemate Trap
```bash
# Stalemate awareness
echo "position fen 7k/8/6KQ/8/8/8/8/8 w - - 0 1" | ./bin/seajay
echo "go depth 8" | ./bin/seajay

# Expected: Should avoid Qg7 (stalemate), play Qg6 or similar
```

---

## Performance Validation

### Node Count Test
```bash
# Run with statistics
echo "position startpos" | ./bin/seajay
echo "setoption name UseQuiescence value false" | ./bin/seajay
echo "go depth 6" | ./bin/seajay
# Record: nodes_without_q = _____

echo "setoption name UseQuiescence value true" | ./bin/seajay
echo "go depth 6" | ./bin/seajay
# Record: nodes_with_q = _____

# Calculate: increase = (nodes_with_q / nodes_without_q - 1) * 100%
# Target: < 300% increase for Phase 1
```

### Time Performance Test
```bash
# Fixed depth timing
time echo -e "position startpos\ngo depth 8\nquit" | ./bin/seajay

# Record times:
# Without quiescence: _____
# With quiescence: _____
# Target: < 20% increase
```

---

## Safety Validation

### Stack Depth Test
```bash
# Deep position to test stack limits
echo "position fen r4rk1/1b2qppp/p1n1p3/1p6/3P4/P1N1P3/1PQ2PPP/R1B2RK1 b - - 0 1" | ./bin/seajay
echo "go depth 10" | ./bin/seajay

# Monitor: Should not crash
# Check: Maximum ply reached (should be < 128)
```

### Node Explosion Test
```bash
# Position with many captures
echo "position fen r1bqk1nr/pppp1ppp/2n5/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1" | ./bin/seajay
echo "qstats" | ./bin/seajay  # Show quiescence statistics
echo "go depth 6" | ./bin/seajay
echo "qstats" | ./bin/seajay  # Check node limits enforced

# Expected: qsearchNodes per position < 10000
```

### Time Limit Test
```bash
# Very short time limit
echo "position startpos" | ./bin/seajay
echo "go movetime 1" | ./bin/seajay

# Expected: Should return move within 1ms
# FAIL if: Timeout or no response
```

---

## Regression Tests

### Perft Consistency
```bash
# Perft must remain unchanged
./bin/seajay perft 5 > perft_with_qsearch.txt
diff perft_with_qsearch.txt ../tests/perft/perft_expected.txt

# Expected: No differences
```

### Existing Test Suite
```bash
# All existing tests must pass
cd /workspace/build
ctest --verbose

# Expected: 100% pass rate
```

---

## Tactical Improvement Tests

### WAC Test Suite (After Phase 3)
```bash
# Run WAC tactical test
./tools/run_wac_test.sh

# Baseline (Stage 13): ___/300
# Target (Stage 14): 280+/300
```

### Specific Tactical Patterns
```bash
# Back rank mate threat
echo "position fen 6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1" | ./bin/seajay
echo "go depth 8" | ./bin/seajay
# Should find Ra8#

# Promotion tactics
echo "position fen 8/2P5/3K4/8/8/3k4/2p5/8 w - - 0 1" | ./bin/seajay
echo "go depth 8" | ./bin/seajay
# Should evaluate correctly
```

---

## Debug Commands Testing

### Quiescence Statistics
```bash
echo "position startpos" | ./bin/seajay
echo "go depth 6" | ./bin/seajay
echo "qstats" | ./bin/seajay

# Should display:
# - qsearchNodes: _____
# - standPatCutoffs: _____
# - qsearchCutoffs: _____
# - deltasPruned: _____
# - qsearch ratio: _____%
```

### Quiescence Direct Test
```bash
echo "position startpos" | ./bin/seajay
echo "qsearch" | ./bin/seajay

# Should return:
# - Score: _____
# - Nodes: _____
# - Time: _____
```

---

## Validation Checklist Template

### For Each Deliverable:
- [ ] Compiles without warnings
- [ ] Perft unchanged
- [ ] Unit tests pass
- [ ] No memory leaks (spot check)
- [ ] Performance acceptable
- [ ] Git committed

### For Each Phase:
- [ ] All deliverables complete
- [ ] Critical positions pass
- [ ] Performance targets met
- [ ] Documentation updated
- [ ] No regressions
- [ ] Tag created

---

## Emergency Validation

If something goes wrong:

```bash
# Quick disable test
echo "setoption name UseQuiescence value false" | ./bin/seajay
echo "position startpos" | ./bin/seajay
echo "go depth 8" | ./bin/seajay

# Should work exactly as Stage 13
```

```bash
# Revert to last known good
git log --oneline -10  # Find last good commit
git checkout [commit]  # Revert to it
make clean && make -j  # Rebuild
./bin/seajay perft 5   # Verify working
```

---

## Notes

Record any validation failures or unexpected behaviors here for investigation.