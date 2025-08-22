# Feature Implementation Guidelines for SeaJay

## Core Development Philosophy

**STOP → TEST → PROCEED**

Every feature implementation follows a phased approach with mandatory testing gates between phases. No exceptions.

## Implementation Process

### 1. Pre-Implementation Planning
- [ ] Create feature implementation plan document
- [ ] Define clear phases with specific deliverables
- [ ] Set expected ELO ranges for each phase
- [ ] Identify risk factors and mitigation strategies
- [ ] Review plan with domain experts (chess-engine-expert agent)

### 2. Phase Structure

#### Phase Sizing
- **Maximum changes per phase:** 2-3 related modifications
- **Preferred:** Single atomic feature per phase
- **Testing requirement:** FULL STOP after each phase

#### Phase Naming Convention
```
Feature_Name_Phase_X.Y
Example: PP1, PP2, PP3a, PP3b
```

### 3. Development Workflow

#### For Each Phase:

1. **Implementation**
   ```bash
   # Start from clean baseline
   git checkout <base-commit>
   
   # Implement feature
   # Build and validate locally
   ./build.sh
   ```

2. **Local Testing**
   ```bash
   # Run benchmark
   echo "bench" | ./bin/seajay | grep "Benchmark complete"
   
   # Test specific positions if applicable
   # Validate with Stockfish when needed
   echo -e "position fen [FEN]\neval\nquit" | ./external/engines/stockfish/stockfish
   ```

3. **Commit Protocol**
   ```bash
   # MANDATORY: Include bench count in commit message
   git add -A
   git commit -m "feat: [Phase ID] - Brief description
   
   Detailed explanation of changes...
   
   bench [node-count]"
   ```

4. **Push to Remote**
   ```bash
   # CRITICAL: Push immediately for OpenBench access
   git push origin <branch-name>
   ```

5. **Report Completion**
   ```
   === PHASE [X] COMPLETE ===
   Branch: <branch-name>
   Commit: <full-sha>
   Bench: <node-count>
   Expected ELO: <range>
   UCI Options: <if any>
   Ready for OpenBench: YES
   
   AWAITING HUMAN TEST CONFIRMATION
   Do not proceed to Phase [X+1] until test results are reviewed.
   ```

### 4. Testing Protocol

#### OpenBench Configuration
```
Dev Branch: <feature-branch>
Base Branch: main (or previous phase)
Time Control: 10+0.1 (standard) or 60+0.6 (complex features)
Test Bounds: [lower, upper] based on expectations
Book: UHO_4060_v2.epd
Description: Phase XX - <brief description>
```

#### Test Interpretation
- **Within expected range:** Proceed to next phase
- **Below expectations but positive:** Analyze and decide
- **Negative result:** STOP and debug
- **Catastrophic failure (>50 ELO loss):** Revert and redesign

### 5. Debugging Protocol

When a phase fails:

1. **Incremental Decomposition**
   - Break failing phase into smaller sub-phases
   - Test each component individually
   - Example: PP3 → PP3a, PP3b, PP3c...

2. **Binary Search Method**
   - Comment out half the changes
   - Test to identify problematic half
   - Repeat until issue isolated

3. **Validation Tools**
   ```bash
   # Compare evaluation with Stockfish
   echo -e "position fen [FEN]\neval\nquit" | ./external/engines/stockfish/stockfish
   
   # Check perft if move generation affected
   echo -e "position fen [FEN]\ngo perft [depth]\nquit" | ./external/engines/stockfish/stockfish
   ```

### 6. Critical Rules

#### NEVER:
- ❌ Implement multiple unrelated features in one phase
- ❌ Proceed to next phase without test results
- ❌ Forget to include bench count in commit
- ❌ Test locally without pushing to remote
- ❌ Assume a feature works without validation

#### ALWAYS:
- ✅ Push every commit immediately
- ✅ Document unexpected results
- ✅ Keep phases small and focused
- ✅ Maintain backward compatibility
- ✅ Test against main branch (not previous phase)

### 7. Risk Mitigation

#### Rollback Strategy
```bash
# If phase fails catastrophically
git checkout <last-known-good-commit>
git checkout -b <feature-branch-recovery>
```

#### Documentation Requirements
- Update feature_status.md after each phase
- Document failures as learning opportunities
- Keep detailed notes on what worked/didn't work

### 8. Success Metrics

#### Phase Success Criteria:
- **Infrastructure phases:** No regression (±10 ELO)
- **Minor features:** +5-15 ELO
- **Major features:** +20-50 ELO
- **Tuning phases:** +5-10 ELO

#### Overall Feature Success:
- Achieves minimum target ELO gain
- No regression in tactical strength
- Clean compilation, no warnings
- All tests pass

### 9. Communication Template

#### Phase Start:
```
Beginning Phase [X]: <description>
Base: <commit-sha>
Expected changes: <list>
Expected ELO: <range>
Risk factors: <if any>
```

#### Phase Complete:
```
Phase [X] Complete
Commit: <sha>
Bench: <count>
Local tests: PASSED
Ready for OpenBench: YES
```

#### Test Results:
```
Phase [X] Results:
ELO: <value> ± <error>
Games: <count>
Verdict: PASS/FAIL
Next steps: <action>
```

### 10. Common Pitfalls to Avoid

1. **Feature Interaction:** Features that work individually may conflict when combined
2. **Phase Scaling:** What works in endgame may hurt opening/middlegame
3. **Over-tuning:** Aggressive values that seem good locally but fail globally
4. **Hidden Dependencies:** Changes that affect unrelated systems
5. **Incomplete Testing:** Not testing all game phases

### 11. Emergency Procedures

If you lose >100 ELO:
1. STOP all development
2. Revert to last known good
3. Analyze with binary search
4. Create minimal reproducible case
5. Test each component in isolation

### 12. Feature Tracking Documentation

#### MANDATORY: Create feature_status.md for Each Feature

Every feature implementation MUST have a corresponding **temporary** tracking document:

**Location**: `/workspace/feature_status.md` (workspace root)
**Lifecycle**: Created on feature branch, deleted when merging to main
**Purpose**: Working document to track feature progress during development

The document should include:
- **Overview**: Feature description and expected ELO gain
- **Timeline**: Start date, branch name, base commit
- **Phase Status**: Each phase with commit SHA, bench count, test results
- **Testing Summary Table**: Clear view of all phases and results
- **Key Learnings**: What worked, what failed, and why
- **Implementation Details**: Technical details of successful approaches

This is a **TEMPORARY WORKING DOCUMENT** that:
- Lives only on the feature branch
- Gets removed before merging to main
- Serves as real-time tracking during development
- Helps organize multi-phase implementations
- Documents failures and successes for learning

The documentation is CRITICAL for:
- Learning from failures during development
- Tracking incremental progress in real-time
- Understanding which combinations work
- Debugging when phases fail
- Organizing complex multi-phase features

### 13. Final Checklist

Before marking any phase complete:
- [ ] Code compiles without warnings
- [ ] Bench count recorded
- [ ] Commit message includes bench
- [ ] Pushed to remote
- [ ] Status documented in feature_status.md
- [ ] Ready for OpenBench
- [ ] Completion message posted

## Remember

**Quality > Speed**

It's better to implement one feature correctly than three features poorly. Each successful phase builds confidence and understanding. Each failure teaches valuable lessons.

The goal is sustainable, measurable improvement - not rapid, unstable gains.