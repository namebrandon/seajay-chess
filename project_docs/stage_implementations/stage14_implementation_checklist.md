# Stage 14: Quiescence Search - Implementation Checklist

**Stage:** 14 - Quiescence Search  
**Date Started:** [TBD]  
**Target Completion:** [TBD]  
**Theme:** METHODICAL VALIDATION  

## Critical Reminders
- [ ] Each deliverable MUST be < 50 lines of code
- [ ] Commit after EVERY successful validation
- [ ] Update this document after EACH step
- [ ] Run tests after EVERY code change
- [ ] Document any issues immediately

---

## Phase 1: Minimal SAFE Implementation (Days 1-2)

### Day 1 Morning: Infrastructure Setup (2 hours)

#### Deliverable 1.1: Create Header File Structure
**Code Changes:**
- [x] Create `/workspace/src/search/quiescence.h` with function declarations only
- [x] Add include guards and namespace

**Validation:**
- [x] File compiles with `make -j`
- [x] No compilation warnings

**Git Commit:**
```bash
git add src/search/quiescence.h
git commit -m "feat(qsearch): Add quiescence search header file structure"
```

**Documentation:**
- [ ] Create `/workspace/project_docs/stage_implementations/stage14_phase1_infrastructure.md`
- [ ] Document file structure decisions

---

#### Deliverable 1.2: Add Safety Constants
**Code Changes:**
- [x] Add safety constants to `quiescence.h`:
  - `QSEARCH_MAX_PLY = 32`
  - `TOTAL_MAX_PLY = 128`
  - `NODE_LIMIT_PER_POSITION = 10000`
  - `MAX_CAPTURES_PER_NODE = 32`

**Validation:**
- [x] Verify constants compile
- [x] Add static_asserts for safety checks
- [x] Test compilation with `make clean && make -j`

**Git Commit:**
```bash
git add src/search/quiescence.h
git commit -m "feat(qsearch): Add safety constants and limits"
```

**Documentation:**
- [ ] Update `stage14_phase1_infrastructure.md` with constant rationale

---

#### Deliverable 1.3: Extend SearchData Structure
**Code Changes:**
- [x] Add to `src/search/types.h`:
  ```cpp
  uint64_t qsearchNodes = 0;
  uint64_t qsearchCutoffs = 0;
  uint64_t standPatCutoffs = 0;
  uint64_t deltasPruned = 0;
  ```

**Validation:**
- [x] Compile entire project
- [x] Run existing tests to ensure no breakage
- [x] Verify SearchData initialization

**Git Commit:**
```bash
git add src/search/types.h
git commit -m "feat(qsearch): Extend SearchData with quiescence metrics"
```

**Documentation:**
- [ ] Document SearchData changes in implementation doc

---

### Day 1 Afternoon: Stand-Pat Implementation (3 hours)

#### Deliverable 1.4: Create Minimal Quiescence Function
**Code Changes:**
- [x] Create `/workspace/src/search/quiescence.cpp`
- [x] Implement ONLY stand-pat logic (no move generation yet):
  ```cpp
  eval::Score quiescence(...) {
      // Safety checks
      if (ply >= TOTAL_MAX_PLY) return board.evaluate();
      
      // Stand-pat only
      eval::Score staticEval = board.evaluate();
      if (staticEval >= beta) return staticEval;
      
      return std::max(alpha, staticEval);
  }
  ```

**Validation:**
- [x] Function compiles
- [x] Write unit test for stand-pat behavior
- [x] Test returns correct values for quiet positions

**Git Commit:**
```bash
git add src/search/quiescence.cpp tests/search/test_quiescence.cpp
git commit -m "feat(qsearch): Implement minimal stand-pat quiescence"
```

**Documentation:**
- [ ] Create `stage14_phase1_standpat.md`
- [ ] Document stand-pat implementation

---

#### Deliverable 1.5: Add Repetition Detection
**Code Changes:**
- [x] Add repetition check BEFORE stand-pat:
  ```cpp
  if (searchInfo.isRepetition(board.zobristKey())) {
      return 0;  // Draw
  }
  ```

**Validation:**
- [x] Test with perpetual check position
- [x] Verify no infinite loops
- [x] Run perft to ensure no side effects

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "fix(qsearch): Add critical repetition detection"
```

**Documentation:**
- [ ] Update implementation doc with repetition handling

---

#### Deliverable 1.6: Add Time Checking
**Code Changes:**
- [x] Add time check every 1024 nodes:
  ```cpp
  if ((data.qsearchNodes & 1023) == 0) {
      if (searchInfo.shouldStop()) return 0;
  }
  ```

**Validation:**
- [x] Test with very short time limit (1ms)
- [x] Verify search stops appropriately
- [x] Check no timeouts occur

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Add time limit checking"
```

