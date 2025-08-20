# Getting Documentation to Main While Preserving Feature Branch

**Date:** August 20, 2025  
**Scenario:** Working on feature/20250819-lmr, need to get documentation updates to main  
**Status:** Ready to execute  

## Professional Strategy

When you discover important documentation that needs to be on main while you're working on a feature branch, the professional approach is to use Git's cherry-pick functionality to move specific commits.

## Step-by-Step Process

### Step 1: Commit Documentation Changes on Feature Branch

```bash
# We're currently on feature/20250819-lmr
git status
# Make sure all documentation changes are staged

git add project_docs/Git_Strategy_for_SeaJay.txt
git add project_docs/integration_branches.md
git add project_docs/project_status.md
git add project_docs/tracking/deferred_items_tracker.md
git add project_docs/documentation_to_main_strategy.md

git commit -m "docs: Add integration branch strategy and LMR analysis

- Add integration/ branch category to Git strategy
- Create integration_branches.md tracker for complex features
- Document LMR sequencing issue and integration plan
- Update project status with Stage 18 findings
- Create documentation migration strategy guide

This documentation should be on main for team reference
while LMR implementation continues on feature branch.

bench 19191913"
```

### Step 2: Cherry-pick Documentation to Main

```bash
# Note the commit hash from the documentation commit
DOCS_COMMIT=$(git rev-parse HEAD)

# Switch to main
git checkout main

# Cherry-pick only the documentation commit
git cherry-pick $DOCS_COMMIT

# Push to main
git push origin main
```

### Step 3: Return to Feature Branch and Continue Work

```bash
# Return to feature branch
git checkout feature/20250819-lmr

# The feature branch still has all the LMR code changes
# The documentation is now also on main
# Continue working on LMR fixes
```

### Step 4: Handle Any Merge Conflicts Later

When you eventually merge the feature branch, Git will detect that the documentation commit is already on main and handle it intelligently.

## Alternative: Separate Documentation Commits

If the documentation is mixed with code changes, you can separate them:

### Option A: Interactive Rebase to Separate Commits

```bash
# On feature branch
git rebase -i HEAD~3  # Go back 3 commits to find where to split

# In the interactive editor, mark commits to 'edit'
# Then use git reset to unstage code changes, commit docs separately
```

### Option B: Create Documentation-Only Branch

```bash
# From feature branch
git checkout -b docs/integration-branch-strategy

# Reset to remove code changes, keep only docs
git reset --mixed main
git add project_docs/
git commit -m "docs: Add integration branch strategy and analysis"

# Cherry-pick this to main
git checkout main
git cherry-pick docs/integration-branch-strategy

# Return to feature branch for continued development
git checkout feature/20250819-lmr
```

## What We've Accomplished

### Documentation Now Ready for Main:

1. **Git_Strategy_for_SeaJay.txt** - Extended with integration/ branch support
2. **integration_branches.md** - New tracker for complex features
3. **project_status.md** - Updated with Stage 18 LMR analysis
4. **deferred_items_tracker.md** - LMR sequencing issue documented
5. **documentation_to_main_strategy.md** - This guide

### LMR Code Fixed and Ready for Testing:

1. **Re-search condition bug fixed** - Now checks `score > alpha && score < beta`
2. **Default parameters made conservative** - minMoveNumber=8, enabled=false
3. **Critical bug fixes preserved** - depthFactor=3, boolean parsing
4. **Safe for integration branch** - Disabled by default, won't affect gameplay

## Testing Strategy

### Phase 1: Test Current State (LMR Disabled)
```bash
# Should be neutral since LMREnabled=false by default
# This validates that the feature doesn't break anything when disabled
```

### Phase 2: Test Current State (LMR Enabled, Conservative)
```bash
# Set LMREnabled=true, minMoveNumber=8 
# Should not lose ELO with conservative parameters and fixed re-search bug
```

### Phase 3: Create Integration Branch (If Tests Pass)
```bash
git checkout -b integration/lmr-with-move-ordering
git push -u origin integration/lmr-with-move-ordering
# Document in integration_branches.md
```

### Phase 4: Implement Prerequisites on Main
```bash
git checkout main
git feature history-heuristic
# Implement, test, merge

git checkout main
git feature killer-moves
# Implement, test, merge
```

### Phase 5: Complete LMR Integration
```bash
git checkout integration/lmr-with-move-ordering
git rebase main  # Now has move ordering
# Enable LMR by default, test, should gain 50-100 ELO
```

## Professional Benefits

1. **Clean Separation**: Documentation separate from code changes
2. **Team Access**: Important docs available on main immediately  
3. **Preserved Work**: Feature branch maintains all implementation progress
4. **Clear History**: Git history shows logical progression
5. **Standard Practice**: Used by professional development teams

## Next Steps

1. Execute the cherry-pick strategy to get docs to main
2. Test LMR in current state (disabled, then enabled conservative)
3. Create integration branch if tests are acceptable
4. Begin prerequisite implementation on main
5. Complete LMR integration when dependencies ready

This approach ensures no work is lost while maintaining professional Git practices and keeping the team informed of architectural decisions.