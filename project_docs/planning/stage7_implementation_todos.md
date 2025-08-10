# Stage 7: Negamax Search - Implementation TODOs

## Phase 1: Core Negamax (Day 1 Morning)
- [ ] Create search/types.h with SearchInfo struct
- [ ] Create search/negamax.h/cpp files
- [ ] Implement basic recursive negamax without alpha-beta
- [ ] Add node counting
- [ ] Add seldepth tracking
- [ ] Implement terminal node evaluation
- [ ] Handle checkmate/stalemate detection
- [ ] Add debug assertions for state validation
- [ ] Unit test negamax with simple positions

## Phase 2: Alpha-Beta Framework (Day 1 Afternoon)
- [ ] Add alpha-beta parameters to negamax
- [ ] Pass infinite bounds initially
- [ ] Structure code for future beta cutoffs
- [ ] Add placeholder for move ordering
- [ ] Test that results unchanged with infinite bounds

## Phase 3: Iterative Deepening (Day 1 Evening)
- [ ] Create search controller function
- [ ] Implement ID loop from depth 1
- [ ] Store best move at each iteration
- [ ] Add early exit on mate score
- [ ] Test ID produces same results as fixed depth

## Phase 4: Time Management (Day 2 Morning)
- [ ] Create SearchLimits struct
- [ ] Implement time allocation (5% of remaining)
- [ ] Add time checking every 4096 nodes
- [ ] Handle search interruption cleanly
- [ ] Add safety margins to time allocation
- [ ] Test time limits are respected

## Phase 5: UCI Integration (Day 2 Afternoon)
- [ ] Parse go command parameters
- [ ] Support go depth X
- [ ] Support go movetime X
- [ ] Support go wtime/btime/winc/binc
- [ ] Support go infinite
- [ ] Implement UCI info output
- [ ] Send depth, score, nodes, nps, time, pv
- [ ] Update selectBestMove to use negamax
- [ ] Test with chess GUI

## Phase 6: Testing & Validation (Day 2 Evening)
- [ ] Create mate-in-1 test suite
- [ ] Create mate-in-2 test suite
- [ ] Test starting position to depth 4
- [ ] Verify NPS in expected range (50K-200K)
- [ ] Run perft tests to ensure move gen unchanged
- [ ] Memory leak check with valgrind
- [ ] Run 100 game test for stability
- [ ] Prepare SPRT test vs Stage 6

## Bug Prevention Checklist
- [ ] Add hash validation after unmake
- [ ] Check score negation is correct
- [ ] Verify mate score propagation
- [ ] Test make/unmake state preservation
- [ ] Validate ply counting
- [ ] Check for stack overflow protection
- [ ] Ensure proper side-to-move handling

## Test Positions

### Mate in 1
```
6k1/5ppp/8/8/8/8/8/R7 w - - 0 1  // Ra8#
7k/8/8/8/8/8/1Q6/7K w - - 0 1   // Qb8# or Qa8# or Qa7#
r3k3/8/8/8/8/8/8/2KR4 w - - 0 1  // Rd8#
```

### Mate in 2
```
6k1/5ppp/8/8/8/8/R7/6K1 w - - 0 1  // Ra8+ then mate
7k/8/8/8/8/8/8/KQ6 w - - 0 1     // K+Q vs K
5rk1/5ppp/8/8/8/8/5PPP/3RR1K1 w - - 0 1  // Back rank pattern
```

### Tactical Positions (4 ply)
```
r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1  // Castling and tactics
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1  // Starting position
```

## Performance Targets
- Depth 1: < 0.1 seconds
- Depth 2: < 1 second  
- Depth 3: < 10 seconds
- Depth 4: < 60 seconds (from startpos)
- NPS: 50,000 - 200,000 (without optimizations)

## Documentation Updates
- [ ] Update search.h with new functions
- [ ] Document SearchInfo struct
- [ ] Add search algorithm description
- [ ] Update README with search capability
- [ ] Create Stage 7 implementation summary
- [ ] Update project_status.md

## Success Criteria
- [ ] All mate-in-1 found
- [ ] 80%+ mate-in-2 found
- [ ] Depth 4 search completes in reasonable time
- [ ] Time management works correctly
- [ ] UCI info output functional
- [ ] No crashes in extended testing
- [ ] SPRT shows +200 Elo vs Stage 6

## Notes
- Keep implementation simple - optimization comes later
- Focus on correctness over speed
- Test frequently during development
- Use debug assertions liberally
- Document any deferred items discovered