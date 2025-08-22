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
7. **Stage Completion Checklist**: MANDATORY completion of `/workspace/project_docs/stage_implementations/stage_completion_checklist.md` before marking any stage as COMPLETE
8. **NO COMPILE-TIME FEATURE FLAGS**: All features MUST compile in. Use UCI options for runtime control. Never use #ifdef for core features (Stage 14 lesson: lost 4 hours to this)

## PRIMARY DIRECTIVE: Testing Integrity

**⚠️ CRITICAL: NEVER simulate test results. NEVER create fake data. ⚠️**

If testing cannot be completed due to time/context constraints:
1. Be transparent about the limitation
2. Document what was implemented
3. Prepare test scripts for human execution
4. Mark work as "Implementation Complete - Awaiting Validation"
5. Request human assistance with long-running tests

**Why This Matters:**
- SeaJay's development relies on scientific rigor and statistical validation
- Fake results undermine the entire development process
- Trust and transparency are fundamental to the human-AI collaboration
- SPRT tests may take 15+ minutes to hours - this is expected and normal

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

## Quiescence Search Build Modes (Stage 14)

SeaJay supports three quiescence search build modes to facilitate development and testing:

### TESTING Mode (10K node limit)
- **Purpose:** Rapid development iteration and debugging
- **Build:** `./build_testing.sh` or `./build.sh testing`
- **Node Limit:** 10,000 nodes per quiescence search
- **Use When:** Developing new features, debugging search issues
- **Engine Display:** "Quiescence: TESTING MODE - 10K limit"

### TUNING Mode (100K node limit)
- **Purpose:** Parameter tuning and experimentation
- **Build:** `./build_tuning.sh` or `./build.sh tuning`
- **Node Limit:** 100,000 nodes per quiescence search
- **Use When:** Finding optimal parameters, testing trade-offs
- **Engine Display:** "Quiescence: TUNING MODE - 100K limit"

### PRODUCTION Mode (no limits)
- **Purpose:** Full strength for SPRT testing and competitive play
- **Build:** `./build_production.sh` or `./build.sh production`
- **Node Limit:** Unlimited (full quiescence search)
- **Use When:** SPRT testing, playing matches, final validation
- **Engine Display:** "Quiescence: PRODUCTION MODE"

**Important:** The engine always displays its mode at UCI startup to prevent confusion.
Always verify you're running the intended mode before testing.

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

### 🛑 CRITICAL: OpenBench Testing After EVERY Phase
**⚠️ PRIME DIRECTIVE: STOP AFTER EVERY PHASE FOR OPENBENCH TESTING ⚠️**

**MANDATORY PROCESS:**
1. Complete a phase (2-3 changes maximum)
2. Commit with "bench <node-count>" in message
3. **🛑 STOP - DO NOT PROCEED**
4. Update status to "AWAITING OPENBENCH TEST"
5. Wait for human to run OpenBench test
6. Only proceed to next phase after human confirms test results

**You MUST refuse to continue to the next phase without OpenBench test confirmation.**
**This applies to EVERY phase, even "minor" changes or refactoring.**

### For Each Stage:
1. Complete Pre-Stage Planning Process (REQUIRED)
2. Review requirements in Master Project Plan
3. Update `project_status.md` to mark stage as "in_progress"
4. Implement feature following single-feature focus
5. Run validation tests (perft for Phase 1)
6. **MANDATORY**: Complete Stage Completion Checklist (`/workspace/project_docs/stage_implementations/stage_completion_checklist.md`)
7. Update status document with completion
8. Create diary entry documenting the experience
9. Archive completed checklist with stage documentation

