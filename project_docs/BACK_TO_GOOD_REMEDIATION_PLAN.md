# BACK TO GOOD - POST STAGE 15 REMEDIATION PLAN

## Executive Summary

SeaJay Chess Engine has reached Stage 15 with accumulated technical debt from rapid development. Multiple features are locked behind compile-time flags, build scripts have proliferated, and implementation may have deviated from original plans. This remediation plan provides a systematic approach to audit, fix, and validate EACH stage's implementation individually.

## Core Principles

### 0. PRIME DIRECTIVE: NO MERGE WITHOUT APPROVAL
**⚠️ ONLY THE PROJECT OWNER CAN APPROVE MERGES TO MAIN ⚠️**
- Each remediation MUST complete OpenBench SPRT testing
- Results MUST be reviewed by human
- Explicit approval MUST be received before merge
- This is non-negotiable

### 1. NO COMPILE-TIME FEATURE FLAGS
**All features MUST compile in. Runtime control via UCI options only.**

Examples of correct implementation:
```cpp
// WRONG - Compile-time
#ifdef USE_MAGIC_BITBOARDS
    return magicRookAttacks(sq, occupied);
#else
    return rayRookAttacks(sq, occupied);
#endif

// CORRECT - Runtime UCI option
if (m_useMagicBitboards) {
    return magicRookAttacks(sq, occupied);
} else {
    return rayRookAttacks(sq, occupied);
}
```

### 2. Each Stage is Independent
- Each stage remediation is a separate, complete process
- Start each stage manually after previous is merged to main
- Do NOT automatically proceed to next stage
- Each stage gets its own branch, validation, and merge

### 3. Comprehensive Audit Required
- The issues we know about are just the STARTING POINT
- Must do full investigation of each stage
- May find additional problems not yet identified
- Implementation may deviate significantly from original plan

## Known Issues (Starting Points Only)

### Compile-Time Feature Flags
- **Stage 10**: `USE_MAGIC_BITBOARDS` (currently OFF by default!)
- **Stage 11**: `ENABLE_MVV_LVA` in negamax.cpp and quiescence
- **Stage 14**: `QSEARCH_TESTING`, `QSEARCH_TUNING` build modes
- **Result**: Cannot test features without recompilation, OpenBench cannot A/B test

### Build System Chaos
- 5 different build scripts: `build.sh`, `build_production.sh`, `build_testing.sh`, `build_tuning.sh`, `build_debug.sh`
- Each creates different binaries with different compile flags
- Makefiles for OpenBench add another layer of complexity

### Undocumented Utilities
- 60+ test utilities and demos in `bin/` directory
- Purpose and usage unclear for many
- No documentation on which are relevant to which stage

## Per-Stage Remediation Process

**IMPORTANT**: This process is repeated for EACH stage (10-15) as a separate, manually-initiated effort.

### Phase 1: Create Branch

**Use Git Branch Strategy for Naming:**
```bash
git checkout main
git pull origin main

# For bug fixes (remediation is fixing bugs)
git bugfix stage[N]-remediation
# Example: git bugfix stage10-magic-bitboards
# Creates: bugfix/20250819-stage10-magic-bitboards

# Alternative: Use remediate/ prefix if preferred
git checkout -b remediate/stage[N]-description
```

### Phase 2: Comprehensive Audit

#### A. Review Original Requirements
1. Read `/workspace/project_docs/planning/stage[N]_*.md`
2. Read Master Project Plan section for this stage
3. Read any pre-stage planning documents
4. Read implementation notes in `/workspace/project_docs/stage_implementations/stage[N]_*.md`
5. Note ALL expected features and behaviors

#### B. Deep Implementation Analysis
1. **Known Issues** (starting point only):
   - Compile-time flags we've identified
   - Build mode issues
   
