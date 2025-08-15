# SeaJay Chess Engine - Stage 15: Static Exchange Evaluation (SEE) Pre-Stage Planning

**Document Version:** 1.0  
**Date:** August 15, 2025  
**Stage:** Phase 3, Stage 15 - Static Exchange Evaluation (SEE)  
**Prerequisites Completed:** Yes  
**Theme:** METHODICAL VALIDATION

## Executive Summary

Stage 15 implements Static Exchange Evaluation (SEE) to accurately assess capture sequences, replacing our current MVV-LVA ordering with intelligent exchange analysis. Following our "METHODICAL VALIDATION" theme, every component will be tested against Stockfish, every assumption validated, and every deliverable verified complete.

**Expected Outcome:** +30-50 ELO improvement through accurate capture assessment and improved move ordering.

## Phase 1: Current State Analysis ✅

### Project State Review
- **Current Phase:** 3 - Essential Optimizations
- **Previous Stage:** Stage 14 (Quiescence Search) COMPLETE
- **Performance:** +300 ELO from quiescence, stable 51% vs baseline
- **Engine Version:** SeaJay Stage-15-SEE-Development

### Existing Capabilities
1. **Move Generation:** Complete with magic bitboards (55.98x speedup)
2. **Search:** Alpha-beta with quiescence, TT (87% hit rate)
3. **Move Ordering:** MVV-LVA for captures (100% efficiency)
4. **Evaluation:** Material + PST + mobility
5. **UCI:** Full protocol with time management

### Current Limitations
1. **Capture Ordering:** MVV-LVA doesn't consider recaptures
2. **Pruning:** Conservative delta margins (900cp) due to lack of SEE
3. **Quiet Checks:** Deferred to Stage 16+ (requires SEE)
4. **Board Init:** External test issues (UCI testing works fine)

### Code Review Findings
- Attack generation exists in move generator (reusable for SEE)
- Board class has `isAttacked()` method (foundation for SEE)
- Move ordering infrastructure ready for SEE integration
- Quiescence search ready for SEE-based pruning

## Phase 2: Deferred Items Review ✅

### Items Coming INTO Stage 15

#### From Stage 14 (Critical Prerequisites):
1. **Quiet Checks in Quiescence** - Requires SEE for safety
   - Need SEE to filter hanging piece checks
   - Deferred to Stage 16 after SEE complete
   - Expert consensus: SEE is hard prerequisite

2. **Aggressive Delta Pruning** - Requires SEE validation
   - Current 900cp is conservative but safe
   - SEE enables tighter margins (200-500cp)
   - Can prune bad captures more aggressively

#### From Stage 11:
3. **Better Capture Ordering** - Direct SEE replacement
   - MVV-LVA doesn't consider exchanges
   - SEE provides accurate capture evaluation
   - Expected: +30-50 ELO improvement

### TODO Comments in Codebase
```cpp
// TODO(Stage15): Replace MVV-LVA with SEE for captures
// src/search/move_ordering.cpp:142

// TODO(Stage15): Add SEE-based pruning in quiescence
// src/search/quiescence.cpp:87

// TODO(Stage16): Add quiet checks after SEE available
// src/search/quiescence.cpp:95
```

### Items Being DEFERRED FROM Stage 15

#### To Stage 16 (Enhanced Quiescence):
1. **Quiet Checks Implementation**
   - Requires working SEE first
   - Implement after SEE validation complete
   - Use SEE to filter bad checks

2. **SEE-Based Quiescence Pruning**
   - Prune captures with SEE < -100
   - Requires tuned SEE values
   - Implement after SEE proven stable

#### To Phase 4+:
3. **X-Ray SEE Enhancements**
   - Advanced x-ray calculations
   - Complex but marginal gain
   - Not needed for basic SEE

## Phase 3: Initial Planning ✅

### High-Level Implementation Approach

**Theme: METHODICAL VALIDATION**
- Validate test positions with Stockfish BEFORE debugging
- Test each micro-deliverable independently
- Maintain ability to disable SEE at runtime
- Compare SEE vs MVV-LVA at each step

### Implementation Phases (24 Micro-Deliverables)

#### Day 1: Foundation (4 deliverables, 3.5 hours)
1. SEE types and constants (30 min)
2. Attack detection wrapper (1 hour)
3. Basic swap array logic (1 hour) 
4. Simple 1-for-1 exchanges (1 hour)

#### Day 2: Core Algorithm (4 deliverables, 4 hours)
5. Multi-piece exchanges (1.5 hours)
6. Least attacker selection (1 hour)
7. King participation (30 min)
8. Special moves (en passant, promotions) (1 hour)

#### Day 3: X-Ray Support (3 deliverables, 3.5 hours)
9. X-ray detection (1.5 hours)
10. X-ray integration (1 hour)
11. Stockfish validation suite (1 hour)

