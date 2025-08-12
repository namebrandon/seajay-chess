# Stage 10: Git Strategy for Magic Bitboards Implementation

**Parent Document:** [`stage10_implementation_steps.md`](./stage10_implementation_steps.md) (MASTER)  
**Purpose:** Git workflow and version control strategy for Stage 10  
**Integration:** Commit after each step, tag after each phase  

## Recommended Approach: Single Branch with Tagged Checkpoints

We'll use your existing `stage-10-magic-bitboards` branch with **frequent commits** and **tags at major validation points**.

## Why This Strategy?

1. **Simple** - One branch to manage, no complex merging
2. **Safe** - Can rollback to any checkpoint via tags
3. **Debuggable** - Git bisect works perfectly with atomic commits
4. **Clear History** - Each commit maps to one implementation step

## Commit Strategy

### Commit After Every Step
```bash
# After completing Step 0A
git add -A
git commit -m "feat(magic): Add MagicValidator class infrastructure

- Implement slowRookAttacks() wrapper
- Implement slowBishopAttacks() wrapper  
- Add symmetry test function
- Validates successfully with ray-based implementation"

# After completing Step 0B
git add -A
git commit -m "feat(magic): Create test infrastructure

- Add critical test positions from expert review
- Set up DEBUG_MAGIC compile flag
- Create trace generation functions
- All tests compile and run"
```

### Commit Message Format
```
type(scope): Brief description

- Bullet point of what was done
- Another change
- Validation result
```

Types:
- `feat`: New functionality
- `fix`: Bug fix
- `test`: Test additions
- `perf`: Performance improvement
- `docs`: Documentation

## Tagging Strategy

### Create Tags at Major Validation Gates

```bash
# After Phase 0 complete (validation framework ready)
git tag -a "magic-phase0-complete" -m "Validation framework complete and tested"

# After Phase 1 complete (masks and magic numbers validated)
git tag -a "magic-phase1-complete" -m "All magic numbers validated, no collisions"

# After Phase 2 complete (attack tables generated)
git tag -a "magic-phase2-complete" -m "All 294,912 attack patterns validated"

# After Phase 3 complete (integration done)
git tag -a "magic-phase3-complete" -m "Perft matches 99.974%, integration successful"

# After Phase 4 complete (performance validated)
git tag -a "magic-phase4-complete" -m "3.5x speedup achieved, 100 games validated"

# After Phase 5 complete (ready for merge)
git tag -a "magic-phase5-complete" -m "1000 games validated, ready for production"
```

## Safety Checkpoints

### Before Each Phase
```bash
# Create a safety tag before starting risky work
git tag -a "before-magic-integration" -m "Last known good state before integration"
```

### If Something Goes Wrong
```bash
# View all tags
git tag -l "magic-*"

# Rollback to a checkpoint
git reset --hard magic-phase2-complete

# Or create a new branch from a tag to try different approach
git checkout -b magic-bitboards-attempt2 magic-phase1-complete
```

## Daily Backup Strategy

### End of Each Day
```bash
# Push your branch and tags to remote
git push origin stage-10-magic-bitboards
git push origin --tags

# Create a daily backup tag
git tag -a "magic-day2-end" -m "Day 2 complete: Masks and magic numbers done"
```

## Git Bisect Strategy (If Bugs Appear)

```bash
# If perft breaks somewhere in Phase 3
git bisect start
git bisect bad                    # Current commit is bad
git bisect good magic-phase2-complete  # This tag was good

# Git will checkout commits for you to test
./build/seajay perft
# Based on result:
git bisect good  # or
git bisect bad

# Git will find the exact commit that introduced the bug
```

## Branch Protection Rules

### DON'T:
- Force push (`git push --force`) - You'll lose history
- Rebase - Keep the linear history for debugging
- Squash commits - We want the granular history

### DO:
- Commit frequently (after each step)
- Write descriptive commit messages
- Tag major milestones
- Push regularly to backup

## Implementation Workflow

```bash
# Start of day
git checkout stage-10-magic-bitboards
git pull origin stage-10-magic-bitboards  # If working on multiple machines

# After each step (example: Step 1A)
git add -A
git status  # Review changes
git diff --staged  # Review actual code changes
git commit -m "feat(magic): Implement blocker mask generation

- Add computeRookMask() with edge square exclusion
- Add computeBishopMask() with proper edge handling
- Verify bit counts: 12 for rook, 9 for bishop
- All 64 squares validated"

# After each phase
git tag -a "magic-phase1-complete" -m "Descriptive message"
git push origin stage-10-magic-bitboards --tags

# End of day
git log --oneline -10  # Review today's progress
git push origin stage-10-magic-bitboards
```

## Emergency Recovery

### If You Accidentally Break Something
```bash
# See what you changed
git status
git diff

# Undo uncommitted changes
git restore .

# Undo last commit but keep changes
git reset --soft HEAD~1

# Nuclear option - go back to last tag
git reset --hard magic-phase2-complete
```

### If Tests Start Failing
```bash
# Find when tests last passed
git log --oneline --grep="test.*pass"

# Checkout that commit temporarily
git checkout <commit-hash>
# Run tests to confirm they pass
# Then return to your branch
git checkout stage-10-magic-bitboards
```

## Merge Strategy (After Phase 5)

```bash
# Only after 1000+ games validated
git checkout main
git merge stage-10-magic-bitboards --no-ff -m "feat: Implement magic bitboards for sliding pieces

- 3.5x speedup in move generation  
- Maintains 99.974% perft accuracy
- 1000+ games validated without issues
- Memory usage: 2.3MB for lookup tables"

# Tag the release
git tag -a "v2.0.0-magic" -m "Version 2.0.0: Magic bitboards integrated"
```

## Summary

This strategy gives you:
1. **Granular history** - Every step is recorded
2. **Safe checkpoints** - Can rollback via tags
3. **Easy debugging** - Git bisect works perfectly
4. **Simple workflow** - Just commit and tag
5. **Full backup** - Push regularly to remote

The key is: **Commit early, commit often, tag milestones**

This approach has saved me many times when a subtle bug appeared 50 commits later - git bisect found it in minutes rather than hours of debugging.