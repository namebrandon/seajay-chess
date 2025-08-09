# Stage 5: Testing Infrastructure - Implementation Checklist

## Phase 1: Configure fast-chess ✓ (Already Installed)
- [ ] Test fast-chess basic functionality with seajay
- [ ] Create tournament configuration template
- [ ] Set up time control configs (10+0.1 and 5+0.05)
- [ ] Configure adjudication settings
- [ ] Run test tournament (10 games seajay vs seajay)
- [ ] Verify PGN output is correct

## Phase 2: Implement Bench Command
- [ ] Add "bench" command to UCI handler
- [ ] Create BenchmarkSuite class with 12 positions
- [ ] Implement perft-based benchmarking (depth 3-4)
- [ ] Add timing with std::chrono::steady_clock
- [ ] Calculate and display NPS
- [ ] Test consistency across multiple runs
- [ ] Add optional depth parameter
- [ ] Document baseline performance

## Phase 3: Create Automated Testing Scripts
- [ ] Create perft validation script (Python/Bash)
- [ ] Create tournament runner wrapper script
- [ ] Create regression test script
- [ ] Add performance tracking script
- [ ] Test all scripts with error cases
- [ ] Create documentation for each script

## Phase 4: Configure SPRT Testing
- [ ] Create SPRT calculation script (Python with scipy)
- [ ] Set up parameters (alpha=0.05, beta=0.05, elo0=0, elo1=5)
- [ ] Integrate with fast-chess output
- [ ] Create live monitoring display
- [ ] Test SPRT with known results
- [ ] Document interpretation guide

## Phase 5: Set Up Opening Book
- [ ] Download 8moves_v3.pgn (or use existing 4moves_test.pgn)
- [ ] Configure fast-chess to use opening book
- [ ] Test opening diversity (100 games)
- [ ] Verify both engines get same position
- [ ] Document book configuration

## Validation & Testing
- [ ] Run 100-game tournament without crashes
- [ ] Verify bench command consistency
- [ ] Test SPRT detection of improvements
- [ ] Confirm all scripts run without errors
- [ ] Check memory usage in long runs
- [ ] Document all baseline metrics

## Documentation
- [ ] Update project_status.md
- [ ] Create script usage documentation
- [ ] Document SPRT interpretation
- [ ] Record baseline benchmark results
- [ ] Update deferred_items_tracker.md

## Implementation Order (Recommended)
1. Bench command (immediately useful for performance tracking)
2. fast-chess configuration (enables automated testing)
3. Basic test scripts (automation foundation)
4. SPRT setup (needed for Phase 2)
5. Opening book integration (final polish)

## Time Estimates
- Phase 1: 1-2 hours (fast-chess already installed)
- Phase 2: 2-3 hours (bench command)
- Phase 3: 3-4 hours (scripts)
- Phase 4: 2-3 hours (SPRT)
- Phase 5: 1 hour (opening book)
- Validation: 2-3 hours
**Total: 11-17 hours**

## Notes
- fast-chess located at: /workspace/external/testers/fast-chess/fastchess
- Existing 4moves_test.pgn at: /workspace/external/books/4moves_test.pgn
- Use Stockfish for perft validation: /workspace/external/engines/stockfish/stockfish
- Maintain consistent hardware for all tests
- Let SPRT decide when to stop (don't manually interrupt)