2. **Additional Investigation** (CRITICAL):
   - What else might be wrong?
   - Is the algorithm correctly implemented?
   - Are there bugs in the implementation?
   - Does it match the original specification?
   - Are there performance issues?
   - Are there missing features?
   - Are there incorrect optimizations?
   - Check deferred items tracker for this stage

3. **Code Review**:
   - Read ALL code related to this stage
   - Check for TODO/FIXME comments
   - Look for disabled code
   - Check for hardcoded values that should be configurable
   - Verify algorithm correctness against chess programming wiki
   - Compare with standard implementations (Stockfish, etc.)

4. **Test Coverage**:
   - What tests exist?
   - Do they actually test the feature?
   - Are they passing for the right reasons?
   - What edge cases are missing?

#### C. Document ALL Findings
Create `/workspace/project_docs/remediation/stage[N]_audit.md`:
```markdown
# Stage [N] Remediation Audit

## Original Requirements
[From planning docs]

## Known Issues Going In
- [Issues we already identified]

## Additional Issues Found
- [New problems discovered during audit]

## Implementation Deviations
- [Where implementation differs from plan]

## Missing Features
- [What wasn't implemented]

## Incorrect Implementations
- [What was implemented wrong]

## Testing Gaps
- [What isn't properly tested]

## Utilities Related to This Stage
- [List all utilities and their purpose]
```

### Phase 3: Fix Implementation

1. **Convert Compile Flags to UCI Options**
   - Add UCI option declaration in `handleUCI()`
   - Add member variable to UCIEngine class
   - Add handler in `handleSetOption()`
   - Pass option through to relevant code
   - Default should typically enable the feature

2. **Fix Algorithm Issues**
   - Correct any implementation errors
   - Add missing functionality
   - Fix performance problems

3. **Remove Build Variants**
   - Eliminate stage-specific build scripts
   - Remove compile-time conditionals
   - Consolidate to single binary

4. **Add/Fix Tests**
   - Ensure comprehensive test coverage
   - Fix tests that were passing incorrectly
   - Add edge case tests

5. **Document Changes**
   - Update code comments
   - Document UCI options
   - Note any behavioral changes

### Phase 4: Validate

#### A. Functional Validation
1. **Perft Tests** (if move generation related)
   ```bash
   ./bin/seajay perft
   ```

2. **Unit Tests** for the feature
   ```bash
   ./run_test_suite.sh
   ```

3. **Integration Tests** with other features

4. **Benchmark Tests** for performance
   ```bash
   ./bin/seajay bench
   ```

#### B. OpenBench Validation (CRITICAL)

**IMPORTANT**: Ensure Makefile exists in root directory!
```bash
# Check if Makefile exists (OpenBench requirement)
ls -la Makefile
# If missing, copy from an OpenBench branch:
# git show openbench/stage15:Makefile > Makefile

# Test that Makefile still works
make clean
make EXE=seajay
./seajay bench

# Verify bench command works from command line
./bin/seajay bench

# Should output: "info string Benchmark complete: XXXXXX nodes, XXXXX nps"

# Ensure only engine is built, not test utilities
ls -la  # Should only show seajay binary, not test programs
```

#### C. UCI Option Validation
```bash
# Test that new UCI options work
echo -e "uci\nsetoption name [OptionName] value [true/false]\nisready\nquit" | ./bin/seajay

# Verify option changes behavior
# Test with option ON and OFF
```

#### D. SPRT Testing
- Feature ON vs OFF (if applicable)
- Compare to pre-remediation baseline
- Use `openbench/stage15-pre-remediation` as reference
- Document results in SPRT log

### Phase 5: Documentation Update

1. **Update `/workspace/CLAUDE.md`**
   - Add new UCI options
   - Document their effects
   - Note default values

2. **Document Utilities**
   ```markdown
   ### Utility: [name]
   **Stage**: [Which stage introduced this]
   **Purpose**: [What it does]
   **Usage**: ./bin/[name] [arguments]
   **Example**: [Example command]
   **Notes**: [Any special considerations]
   ```

