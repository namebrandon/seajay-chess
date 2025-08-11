# Stage 9b Git Strategy and Tracking

## Current Situation (August 11, 2025)

We're deep in debugging Stage 9b performance regression (-70 Elo loss). Multiple attempts and fixes have been tried. We need clear git organization to:
1. Track what we've tried
2. Return to known points
3. Document why each change was made
4. Keep our work reversible

## Key Reference Points

### Stable Commits
```bash
# Stage 9 Completion (BASELINE - KNOWN GOOD)
git checkout fe33035  # Stage 9 - PST only, no draws, no regression

# Stage 9b Original (KNOWN BAD - -70 ELO) 
git checkout 136d4aa  # Stage 9b - Draw detection with vector ops issue
```

### Current Work
- Uncommitted changes: Vector operations fix (m_inSearch flag)
- Status: Testing shows it may not fully resolve the issue

## Proposed Git Branch Strategy

### 1. Create Debugging Branches
```bash
# Create a branch for our current vector ops fix attempt
git stash
git checkout -b stage9b-debug/vector-ops-fix
git stash pop
git add -A
git commit -m "fix(stage9b): Add m_inSearch flag to skip vector ops in search

- Added m_inSearch boolean to Board class
- Skip pushGameHistory() during search
- Skip pop_back() during unmake in search  
- Draw detection still works via SearchInfo
- SPRT Result: [TO BE ADDED]"

# Create baseline branches for easy return
git checkout fe33035
git checkout -b stage9-baseline

git checkout 136d4aa  
git checkout -b stage9b-original
```

### 2. Naming Convention for Debug Branches
```
stage9b-debug/[approach-name]
```

Examples:
- `stage9b-debug/vector-ops-fix`
- `stage9b-debug/remove-draw-detection`
- `stage9b-debug/profile-based-fix`
- `stage9b-debug/complete-refactor`

### 3. Tag Important Points
```bash
# Tag the baseline
git tag -a stage9-complete -m "Stage 9 Complete - Baseline for regression testing"

# Tag the regression
git tag -a stage9b-regression -m "Stage 9b with -70 Elo regression"

# Tag successful fixes
git tag -a stage9b-fix-v1 -m "First attempt at fixing regression"
```

## Documentation Strategy

### 1. Create Performance Log
```markdown
# /workspace/project_docs/tracking/stage9b_performance_log.md

## Performance Regression Investigation Log

### Attempt 1: Vector Operations Fix (Aug 11, 2025)
- **Branch:** stage9b-debug/vector-ops-fix
- **Hypothesis:** Vector push/pop in hot path causing regression
- **Changes:** Added m_inSearch flag to skip history updates
- **Result:** [PENDING SPRT]
- **Elo Impact:** [TBD]
- **Conclusion:** [TBD]

### Attempt 2: [Next approach]
...
```

### 2. Keep SPRT Results Organized
```bash
/workspace/sprt_results/
├── SPRT-2025-009-STAGE9B-FIXED/     # Vector ops fix attempt
├── SPRT-2025-010-STAGE9B-[NEXT]/    # Next attempt
└── README.md                         # Summary of all tests
```

### 3. Document Each Fix Attempt
Before each commit:
```bash
# Create a detailed commit message
git commit -m "fix(stage9b): [Brief description]

Problem: [What issue we're addressing]
Solution: [What we changed]
Testing: [How we validated]
Result: [SPRT outcome if available]
Next: [What to try if this doesn't work]

Related: #stage9b-regression"
```

## Recovery Points

### Quick Return Commands
```bash
# Return to Stage 9 (known good)
git checkout stage9-baseline

# Return to Stage 9b original (with regression)
git checkout stage9b-original

# See all debugging attempts
git branch | grep stage9b-debug

# Compare current with baseline
git diff stage9-baseline
```

## Workflow for Each Debug Attempt

1. **Create branch from last attempt**
   ```bash
   git checkout -b stage9b-debug/new-approach
   ```

2. **Make changes and test locally**
   ```bash
   # Quick performance test
   ./build/seajay bench 5
   ```

3. **Run SPRT test**
   ```bash
   ./run_stage9b_fixed_sprt.sh
   ```

4. **Document results**
   - Update performance_log.md
   - Save SPRT results
   - Commit with detailed message

5. **Decide next step**
   - If successful: Merge to main
   - If failed: Document learnings, try next approach

## Important Files to Track

### Core Files (Most Likely Issues)
- `src/core/board.cpp` - makeMove/unmakeMove
- `src/core/board.h` - Board class structure
- `src/search/negamax.cpp` - Search implementation
- `src/search/search_info.h` - Search-specific data

### Test Files
- `stage9b_regression_analysis.md` - Expert analysis
- `project_docs/tracking/stage9b_performance_log.md` - Our attempts
- `project_docs/tracking/deferred_items_tracker.md` - Future work

## Emergency Recovery

If everything gets messy:
```bash
# Save current work
git stash save "Emergency save - $(date)"

# Return to last known good
git checkout fe33035

# Start fresh
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

# Verify it works
./build/seajay bench 5
```

## Next Steps

1. **Commit current vector ops fix** to `stage9b-debug/vector-ops-fix`
2. **Wait for SPRT results**
3. **If failed:** Create new branch for next approach
4. **Document everything** in performance log

## Remember

- Every attempt teaches us something
- Document failures as thoroughly as successes  
- Keep branches until we're sure we don't need them
- Tag successful fixes immediately
- Update this strategy doc as we learn more