---

### Day 1 Evening: Integration Point (2 hours)

#### Deliverable 1.7: Integrate with Negamax
**Code Changes:**
- [ ] Modify `src/search/negamax.cpp` line ~188:
  ```cpp
  if (depth <= 0) {
      return quiescence(board, ply, alpha, beta, searchInfo, info, tt);
  }
  ```
- [ ] Add include for quiescence.h

**Validation:**
- [ ] Compile entire project
- [ ] Run perft tests - should still pass
- [ ] Run basic search test - should return same scores for quiet positions

**Git Commit:**
```bash
git add src/search/negamax.cpp src/search/CMakeLists.txt
git commit -m "feat(qsearch): Integrate quiescence with main search"
```

**Documentation:**
- [ ] Create `stage14_integration_notes.md`
- [ ] Document integration point

---

#### Deliverable 1.8: Add UCI Kill Switch
**Code Changes:**
- [ ] Add to UCI options:
  ```cpp
  options["UseQuiescence"] = UCIOption(true);
  ```
- [ ] Add flag check in negamax

**Validation:**
- [ ] Test UCI option works
- [ ] Verify search works with quiescence on/off
- [ ] Compare outputs with flag toggled

**Git Commit:**
```bash
git add src/uci/uci.cpp src/search/negamax.cpp
git commit -m "feat(qsearch): Add UCI kill switch for emergency disable"
```

---

### Day 2 Morning: Capture Generation (3 hours)

#### Deliverable 2.1: Basic Capture Search
**Code Changes:**
- [ ] Add capture generation after stand-pat:
  ```cpp
  MoveList captures;
  MoveGenerator::generateCaptures(board, captures);
  // Just generate, don't search yet
  return staticEval;  // Still return stand-pat for now
  ```

**Validation:**
- [ ] Verify captures are generated correctly
- [ ] Check move count matches expected
- [ ] No crashes or memory issues

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Add capture generation (not searched yet)"
```

---

#### Deliverable 2.2: Add Capture Ordering
**Code Changes:**
- [ ] Add MVV-LVA ordering:
  ```cpp
  orderMovesMVVLVA(captures, board);
  ```

**Validation:**
- [ ] Verify moves are ordered correctly
- [ ] Test with position having multiple captures
- [ ] Check first move is best capture

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Add MVV-LVA ordering for captures"
```

---

#### Deliverable 2.3: Search First Capture Only
**Code Changes:**
- [ ] Search just the first capture:
  ```cpp
  if (!captures.empty()) {
      Move move = captures[0];
      board.makeMove(move);
      eval::Score score = -quiescence(board, ply+1, -beta, -alpha, ...);
      board.unmakeMove(move);
      return std::max(staticEval, score);
  }
  ```

**Validation:**
- [ ] Test doesn't crash
- [ ] Check recursion works
- [ ] Verify ply limits respected

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Search first capture only (testing recursion)"
```

---

#### Deliverable 2.4: Search All Captures
**Code Changes:**
- [ ] Full capture search loop:
  ```cpp
  eval::Score bestScore = staticEval;
  for (const Move& move : captures) {
      board.makeMove(move);
      eval::Score score = -quiescence(...);
      board.unmakeMove(move);
      bestScore = std::max(bestScore, score);
      if (score >= beta) return score;  // Beta cutoff
  }
  return bestScore;
  ```

**Validation:**
- [ ] Test with tactical position
- [ ] Verify finds simple tactics
- [ ] Check node counts are reasonable

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Implement full capture search"
```

---

### Day 2 Afternoon: Check Handling (3 hours)

#### Deliverable 2.5: Add Check Detection
**Code Changes:**
- [ ] Add check detection before stand-pat:
  ```cpp
  if (board.inCheck()) {
      // For now, just return static eval
      // Will implement full handling next
      return board.evaluate();
  }
  ```

**Validation:**
- [ ] Test with check positions
- [ ] Verify detection works
- [ ] No crashes

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Add basic check detection"
```

---

#### Deliverable 2.6: Create quiescenceInCheck Function
**Code Changes:**
- [ ] Create separate function for check handling:
  ```cpp
  eval::Score quiescenceInCheck(Board& board, int ply, ...) {
      // Generate ALL legal moves (not just captures)
      MoveList moves;
      MoveGenerator::generateLegalMoves(board, moves);
      
      if (moves.empty()) {
          return -MATE_SCORE + ply;  // Checkmate
      }
      
      // For now, just return 0
      return 0;
  }
  ```

**Validation:**
- [ ] Test checkmate detection
- [ ] Verify legal move generation
- [ ] Test with various check positions

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Add quiescenceInCheck with checkmate detection"
```