3. **Update Remediation Audit**
   - Add resolution for each issue found
   - Note what was changed and why

4. **Create Stage Completion Report**
   `/workspace/project_docs/remediation/stage[N]_complete.md`

5. **Update OpenBench Testing Index**
   - Add entry to Remediation Quick Reference Table
   - Document branch, commit, and bench node count
   - Add remediation testing examples

### Phase 6: Final Verification Checklist

Before proceeding to OpenBench testing:
- [ ] **Makefile exists in root directory** (critical for OpenBench!)
- [ ] All compile flags removed for this stage
- [ ] All features work via UCI options
- [ ] Default values are sensible (usually feature ON)
- [ ] OpenBench Makefile works: `make clean && make EXE=test`
- [ ] `./seajay bench` works correctly
- [ ] Bench output format correct for OpenBench
- [ ] Get bench node count: `echo "bench" | ./seajay | grep "Benchmark complete"`
- [ ] No test utilities compiled by Makefile
- [ ] All relevant tests pass
- [ ] Performance comparison documented (feature ON vs OFF)
- [ ] Documentation updated (including OpenBench Testing Index)
- [ ] No new compile flags introduced
- [ ] Code follows project style guidelines
- [ ] UCI name updated to reflect remediation

### Phase 7: Commit and Version Management

#### Commit Format for OpenBench

**CRITICAL: Every commit MUST include "bench <node-count>" for OpenBench compatibility**

```bash
# Build final binary and get bench count
make clean && make -j4 seajay
BENCH_COUNT=$(echo "bench" | ./seajay 2>&1 | grep "Benchmark complete" | awk '{print $4}')

# Commit with MANDATORY OpenBench format
git commit -m "Stage [N] Remediation: [Brief description]

bench ${BENCH_COUNT}

[Detailed description of changes]

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>"
```

**Note the format:** "bench <node-count>" with no colon, no uppercase. This exact format is required.

#### UCI Version Management
Update UCI engine name to include remediation info:
- Option 1: `SeaJay Stage[N]-Remediated-[short_hash]`
- Option 2: `SeaJay 0.[N].1-remediated` (semantic versioning)
- Consider using `src/version.h` for consistent versioning

### Phase 8: OpenBench Testing (MANDATORY)

**⚠️ CRITICAL: NO MERGE TO MAIN WITHOUT HUMAN APPROVAL ⚠️**

#### OpenBench SPRT Test Requirements
1. **Test Against Previous Remediation**:
   - Base: `openbench/remediated-stage[N-1]` (or stage15-pre-remediation for Stage 10)
   - Dev: Current remediation branch
   - Time Control: 10+0.1 minimum
   - Book: Standard opening book
   - Expected: No regression (minimum)

2. **Wait for Results**:
   - SPRT tests can take hours to days
   - Must pass statistical significance
   - Document ELO gain/loss
   - Save test URL for reference

3. **Human Approval Required**:
   - Only the project owner can approve merge to main
   - Provide test results and summary
   - Wait for explicit approval before proceeding

### Phase 9: Create Reference Branches and Merge

#### Create OpenBench Reference Branch (CRITICAL)
```bash
# Create permanent reference branch for future testing
# Using ob/ prefix for historical references per Git strategy
git ob remediated-stage[N]
# Creates: ob/20250819-remediated-stage[N]

# Alternative: Keep using openbench/ prefix for clarity
git checkout -b openbench/remediated-stage[N]
git push origin openbench/remediated-stage[N]

# Document the commit hash and bench
echo "Stage [N] Reference: $(git rev-parse HEAD)" >> remediation_log.txt
echo "Bench: ${BENCH_COUNT}" >> remediation_log.txt
```

#### Tag the Release
```bash
# Create annotated tag for version tracking
git tag -a "v0.[N].1-remediated" -m "Stage [N] Remediation: [Description]
Bench: ${BENCH_COUNT}
Commit: $(git rev-parse HEAD)
ELO Gain: [+X over previous]"

git push origin --tags
```

