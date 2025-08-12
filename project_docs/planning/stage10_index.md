# Stage 10: Magic Bitboards - Document Index

## Document Hierarchy

```
stage10_implementation_steps.md (MASTER DOCUMENT)
├── stage10_magic_bitboards_plan.md     (Overall plan and requirements)
├── stage10_magic_validation_harness.md (Step 0A: Validation implementation)
├── stage10_critical_additions.md       (Critical safety measures from cpp-pro)
└── stage10_git_strategy.md            (Version control workflow)
```

## Reading Order for Implementation

1. **Start Here:** [`stage10_implementation_steps.md`](./stage10_implementation_steps.md)
   - This is the MASTER document with granular steps
   - Follow this step-by-step during implementation

2. **Pre-Implementation Setup:** [`stage10_critical_additions.md`](./stage10_critical_additions.md)
   - Critical safety measures to implement BEFORE starting
   - CMake configuration, sanitizers, differential testing

3. **Step 0A Reference:** [`stage10_magic_validation_harness.md`](./stage10_magic_validation_harness.md)
   - Complete MagicValidator class implementation
   - Critical test positions and validation methods

4. **Git Workflow:** [`stage10_git_strategy.md`](./stage10_git_strategy.md)
   - Commit after each step
   - Tag after each phase
   - Emergency recovery procedures

5. **Overall Context:** [`stage10_magic_bitboards_plan.md`](./stage10_magic_bitboards_plan.md)
   - High-level plan and rationale
   - Expert review feedback
   - Success criteria

## Quick Reference

| Task | Document | Section |
|------|----------|---------|
| Setting up CMake | `stage10_critical_additions.md` | Section 2 |
| Creating validator | `stage10_magic_validation_harness.md` | Master Validator Class |
| Differential testing | `stage10_critical_additions.md` | Section 3 |
| Critical test positions | `stage10_magic_validation_harness.md` | Critical Test Positions |
| Commit strategy | `stage10_git_strategy.md` | Commit Strategy |
| Edge case bugs | `stage10_magic_bitboards_plan.md` | Critical Edge Cases |
| Performance targets | `stage10_implementation_steps.md` | Step 4A |

## Implementation Checkpoints

After completing each phase in `stage10_implementation_steps.md`:

| Phase | Validation | Git Tag |
|-------|------------|---------|
| Phase 0 | Validation framework works | `magic-phase0-complete` |
| Phase 1 | All magic numbers validated | `magic-phase1-complete` |
| Phase 2 | 294,912 patterns validated | `magic-phase2-complete` |
| Phase 3 | Perft matches 99.974% | `magic-phase3-complete` |
| Phase 4 | 3-5x speedup achieved | `magic-phase4-complete` |
| Phase 5 | 1000 games validated | `magic-phase5-complete` |

## Key Reminders

1. **ALWAYS** start with `stage10_implementation_steps.md`
2. **NEVER** skip validation gates between steps
3. **COMMIT** after every step (44 commits total)
4. **TAG** after every phase (6 phase tags)
5. **TEST** with smoke test after every change (<1 second)

## Emergency Contacts

If you find a document without clear parent reference, check this index.
All Stage 10 documents should reference back to `stage10_implementation_steps.md` as the master.