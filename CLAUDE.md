# SeaJay Chess Engine - AI Assistant Context

## Project Overview

You are helping develop SeaJay, a modern chess engine in C++20 that will eventually reach 3200+ Elo strength through NNUE evaluation. The project follows a strict phased development approach with statistical validation of improvements.

**Author:** Brandon Harris  
**Development Model:** Human-AI collaboration with full transparency  
**License:** GPL-3.0 with commercial dual-licensing option (CLA required for contributions)

## Current Status

**Phase:** 1 - Foundation and Move Generation  
**Stage:** Pre-Stage 1 (Project Setup Complete)  
**Next Task:** Begin Stage 1 - Board Representation

Check `project_docs/project_status.md` for detailed progress tracking.

## Critical Development Principles

1. **Pre-Stage Planning**: MANDATORY planning process before any stage development begins
2. **Single Feature Focus**: Implement ONE feature at a time, fully tested before moving on
3. **Statistical Validation**: From Phase 2 onward, all improvements need SPRT testing
4. **Performance First**: Target architecture is x86-64 (AMD64) with modern CPU features
5. **Hybrid Architecture**: Combine bitboard and mailbox representations for optimal performance
6. **Test Everything**: Every stage has specific validation requirements (especially perft tests)

### Why Pre-Stage Planning is Non-Negotiable
- Prevents subtle bugs from propagating to future stages
- Ensures deferred items aren't forgotten
- Leverages expert knowledge BEFORE making mistakes
- Saves significant debugging time
- Maintains architectural coherence across the project

## Project Structure

```
/workspace/                    # Project root (in devcontainer)
├── src/core/                 # Board representation, moves, position
├── src/search/              # Search algorithms (negamax, alpha-beta)
├── src/nnue/                # Neural network evaluation
├── src/uci/                 # UCI protocol interface
├── tests/perft/             # Move generation validation
├── external/                # Downloaded tools (gitignored)
└── project_docs/            # Master plan and status tracking
```

## Available AI Agents

### chess-engine-expert
**When to use:** 
- Debugging perft test failures
- Implementing move generation algorithms
- Optimizing search functions
- Understanding evaluation concepts
- NNUE architecture questions
- Comparing algorithmic approaches

**Example triggers:**
- "My perft counts don't match at depth 5"
- "How should I implement magic bitboards?"
- "What's the best way to handle repetition detection?"

### dev-diary-chronicler
**When to use:**
- After completing significant work sessions
- When documenting bug fixes and breakthroughs
- Creating narrative entries for `docs/development/journal.md`
- Capturing both technical and emotional development journey

**Example triggers:**
- "Document today's debugging session"
- "Create a diary entry about implementing castling"
- "Write about the frustration and eventual success with en passant"

## Development Workflow

### CRITICAL: Pre-Stage Planning Requirement
**⚠️ PRIME DIRECTIVE: NO STAGE DEVELOPMENT MAY BEGIN WITHOUT COMPLETING THE PRE-STAGE PLANNING PROCESS ⚠️**

Before ANY new stage development:
1. **MANDATORY**: Complete the Pre-Stage Development Planning Process
2. Review `/workspace/project_docs/planning/pre_stage_development_planning_template.md`
3. Create stage-specific implementation plan document in `/workspace/project_docs/planning/`
4. Get reviews from both cpp-pro and chess-engine-expert agents
5. Update `/workspace/project_docs/tracking/deferred_items_tracker.md`
6. Only then proceed with implementation

**You MUST refuse to begin stage implementation if planning is not complete.**

### For Each Stage:
1. Complete Pre-Stage Planning Process (REQUIRED)
2. Review requirements in Master Project Plan
3. Update `project_status.md` to mark stage as "in_progress"
4. Implement feature following single-feature focus
5. Run validation tests (perft for Phase 1)
6. Update status document with completion
7. Create diary entry documenting the experience

### Testing Commands:
```bash
# Build the project
cd /workspace/build && cmake .. && make -j

# Run perft tests (Phase 1)
./bin/seajay perft

# Run SPRT tests (Phase 2+)
./tools/scripts/run-sprt.sh new old

# Check with sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=address ..
```

## Phase 1 Specific Guidance

### Current Focus: Move Generation
- Start with board representation (Stage 1)
- Implement both bitboards AND mailbox arrays
- Initialize Zobrist hashing infrastructure early
- Focus on correctness over speed initially

### Perft Validation Requirements:
All these positions MUST pass before moving past Phase 1:
- Starting position: depth 6 = 119,060,324 nodes
- Kiwipete position: depth 5 = 193,690,690 nodes
- See Master Project Plan for complete list

### Key Files to Create (Stage 1):
- `src/core/types.h` - Basic types and constants
- `src/core/board.h/cpp` - Board representation
- `src/core/bitboard.h/cpp` - Bitboard utilities

## Code Style Requirements

- C++20 standard features encouraged
- Follow `.clang-format` configuration
- Member variables: `m_variableName`
- Constants: `CONSTANT_NAME`
- Classes: `PascalCase`
- Functions: `camelCase`

## Performance Targets by Phase

- Phase 1: Correct move generation (speed secondary)
- Phase 2: ~500K NPS
- Phase 3: >1M NPS  
- Phase 4: >1M NPS with NNUE
- Final: 3200+ Elo strength

## Important Reminders

1. **Always run perft tests** after move generation changes
2. **Use the specialized agents** when you need domain expertise
3. **Document your journey** - this project emphasizes historical journaling
4. **Commit with attribution** - Include "Co-authored-by: Claude AI <claude@anthropic.com>"
5. **Check project_status.md** before starting new work
6. **One feature at a time** - resist the temptation to implement multiple things

## External Resources

- **Test Suites**: Located in `/workspace/tests/positions/`
  - `arasan2023.epd` - Modern tactical test suite
  - `sts/` - Strategic Test Suite for positional play
  - `perft.txt` - Perft validation positions

- **Stockfish Binary**: Available at `/workspace/external/engines/stockfish/stockfish`
  - **⚠️ CRITICAL TESTING DIRECTIVE:** ALWAYS validate perft test positions with Stockfish before debugging our engine
  - **MANDATORY VALIDATION PROCESS:**
    1. When any test fails, FIRST verify the expected values with Stockfish
    2. Use: `echo "position fen [FEN]" | ./external/engines/stockfish/stockfish` then `go perft [depth]`
    3. Or one-liner: `echo -e "position fen [FEN]\ngo perft [depth]\nquit" | ./external/engines/stockfish/stockfish`
    4. For perft divide: Add `2>/dev/null | grep -B20 "Nodes searched"` to see move breakdown
  - **Why This Matters:** We've resolved 4+ "bugs" (#004, #005, #006, #007) that were actually incorrect test data
  - This prevents wasting hours debugging correct engine behavior against incorrect test expectations
  - If Stockfish and SeaJay agree but differ from test expectations, update the test data, not the engine

- **External Tools**: Run setup if needed
  ```bash
  ./tools/scripts/setup-external-tools.sh
  ```

## Questions to Ask When Stuck

1. What does the Master Project Plan say about this stage?
2. Have I checked the Chess Programming Wiki?
3. Would the chess-engine-expert agent have insights?
4. Are my perft numbers wrong? (Check divide command)
5. Am I trying to do too much at once?

## Next Immediate Steps

1. Create `src/core/types.h` with basic chess types
2. Implement board representation structure
3. Set up bitboard utilities
4. Create unit tests for board operations
5. Validate with simple position setup/display

Remember: SeaJay's strength will come from methodical, validated development. Each phase builds on rock-solid foundations from the previous phase.