# SeaJay Chess Engine - Pre-Stage Development Planning Process

**Document Type:** Process Template  
**Purpose:** Mandatory planning process to be completed BEFORE any stage development begins  
**Status:** REQUIRED - No stage development may begin without completing this process  

## CRITICAL REQUIREMENT

**⚠️ THIS PROCESS MUST BE COMPLETED BEFORE STARTING ANY NEW STAGE ⚠️**

Failure to complete this planning process risks:
- Introduction of subtle bugs that propagate to future stages
- Missing deferred requirements from previous stages
- Inefficient or incorrect architectural decisions
- Wasted development time on rework
- Loss of project coherence and quality

## Pre-Stage Planning Checklist

### Phase 1: Current State Analysis
- [ ] Review project_status.md for overall project state
- [ ] Review the Master Project Plan for the target stage requirements
- [ ] Examine all existing code from previous stages
- [ ] Check git history for recent changes and context
- [ ] Document current capabilities and limitations

### Phase 2: Deferred Items Review
- [ ] Review `/workspace/project_docs/tracking/deferred_items_tracker.md` for items coming INTO this stage
- [ ] Identify all TODO comments in codebase related to current stage
- [ ] Check disabled tests that should be enabled in this stage
- [ ] Document all deferred items that must be addressed
- [ ] Update tracker with items being addressed

### Phase 3: Initial Planning
- [ ] Create high-level implementation plan based on Master Project Plan
- [ ] Identify potential risks and edge cases
- [ ] Consider modularity and future stage requirements
- [ ] Document key design decisions and rationale
- [ ] List success criteria and validation requirements

### Phase 4: Technical Review with cpp-pro Agent
- [ ] Present plan to cpp-pro agent for C++ best practices review
- [ ] Request specific C++20/23 feature recommendations
- [ ] Get feedback on error handling approach
- [ ] Review memory management and performance considerations
- [ ] Incorporate feedback into plan

### Phase 5: Domain Expert Review with chess-engine-expert Agent
- [ ] Present plan to chess-engine-expert for domain-specific review
- [ ] Request identification of common chess engine pitfalls
- [ ] Get specific test positions and edge cases
- [ ] Review algorithmic approaches and optimizations
- [ ] Validate chess rules and special case handling
- [ ] Incorporate expert feedback into plan

### Phase 6: Risk Analysis and Mitigation
- [ ] Identify all areas where bugs could be introduced
- [ ] Document how each risk will be mitigated
- [ ] Consider impact on future stages
- [ ] Plan comprehensive validation strategy
- [ ] Create test-first development approach where applicable

### Phase 7: Final Plan Documentation
- [ ] Create stage-specific implementation plan document in `/workspace/project_docs/planning/`
- [ ] Include all feedback from technical and domain reviews
- [ ] Document items being deferred FROM this stage
- [ ] Update `/workspace/project_docs/tracking/deferred_items_tracker.md` with new deferrals
- [ ] Define clear success criteria and exit conditions

### Phase 8: Pre-Implementation Setup
- [ ] Create TODO list for stage implementation
- [ ] Set up test file structure
- [ ] Add TODO comments for deferred items
- [ ] Create disabled test placeholders for future features
- [ ] Ensure all tools and dependencies are ready

## Document Templates

### Stage Plan Document Structure
```markdown
# SeaJay Chess Engine - Stage [X]: [Stage Name] Implementation Plan

**Document Version:** 1.0  
**Date:** [Date]  
**Stage:** Phase [X], Stage [Y] - [Stage Name]  
**Prerequisites Completed:** [Yes/No]  

## Executive Summary
[Brief overview of stage goals and approach]

## Current State Analysis
[What exists from previous stages]

## Deferred Items Being Addressed
[Items from `/workspace/project_docs/tracking/deferred_items_tracker.md` for this stage]

## Implementation Plan
[Detailed implementation approach with phases]

## Technical Considerations
[C++ specific details from cpp-pro review]

## Chess Engine Considerations  
[Domain specific details from chess-engine-expert review]

## Risk Mitigation
[Identified risks and mitigation strategies]

## Validation Strategy
[How we'll verify correctness]

## Items Being Deferred
[What we're pushing to future stages and why]

## Success Criteria
[Clear, measurable completion criteria]

## Timeline Estimate
[Realistic time estimates]
```

## Review Questions for Agents

### For cpp-pro Agent:
1. Review existing code architecture - any concerns?
2. Optimal C++20/23 features for this stage?
3. Error handling recommendations?
4. Performance considerations while maintaining correctness?
5. Memory management best practices?
6. Testing approach recommendations?

### For chess-engine-expert Agent:
1. Common pitfalls for this stage's features?
2. Critical test positions for validation?
3. Chess rule edge cases to consider?
4. Performance vs correctness trade-offs?
5. Integration considerations with other engine components?
6. Industry best practices for this functionality?

## Quality Gates

Before proceeding with implementation:
- [ ] All checklist items completed
- [ ] Stage plan document created and reviewed
- [ ] Both agent reviews incorporated
- [ ] Deferred items tracker updated
- [ ] TODO list created in task management tool
- [ ] All team members (human and AI) aligned on approach

## Process Improvements

After each stage completion:
1. Review what worked well in planning
2. Identify what could be improved
3. Update this template with lessons learned
4. Share insights in development diary

## Enforcement

**This process is MANDATORY. The AI assistant should:**
1. Refuse to begin stage implementation without completed planning
2. Prompt for planning document creation if not present
3. Ensure all reviews are conducted
4. Verify deferred items are tracked
5. Confirm success criteria are defined

## Example Usage

```
Human: Let's start working on Stage 3 - UCI Protocol

AI: Before we begin Stage 3 implementation, we need to complete the mandatory Pre-Stage Development Planning Process. Let me start by:
1. Reviewing the current project state
2. Checking deferred items from Stage 2
3. Creating an initial plan for Stage 3
4. Getting reviews from our expert agents
[Proceeds through all phases]
```

## Version History

- v1.0 (December 2024): Initial template based on Stage 2 planning experience

---

**Remember:** The time spent in planning saves multiples of that time in debugging and rework. A well-planned stage implementation is a successful stage implementation.