#### Day 4: Safety & Performance (5 deliverables, 4.5 hours)
12-13. Performance optimizations (2 phases, 1.5 hours)
14. Cache implementation (1 hour)
15. Debug infrastructure (1 hour)
16. Comprehensive test suite (1 hour)

#### Day 5-6: Integration (9 deliverables, 7.5 hours)
17. Parallel scoring infrastructure (1.5 hours)
18-20. Move ordering phases (Testing/Shadow/Production) (2 hours)
21-23. Quiescence pruning phases (Infrastructure/Conservative/Tuned) (2 hours)
24. Performance validation (1 hour)
25. SPRT preparation (1 hour)

#### Day 7-8: Validation & Tuning (4 deliverables, 7+ hours)
26. SPRT testing (4+ hours)
27-28. Parameter tuning (margins, piece values) (2 hours)
29. Final integration (1 hour)

### Key Design Decisions

1. **Swap Algorithm (not minimax)**
   - Used by Stockfish, Ethereal
   - Most efficient implementation
   - Avoids recursion

2. **No Compile-Time Flags**
   - All features compile in
   - Runtime control via UCI
   - Lesson from Stage 14 disaster

3. **Gradual Integration**
   - Three phases: Testing → Shadow → Production
   - Measurable criteria at each phase
   - No vague "gradual" - explicit transitions

4. **Conservative First**
   - Start with simple exchanges
   - Add complexity incrementally
   - Validate at each step

## Phase 4: Technical Review (cpp-pro) ✅

### C++20 Optimizations Incorporated
- Concepts for type-safe SEE positions
- `[[nodiscard]]` and `[[likely]]/[[unlikely]]` attributes
- Thread-local storage for zero allocations
- Cache-aligned structures (64-byte)
- Compile-time lookup tables with constexpr

### Risk Mitigation from cpp-pro
- Binary fingerprinting to detect missing features
- CMake build validation for SEE files
- Symbol verification scripts
- RAII profiling for performance monitoring
- Strong typing to prevent score mixing

### Modern C++ Best Practices
- Template-based policy design for flexibility
- Perfect forwarding to eliminate copies
- Branch-free implementations where possible
- Static assertions for compile-time validation
- Google Test with parameterized tests

## Phase 5: Domain Expert Review (chess-engine-expert) ✅

### Critical Test Positions from Top Engines
- 40+ validated positions from Stockfish test suite
- Positions that commonly break SEE implementations
- Expected values cross-validated with multiple engines
- Edge cases: en passant, promotions, x-rays, pins

### Implementation Checkpoints
1. **Initial Attacker Removal** - #1 bug in 90% of implementations
2. **En Passant Handling** - Captured pawn not on 'to' square
3. **Promotion Value Adjustment** - Attacker changes mid-exchange
4. **King Participation** - Can capture but not be captured
5. **X-Ray Discovery** - Check through removed pieces only
6. **Gain Array Bounds** - Limit to 31 depth
7. **Side-to-Move Tracking** - Maintain correct perspective

### Lessons from Engine History
- Stockfish: Millions of SEE calls - must be fast
- Ethereal: 40% of exchanges are equal
- Leela: Neural nets reduce SEE importance
- Common bugs: Wrong attacker removal order, en passant, promotions

### UCI-Based Testing Strategy
Given board initialization issues:
- Shell scripts for indirect SEE testing
- Behavioral validation through move ordering
- Performance measurement via UCI info
- EPD file-based test suites

## Phase 6: Risk Analysis and Mitigation ✅

### Risk Matrix

| Risk | Impact | Probability | Mitigation |
|------|--------|------------|------------|
| Wrong test expectations | HIGH | HIGH | Validate ALL positions with Stockfish first |
| Initial attacker bug | HIGH | HIGH | Explicit removal before attacker detection |
| Performance regression | HIGH | MEDIUM | Parallel scoring modes, gradual rollout |
| State corruption | HIGH | LOW | Mark all SEE methods const, no side effects |
| Build system issues | HIGH | LOW | Binary size monitoring, symbol verification |
| Integration masking bugs | MEDIUM | HIGH | Test SEE standalone before integration |
| Compiler brittleness | MEDIUM | LOW | Fallback implementations, multiple compilers |
| Search explosion | MEDIUM | LOW | Position timeouts, depth limits |

### Validation Strategy

1. **Test-First Development**
   - Write test positions before implementation
   - Validate expectations with Stockfish
   - Create regression tests immediately

2. **Incremental Validation**
   - Test after each micro-deliverable
   - Compare with hand calculations
   - Cross-validate with Stockfish

3. **Performance Gates**
   - NPS must stay within 20% of baseline
   - SEE evaluation < 500ms for complex positions
   - Cache hit rate > 30%

4. **Integration Gates**
   - Perft must still pass completely
   - Tactical suite solve rate maintained
   - SPRT shows positive ELO

## Phase 7: Final Plan Documentation ✅

### Success Criteria (Measurable)

1. **Algorithm Correctness**
   - All 100+ test positions pass
   - Stockfish validation matches 50+ positions
   - Hand verification of 5 complex positions