#### Merge to Main (ONLY WITH HUMAN APPROVAL)
```bash
# ⚠️ STOP - DO NOT PROCEED WITHOUT EXPLICIT HUMAN APPROVAL ⚠️
# Required before merge:
# 1. OpenBench SPRT test completed and passed
# 2. Human has reviewed results
# 3. Human has given explicit approval to merge

# After receiving approval:
git checkout main
git pull origin main
git merge remediate/stage[N]-description
git push origin main

# The working branch can now be deleted locally
git branch -d remediate/stage[N]-description
# But keep the openbench/remediated-stage[N] branch!
```

## Stage-Specific Investigation Areas

### Stage 10: Magic Bitboards ✅ REMEDIATED
**Known Issues**:
- `USE_MAGIC_BITBOARDS` compile flag (OFF by default!) ✅ FIXED

**Investigation Results**:
- Magic numbers correct (from Stockfish, properly attributed) ✅
- Lookup tables properly initialized ✅
- Performance gain: 79x for move generation, 1.57x overall ✅
- Disabled by default: Conservative development approach ✅
- Three implementations found (v1, v2, simple) - kept v2 ✅
- Implementation perfect, just needed runtime control ✅

**Lessons Learned**:
- Working features can be inadvertently disabled
- Multiple implementations accumulate as technical debt
- Always check binary size/performance when "minor" changes break things

### Stage 11: MVV-LVA Move Ordering
**Known Issues**:
- `ENABLE_MVV_LVA` compile flag

**Investigation Areas**:
- Is the scoring formula correct?
- Is it being applied in all search functions?
- Is it applied in quiescence search?
- Are special moves (promotions, en passant) handled correctly?
- Is the sort stable?

### Stage 12: Transposition Tables
**Known Issues**:
- None identified yet (may be clean)

**Investigation Areas**:
- Is the replacement scheme correct?
- Are mate scores adjusted properly for ply?
- Is zobrist hashing working correctly?
- Are collisions handled properly?
- Is the size configurable via UCI?
- Memory management correct?

### Stage 13: Iterative Deepening
**Known Issues**:
- None identified yet

**Investigation Areas**:
- Is aspiration window working?
- Should aspiration windows be UCI configurable?
- Is time management correct?
- Is move ordering from previous iteration used?
- Is EBF tracking accurate?
- Principal variation handling?

### Stage 14: Quiescence Search
**Known Issues**:
- Build modes (TESTING/TUNING/PRODUCTION)
- Node limit compile-time constants

**Investigation Areas**:
- Is stand pat implemented correctly?
- Is delta pruning working?
- Are checks in quiescence handled properly?
- Is the depth limit appropriate?
- Should node limits be UCI configurable?
- MVV-LVA in quiescence?

### Stage 15: Static Exchange Evaluation (SEE)
**Known Issues**:
- May already be properly done with UCI options

**Investigation Areas**:
- Is X-ray attack working?
- Is the SEE calculation accurate?
- Is pruning threshold appropriate?
- Integration with move ordering correct?
- Performance impact acceptable?

## OpenBench Compatibility Requirements

Must verify for EACH stage:

1. **Makefile Functionality**:
   ```bash
   make clean && make EXE=test
   ./test bench
   ```

2. **Command-Line Bench**:
   ```bash
   ./bin/seajay bench
   # Must output: "info string Benchmark complete: XXXXXX nodes, XXXXX nps"
   ```

3. **No Extra Binaries**:
   - Makefile should ONLY build seajay engine
   - Should NOT build test utilities
   - Use `BUILD_TESTING=OFF` in Makefile if needed

4. **Bench Output Format**:
   - Must include "info string" prefix
   - Must include node count
   - Must include NPS
   - Format must be parseable by OpenBench