---

#### Deliverable 2.7: Implement Check Evasion Search
**Code Changes:**
- [ ] Add full search in quiescenceInCheck:
  ```cpp
  eval::Score bestScore = -MATE_SCORE;
  for (const Move& move : moves) {
      board.makeMove(move);
      eval::Score score = -quiescence(board, ply+1, -beta, -alpha, ...);
      board.unmakeMove(move);
      bestScore = std::max(bestScore, score);
      if (score >= beta) return score;
  }
  return bestScore;
  ```

**Validation:**
- [ ] Test perpetual check position
- [ ] Verify no infinite loops
- [ ] Test checkmate in N positions

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Complete check evasion implementation"
```

---

### Day 2 Evening: Safety Limits (2 hours)

#### Deliverable 2.8: Add Node Limit Per Position
**Code Changes:**
- [ ] Track entry nodes and check limit:
  ```cpp
  uint64_t entryNodes = data.qsearchNodes;
  // In loop:
  if (data.qsearchNodes - entryNodes > NODE_LIMIT_PER_POSITION) {
      return staticEval;  // Abort
  }
  ```

**Validation:**
- [ ] Test with complex tactical position
- [ ] Verify limit is enforced
- [ ] Check no explosions

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "safety(qsearch): Add per-position node limit"
```

---

#### Deliverable 2.9: Add Capture Limit Per Node
**Code Changes:**
- [ ] Limit captures searched:
  ```cpp
  int capturesSearched = 0;
  for (const Move& move : captures) {
      if (++capturesSearched > MAX_CAPTURES_PER_NODE) break;
      // ... rest of search
  }
  ```

**Validation:**
- [ ] Test with position having many captures
- [ ] Verify limit works
- [ ] Check performance impact

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "safety(qsearch): Add capture limit per node"
```

---

## Phase 1 Validation Suite (End of Day 2)

### Comprehensive Testing
- [ ] Run full perft suite
- [ ] Test all critical positions:
  - [ ] Perpetual check position
  - [ ] Stalemate trap
  - [ ] Deep tactics
  - [ ] Promotion races
- [ ] Run 100-game match vs Stage 13
- [ ] Profile performance metrics

### Documentation
- [ ] Create `stage14_phase1_complete.md`
- [ ] Document all issues encountered
- [ ] Record performance metrics
- [ ] Update implementation plan if needed

### Git Tag
```bash
git tag stage14-phase1-complete
git push origin stage14-phase1-complete
```

---

## Phase 2: Critical Safety Features (Days 3-4)

### Day 3: Delta Pruning

#### Deliverable 3.1: Basic Delta Pruning
**Code Changes:**
- [ ] Add futility check before capture loop:
  ```cpp
  if (staticEval + 900 < alpha) {  // Queen value
      return staticEval;  // Futile
  }
  ```

**Validation:**
- [ ] Test pruning works
- [ ] Verify doesn't miss tactics
- [ ] Check node reduction

**Git Commit:**
```bash
git add src/search/quiescence.cpp
git commit -m "feat(qsearch): Add basic delta pruning"
```

[Continue with similar granular breakdown for remaining phases...]

---

## Critical Validation Points

### After Each Deliverable:
1. **Compile Check:** `make -j`
2. **Test Suite:** Run relevant tests
3. **Perft Verification:** Ensure move generation unchanged
4. **Memory Check:** Run with sanitizers if suspicious
5. **Performance Check:** Monitor node counts

### After Each Phase:
1. **Full Test Suite:** All tests must pass
2. **Tactical Tests:** WAC positions
3. **Performance Profile:** Check for bottlenecks
4. **Match Testing:** Games vs previous version
5. **Documentation Update:** Complete phase summary

---

## Emergency Rollback Plan

If critical issues arise:
1. **Immediate:** Disable with UCI flag
2. **Short-term:** Revert to last working commit
3. **Analysis:** Document issue in postmortem
4. **Fix:** Create targeted fix on branch
5. **Re-integrate:** Careful testing before merge

---

## Success Metrics

### Phase 1 Success:
- [ ] No crashes in 1000 positions
- [ ] Perpetual check handled correctly
- [ ] Time limits respected
- [ ] <50% node increase

### Phase 2 Success:
- [ ] Delta pruning reduces nodes by 20%+
- [ ] TT integration working
- [ ] Statistics accurate

### Phase 3 Success:
- [ ] 150+ Elo improvement in SPRT
- [ ] WAC score >280/300
- [ ] <20% time increase

---

## Notes Section
[Record any issues, insights, or deviations from plan here]