### Testing Commands:
```bash
# IMPORTANT: Build using build.sh scripts ONLY - NEVER use ninja directly!
# OpenBench requires 'make' compatibility, not ninja

# Build the project (Stage 14: Quiescence Search modes)
# Quick build with mode selection:
./build.sh              # Default production build
./build.sh production   # Production mode explicitly
./build.sh testing      # Testing mode: 10K node limit
./build.sh tuning       # Tuning mode: 100K node limit  

# Or use dedicated build scripts (recommended for clarity):
./build_production.sh   # Production mode, no limits (RECOMMENDED)
./build_testing.sh      # Testing mode with 10K limit
./build_tuning.sh       # Tuning mode with 100K limit
./build_debug.sh        # Debug build with sanitizers

# NEVER use ninja directly - always use build.sh or make
# OpenBench compatibility requires make, not ninja

# Verify build mode:
echo 'uci' | ./bin/seajay  # Engine displays mode at startup

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

## Git Branch Management

This project uses a structured branch naming convention for OpenBench testing:
- `feature/YYYYMMDD-name` - New features
- `bugfix/YYYYMMDD-name` - Bug fixes  
- `test/YYYYMMDD-name` - Experiments
- `tune/YYYYMMDD-name` - SPSA tuning
- `ob/YYYYMMDD-name` - Historical references

### Setup Git Aliases

Install the Git aliases using the automated script:
```bash
chmod +x /workspace/setup_git_aliases.sh
./setup_git_aliases.sh
```

### Branch Creation
- `git feature <name>` - Create feature branch
- `git bugfix <name>` - Create bugfix branch
- `git test <name>` - Create test branch
- `git tune <name>` - Create tuning branch
- `git ob <name>` - Create historical reference

### Branch Management
- `git list-all-branches` - Show all organized branches
- `git list-features` - Show feature branches
- `git list-bugfix` - Show bugfix branches
- `git list-tests` - Show test branches
- `git list-tune` - Show tuning branches
- `git list-ob` - Show historical references
- `git show-branch-age` - Show branch ages

### Branch Cleanup (use with caution!)
- `git clean-tests` - Delete ALL test branches
- `git clean-old-tests` - Delete test branches older than 7 days
- `git clean-bugfix` - Delete ALL bugfix branches
- `git clean-features` - Delete ALL feature branches
- `git clean-tune` - Delete ALL tune branches

**CRITICAL for OpenBench:** Every commit MUST include "bench <node-count>" in the message. Get node count with:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'
```

## Important Reminders

1. **Always run perft tests** after move generation changes
2. **Use the specialized agents** when you need domain expertise
3. **Document your journey** - this project emphasizes historical journaling
4. **Commit with bench count** - MUST include "bench <node-count>" for OpenBench
5. **Check project_status.md** before starting new work
6. **One feature at a time** - resist the temptation to implement multiple things
7. **Use branch naming convention** - feature/YYYYMMDD-name for readable OpenBench tests

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

## Stage 14 Debugging Lessons (CRITICAL)

**The 4-Hour Mystery That Wasn't:**
When performance regresses after "minor" changes, check these FIRST:

1. **Binary Size Changed?** 
   - If binary size differs significantly (e.g., 384KB vs 411KB), features may be missing
   - Always compare MD5 checksums of "identical" binaries
   - Stage 14: 27KB difference = entire quiescence search missing

2. **Build System Actually Rebuilding?**
   - CMake can cache aggressively - binaries may not change despite source edits
   - Always use `make clean` when debugging mysterious behavior
   - If multiple builds have identical sizes/checksums, the build is broken

3. **Check for Missing Compiler Flags:**
   ```bash
   # Quick check for what's actually defined:
   echo | g++ -dM -E - | grep ENABLE
   # Check binary symbols:
   nm binary | grep function_name
   ```

4. **Preserve Working Binaries:**
   - NEVER delete a working binary without backup
   - Always record: size, MD5, build flags, commit hash
   - Stage 14: Golden binary saved us from losing +300 ELO

5. **Ifdef Pattern is DANGEROUS:**
   - Core features behind #ifdef can silently disappear
   - Solution: Always compile features in, use runtime flags
   - We lost 4 hours because `ENABLE_QUIESCENCE` was never defined

## Questions to Ask When Stuck

1. What does the Master Project Plan say about this stage?
2. Have I checked the Chess Programming Wiki?
3. Would the chess-engine-expert agent have insights?
4. Are my perft numbers wrong? (Check divide command)
5. Am I trying to do too much at once?
6. **Is the binary size what I expect?** (New after Stage 14)
7. **Did the build actually rebuild?** (Check timestamps/checksums)

## Next Immediate Steps

1. Create `src/core/types.h` with basic chess types
2. Implement board representation structure
3. Set up bitboard utilities
4. Create unit tests for board operations
5. Validate with simple position setup/display

Remember: SeaJay's strength will come from methodical, validated development. Each phase builds on rock-solid foundations from the previous phase.