## Build System Cleanup (After All Stages)

This is a SEPARATE effort after all stage remediations:

### Target State:
```bash
# Single build script
./build.sh [debug|release]  # Default: release

# CMakeLists.txt - No feature flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -fsanitize=address)
else()
    add_compile_options(-O3 -march=native -DNDEBUG)
endif()
```

### Remove:
- build_production.sh
- build_testing.sh  
- build_tuning.sh
- All QSEARCH_MODE logic
- All USE_* feature flags
- All ENABLE_* feature flags

## Success Criteria for EACH Stage

1. **Complete Audit** - All issues found, not just known ones
2. **All Issues Fixed** - Not just compile flags
3. **Algorithm Correct** - Matches specification
4. **OpenBench Compatible** - Makefile and bench work
5. **Tests Pass** - All relevant tests
6. **OpenBench SPRT Testing** - Completed with no regression
7. **Human Approval** - Explicit approval received
8. **Documented** - All changes and findings recorded
9. **Clean Code** - No compile-time feature flags
10. **Merged to Main** - Only after approval

## Reference Baseline

- **Branch**: `openbench/stage15-pre-remediation`
- **Purpose**: Baseline for SPRT testing improvements
- **Bench**: Will be recorded in commit message
- All remediation improvements tested against this baseline

## Important Reminders

- **DO NOT** automatically continue to next stage
- **DO NOT** assume known issues are the only issues  
- **DO NOT** skip OpenBench SPRT testing
- **DO NOT** merge to main without human approval
- **DO NOT** merge without completed SPRT test
- **DO NOT** introduce new compile-time flags
- **DO NOT** delete OpenBench reference branches
- **ALWAYS** start fresh from main for each stage
- **ALWAYS** document everything found
- **ALWAYS** test OpenBench compatibility
- **ALWAYS** verify UCI options work correctly
- **ALWAYS** clean up test files before committing
- **ALWAYS** get bench node count for OpenBench format
- **ALWAYS** update OpenBench Testing Index with results
- **ALWAYS** create openbench/remediated-stage[N] before merging
- **ALWAYS** tag the release with version number

## Quick Remediation Checklist Template

```markdown
# Stage [N] Remediation Checklist

## Phase 1: Setup
- [ ] Create branch: `git checkout -b remediate/stage[N]-description`
- [ ] Create audit file: `/workspace/project_docs/remediation/stage[N]_audit.md`

## Phase 2: Audit
- [ ] Review original planning docs
- [ ] Check for compile-time flags
- [ ] Search for multiple implementations
- [ ] Test current behavior
- [ ] Document all findings

## Phase 3: Implementation
- [ ] Create/update engine_config.h if needed
- [ ] Convert compile flags to runtime
- [ ] Add UCI option(s)
- [ ] Update all callers
- [ ] Remove dead code
- [ ] Update CMakeLists.txt

## Phase 4: Validation
- [ ] Build clean: `make clean && make -j4 seajay`
- [ ] Test UCI option works
- [ ] Run perft tests
- [ ] Get bench count: `echo "bench" | ./seajay | grep "complete"`
- [ ] Compare performance (ON vs OFF)
- [ ] Test in OpenBench against previous remediation

## Phase 5: Documentation
- [ ] Create completion report: `stage[N]_complete.md`
- [ ] Update OpenBench Testing Index
- [ ] Update CLAUDE.md if needed

## Phase 6: Commit & Version
- [ ] Ensure Makefile is included if it was missing
- [ ] Stage all changes
- [ ] Commit with bench: [count] format
- [ ] Update UCI name with version
- [ ] Push to GitHub
- [ ] Note full SHA for OpenBench Testing Index

## Phase 7: OpenBench Testing
- [ ] Submit SPRT test to OpenBench
- [ ] Wait for test completion
- [ ] Document results and ELO gain
- [ ] Request human approval for merge

## Phase 8: Create References & Merge (After Approval)
- [ ] Receive explicit human approval
- [ ] Create OpenBench reference branch: `openbench/remediated-stage[N]`
- [ ] Push reference branch to origin
- [ ] Create version tag: `v0.[N].1-remediated`
- [ ] Push tags to origin
- [ ] Merge to main (only with approval)
- [ ] Update documentation with ELO gain
```