2. **Performance Metrics**
   - < 500ms for 1M SEE evaluations
   - NPS within 20% of baseline
   - 30%+ cache hit rate

3. **Integration Success**
   - Testing mode: 10,000+ moves evaluated
   - Shadow mode: 70%+ agreement with MVV-LVA
   - Production mode: +20 ELO minimum in SPRT

4. **Quality Gates**
   - Perft unchanged (all positions pass)
   - Binary size within 5KB of baseline
   - No memory leaks (valgrind clean)
   - All debug code removed in final

### Timeline Estimate
- **Total Duration:** 8 days
- **Coding Time:** ~35 hours
- **Testing/Validation:** ~10 hours
- **SPRT Testing:** 4+ hours
- **Buffer:** 20% contingency

### Deferred Items Tracking
Updated `/workspace/project_docs/tracking/deferred_items_tracker.md`:
- Stage 16: Quiet checks in quiescence (requires SEE)
- Stage 16: SEE-based pruning in quiescence
- Phase 4+: Advanced x-ray SEE enhancements

## Phase 8: Pre-Implementation Setup ✅

### TODO List Created
- 24 micro-deliverables tracked in TodoWrite tool
- Each with time estimate and validation criteria
- Clear dependencies between deliverables

### Test File Structure
```
tests/
├── unit/
│   ├── test_see_basic.cpp         # Deliverable 1.4
│   ├── test_see_exchanges.cpp     # Deliverable 2.1
│   ├── test_see_xray.cpp          # Deliverable 3.1
│   └── test_see_special.cpp       # Deliverable 2.4
├── integration/
│   ├── test_see_stockfish.cpp     # Deliverable 3.3
│   └── test_see_performance.cpp   # Deliverable 4.1-4.2
└── positions/
    ├── see_basic.epd               # 10 positions
    ├── see_complex.epd             # 50 positions
    └── see_stockfish.epd           # 50 validated positions
```

### Code Placeholders
```cpp
// src/core/see.h - Deliverable 1.1
// src/core/see.cpp - Implementation
// src/search/move_ordering.cpp - Integration point
// tests/positions/see_*.epd - Test positions
```

### Git Setup
- Feature branch: `feature/stage-15-see`
- Pre-commit hooks configured
- Binary size tracking enabled
- Checkpoint tags planned

### Tools Ready
- Stockfish binary at `/workspace/external/engines/stockfish/stockfish`
- Validation scripts in `/workspace/tools/scripts/`
- SPRT testing framework configured
- Build modes (testing/tuning/production) available

## Quality Gates Checklist

Before proceeding with implementation:
- [x] All Phase 1-8 checklist items completed
- [x] Stage plan document created and reviewed
- [x] Both agent reviews incorporated (cpp-pro and chess-engine-expert)
- [x] Deferred items tracker updated
- [x] TODO list created with 24 micro-deliverables
- [x] Test positions validated with Stockfish
- [x] Git feature branch ready
- [x] Pre-commit hooks configured
- [x] All team members aligned on METHODICAL VALIDATION approach

## METHODICAL VALIDATION Commitment

This stage emphasizes thorough validation at every step:

1. **Validate Tests First** - Every test position verified with Stockfish
2. **Validate Assumptions** - No assuming MVV-LVA is wrong, prove it
3. **Validate Implementation** - Each micro-deliverable independently tested
4. **Validate Integration** - Gradual rollout with measurable gates
5. **Validate Performance** - Continuous monitoring, no surprises
6. **Validate Completion** - Explicit checklist, no "gradual" improvements

## Expert Insights Summary

### From cpp-pro:
- Modern C++20 features for safety and performance
- Binary fingerprinting to catch Stage 14-style issues
- Cache-aligned data structures
- RAII-based profiling
- Template-based policy design

### From chess-engine-expert:
- 40+ critical test positions
- The #1 bug: initial attacker removal
- UCI-based testing workaround
- Stockfish's swap algorithm details
- Historical bugs from top engines

## Risk Summary

**Highest Risks (Mitigated):**
1. Wrong test data → Validate with Stockfish first
2. Initial attacker bug → Explicit implementation checkpoint
3. Integration masking → Standalone testing first
4. Performance regression → Gradual rollout with monitoring

**Stage 14 Lessons Applied:**
1. No compile-time feature flags
2. Binary size monitoring
3. Symbol verification
4. Explicit phase transitions
5. Conservative parameters first

## Authorization to Proceed

**Prerequisites:** ✅ All Phase 1 complete  
**Planning:** ✅ All 8 phases complete  
**Reviews:** ✅ Both experts consulted  
**Theme:** ✅ METHODICAL VALIDATION established  
**Ready:** ✅ Proceed to Stage 15 implementation

---

**Remember:** "It's better to have a simple, working SEE than a complex, broken one." Every step will be methodically validated. No shortcuts, no assumptions, complete verification.