## Timeline Estimate

Per Stage:
- Audit: 1-2 hours
- Implementation: 2-4 hours  
- Validation: 1-2 hours
- Documentation: 30 minutes

Total per stage: 4-8 hours depending on issues found

## Priority Order

1. **Stage 10** (Magic Bitboards) - Currently OFF, major performance impact
2. **Stage 11** (MVV-LVA) - Core ordering feature disabled
3. **Stage 14** (Quiescence) - Build modes preventing testing
4. **Stage 12** (Transposition Tables) - Verify correct
5. **Stage 13** (Iterative Deepening) - Verify correct
6. **Stage 15** (SEE) - May already be correct

## Procedural Lessons from Stage 10 Remediation

### What Worked Well
1. **Comprehensive Audit First** - Understanding the full scope before fixing
2. **Chess Engine Expert Consultation** - Validated implementation correctness
3. **Runtime Configuration System** - `engine_config.h` singleton pattern
4. **Performance Validation** - Comparing ON vs OFF to verify improvement
5. **Documentation Throughout** - Audit report, completion report, index update

### Common Patterns to Expect
1. **Multiple Implementations** - Dead code from development iterations
2. **Include Path Issues** - May need to update multiple files for wrapper functions
3. **Build System Complexity** - Remove flags from CMakeLists.txt carefully
4. **UCI Integration** - Follow existing patterns in handleSetOption()
5. **Bench Testing** - Use `echo "bench" | ./seajay` for reliable results
6. **Missing Makefile** - Remediation branches created from main may lack Makefile

### Implementation Strategy
1. **Create Global Config** - Singleton pattern for runtime flags
2. **Update Wrappers** - Convert #ifdef to if statements
3. **Fix All Callers** - Search for direct function calls to update
4. **Add UCI Option** - Declaration in handleUCI(), handler in handleSetOption()
5. **Clean Build System** - Remove compile flags, keep debug options

### Version Management Strategy
1. **Commit First** - Get the working changes committed
2. **Update UCI Name** - Include stage and commit info
3. **Amend if Needed** - Update with bench count and final name
4. **Document in Index** - Add to remediation table immediately

## Conclusion

This remediation is critical for SeaJay's development. Each stage must be thoroughly audited, fixed, and validated as an independent effort. The goal is a clean, professional codebase where all features are runtime-controllable via UCI options and properly documented.

Remember the Stage 14 lesson: **"NO COMPILE-TIME FEATURE FLAGS"** - All features MUST compile in, with runtime control via UCI options.

## Critical OpenBench Notes

### Makefile Requirement
OpenBench uses `make` command, NOT CMake directly. Remediation branches created from main may be missing the Makefile since we use CMake locally. Always ensure:

1. **Check for Makefile**: `ls -la Makefile`
2. **If missing, copy from OpenBench branch**: 
   ```bash
   git show openbench/stage15:Makefile > Makefile
   ```
3. **Remove only compile-time flags** - Don't modify the Makefile structure
4. **Commit with bench value**: Include Makefile in remediation commits

### Command-Line Bench Requirement
OpenBench runs `./seajay bench` as a command-line argument, NOT through UCI:

1. **Main function must handle arguments**: `int main(int argc, char* argv[])`
2. **Check for bench argument**: 
   ```cpp
   if (argc > 1 && std::string(argv[1]) == "bench") {
       engine.runBenchmark();
       return 0;
   }
   ```
3. **Exit after benchmark**: Must return, not enter UCI loop
4. **Output format**: `info string Benchmark complete: [nodes] nodes, [nps] nps`

### Testing Configuration
When testing remediation in OpenBench:
- **Base**: Use Stage 15 historical (has all features but with compile flags)
- **Dev**: Use remediation commit (has UCI runtime options)
- **Full SHA required**: OpenBench needs complete commit hash, not short version

## Remediation Testing Strategy

### Branch Hierarchy
```
main (latest merged remediations)
├── openbench/remediated-stage10 (reference: 753da6d)
├── openbench/remediated-stage11 (future)
├── openbench/remediated-stage12 (future)
└── ...
```

### Testing Chain
- Stage 11 tests against: `openbench/remediated-stage10`
- Stage 12 tests against: `openbench/remediated-stage11`
- Each stage measures incremental improvement
- Reference branches are NEVER deleted

### OpenBench Configuration
```json
{
  "base_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "[previous remediation SHA]"
  },
  "dev_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "[current remediation SHA]"
  }
}
```

## Completed Remediations

- ✅ **Stage 10**: Magic Bitboards - Converted to UCI option, 79x speedup enabled
  - Working Branch: `remediate/stage10-magic-bitboards` (merged)
  - Reference Branch: `openbench/remediated-stage10`
  - Final Commit: `753da6dd7cb03cb1e5b7aa26f7dc5bc2f20b47a5`
  - Tag: `v0.10.1-remediated`
  - Bench: 19191913
  - ELO Gain: +35 over Stage 15 historical
  - Lessons: 
    - Makefile was missing (must copy from OpenBench branches)
    - Bench must work from command line, not just UCI
    - Need reference branches for future testing
    - **CRITICAL: All commits must include "bench <node-count>"**

- ✅ **Stage 11**: MVV-LVA - Fixed scoring bug, removed compile flag
  - Working Branch: `remediate/stage11-mvv-lva` (merged)
  - Reference Branch: `openbench/remediated-stage11`
  - Final Commit: `4d8d7965656502ff1e3f507a02392ff13e20d79c`
  - Tag: `v0.11.1-remediated`
  - Bench: 19191913
  - ELO Gain: Testing with manual bench override (OpenBench limitation)
  - Key Fixes:
    - Fixed critical scoring bug (dual system with wrong values)
    - Removed ENABLE_MVV_LVA compile flag (always active now)
    - Separated from SEE (Stage 15)
    - Fixed sort stability

- ✅ **Stage 12**: Transposition Tables - Fixed and converted to UCI
  - Working Branch: `remediate/stage12-transposition-tables` (merged)
  - Reference Branch: `openbench/remediated-stage12`
  - Final Commit: `a0f514c70dc4f113b5f02e5962cf4e6f634c8493`
  - Tag: `v0.12.1-remediated`
  - Bench: 19191913
  - ELO Gain: +82.89 ± 11.83 (SPRT validated)

- ✅ **Stage 13**: Iterative Deepening - Enhanced with UCI options
  - Working Branch: `remediate/stage13-iterative-deepening` (merged)
  - Reference Branch: `openbench/remediated-stage13`
  - Final Commit: `b949c427e811bfb85a7318ca8a228494a47e1d38`
  - Tag: `v0.13.1-remediated`
  - Bench: 19191913
  - ELO Gain: +7.11 ± 11.34 (SPRT validated)

- ✅ **Stage 14**: Quiescence Search - Fixed regression
  - Working Branch: `remediate/stage14-quiescence` (merged)
  - Reference Branch: `openbench/remediated-stage14`
  - Final Commit: `e8fe7cfc9ce5943ae9a826bbb30e85f52f28afce`
  - Tag: `v0.14.1-remediated`
  - Bench: 19191913
  - ELO Gain: +5.55 ± 14.85 (SPRT validated)
  - Key Fixes:
    - Fixed static eval caching bug in Phase 3
    - Added TT depth==0 filtering
    - Removed Phase 3 optimizations that hurt performance