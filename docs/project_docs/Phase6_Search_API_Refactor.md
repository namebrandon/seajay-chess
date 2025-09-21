Title: Phase 6 — Search API Refactor (Excluded-Move Ready)

Objective
- Prepare the search to support singular extensions, multi-cut, and probcut without changing current behavior or depth-parity results. Introduce an explicit, testable Search Node API that clarifies node context (PV/non-PV/root), and cleanly supports an optional excluded-move parameter.

**C++ Performance Summary:**
- **Zero-Overhead Design**: NodeContext packed to 4 bytes, passed in registers
- **Cache Efficiency**: Aligned to prevent false sharing, fits in single cache line
- **LazySMP Ready**: Thread-local context with no synchronization overhead
- **Compile-time Optimization**: Template specialization and constexpr for feature toggling
- **Target**: < 0.5% performance impact when disabled, measurable gain when enabled

Non-Goals (Phase 6)
- Do not enable singular extensions, multi-cut, or probcut yet.
- Do not change default pruning/reduction behavior or evaluation features.
- Do not alter root or quiescence behavior.

Success Criteria
- All unit tests and perft unchanged.
- Bench within noise; depth/time parity unchanged vs baseline with Phase 6 toggles off.
- With toggles on (NoOp mode), behavior remains identical; telemetry shows no excluded-move usage.
- Clear API that allows adding singular/multicut/probcut in a later phase without further plumbing changes.

Guardrails
- Keep features behind UCI options/compile flags; default OFF.
- Add DEBUG asserts to ensure excluded state never leaks across nodes.
- Ensure TT stores remain hygienic (avoid polluting good entries with shallow NO_MOVE heuristics).

UCI/Build Toggles
- UseSearchNodeAPIRefactor (bool, default false): switches negamax signature to use NodeContext and excludes stack-based ad-hoc flags. No behavior change.
- EnableExcludedMoveParam (bool, default false): threads an excluded move parameter; still disabled by default at call sites.
- DEV_ASSERT_SearchAPI (bool, default true in DEBUG): turns on additional verification logging/asserts.

Sub‑Phases (each SPRT vs current main with toggles OFF; code paths compile but NoOp by default)

**Stage Organization Guidelines:**
- Each stage touches < 5 files ideally, adds < 200 lines of code
- Each stage takes < 2 hours to implement, is independently testable
- Each stage has clear rollback point documented
- Dual testing: Stage with toggle OFF (neutral), then with toggle ON (SPRT bounds [-3.00, 3.00])

**Complete Stage List (10 stages total):**
1. **6a.1** - Add NodeContext structure definition (header only)
2. **6a.2** - Add negamax overload with legacy wrapper
3. **6b.1** - Thread context through main negamax recursive calls
4. **6b.2** - Thread context through quiescence search
5. **6b.3** - Thread context through helper paths
6. **6c** - Replace stack-based excluded-move checks
7. **6d** - Add verification helper (disabled)
8. **6e** - TT probe/store hygiene review
9. **6f** - PV clarity and root/QS safety
10. **6g** - Integration and rollout steps

6a.1 — Add NodeContext Structure Definition (Header Only)
- Add `search/node_context.h` with struct definition only (no usage)
- Define NodeContext with packed layout for optimal performance
- Add factory function declarations (makeRootContext, makeChildContext, etc.)
- Tests: Compilation only, verify struct size/alignment with static_assert
- **Rollback**: Simply delete the new header file
- **SPRT**: Not required (header only, no behavior change)

**C++ Developer Notes:**
- **Structure Layout**: Pack NodeContext carefully for optimal cache usage:
  ```cpp
  struct alignas(8) NodeContext {
      Move excluded;     // 2 bytes (uint16_t)
      uint8_t flags;     // 1 byte: isPv (bit 0), isRoot (bit 1)
      uint8_t padding;   // 1 byte padding for alignment

      // Inline accessors for zero-cost abstraction
      ALWAYS_INLINE bool isPv() const { return flags & 0x01; }
      ALWAYS_INLINE bool isRoot() const { return flags & 0x02; }
      ALWAYS_INLINE void setPv(bool pv) { flags = (flags & ~0x01) | (pv ? 0x01 : 0); }
      ALWAYS_INLINE void setRoot(bool root) { flags = (flags & ~0x02) | (root ? 0x02 : 0); }
  };
  ```
  This reduces size from 8 bytes (with bool members) to 4 bytes packed, improving cache efficiency.
- **Pass by Value**: Since NodeContext is only 4 bytes after packing, pass by value to avoid pointer indirection. Modern x86-64 ABI passes structs ≤8 bytes in registers.
- **Branch-free Initialization**: Use branchless bit manipulation for flag setting to improve pipeline efficiency.
- **Template Specialization**: Consider template parameter for compile-time PV/non-PV specialization when feature is mature:
  ```cpp
  template<bool IsPvNode>
  eval::Score negamax_impl(...); // Allows compiler to optimize out PV-specific code
  ```

**Stage 6a.1 Completion Checklist:**
```
=== STAGE 6a.1 COMPLETE ===
Branch: feature/phase6-stage-6a1
Commit: <sha>
Bench: <count> (should match baseline exactly)
Files Changed: 1 (src/search/node_context.h)
Toggle State: N/A (header only)
SPRT: Not required
Rollback: git checkout <previous-sha> && git rm src/search/node_context.h
Next: Proceed to 6a.2
```

6a.2 — Add Negamax Overload with Legacy Wrapper (NoOp)
- Add overloaded `negamax` accepting NodeContext parameter
- Implement legacy wrapper that creates default context and forwards
- No call sites updated yet (all still use legacy signature)
- Tests: Build, unit tests, perft unchanged, bench parity
- **Rollback**: Revert negamax.cpp changes only
- **SPRT**: Not required with toggle OFF; optional with toggle ON

**C++ Developer Notes:**
- **Overloading Strategy**: Keep both signatures initially for backward compatibility
- **Inline Wrapper**: Ensure legacy wrapper is force-inlined to avoid call overhead
- **Default Context**: Legacy wrapper should create root context with appropriate flags

**Stage 6a.2 Completion Checklist:**
```
=== STAGE 6a.2 COMPLETE ===
Branch: feature/phase6-stage-6a2
Commit: <sha>
Bench: <count> (should match baseline)
Files Changed: 2 (src/search/negamax.cpp, src/search/negamax.h)
Toggle State: UseSearchNodeAPIRefactor=false (default)
SPRT: Not required (wrapper only)
Rollback: git checkout <previous-sha>
Next: Proceed to 6b.1
```

6b.1 — Thread Context through Main Negamax Recursive Calls (NoOp)
- Update recursive negamax calls within negamax to use NodeContext
- Create child contexts with appropriate PV flag propagation
- Leave qsearch and helper paths unchanged for now
- Tests: Build, unit tests, perft; bench parity required
- **Rollback**: Revert negamax.cpp recursive call changes
- **SPRT**: Required if toggle ON shows any difference

6b.2 — Thread Context through Quiescence Search (NoOp)
- Update qsearch wrapper to accept NodeContext
- Ensure ctx.excluded is always NO_MOVE for qsearch
- Add assert to verify no excluded moves in qsearch
- Tests: Unit tests for qsearch context; bench parity
- **Rollback**: Revert qsearch.cpp changes only
- **SPRT**: Not required (qsearch never uses excluded)

6b.3 — Thread Context through Helper Paths (NoOp)
- Update remaining helper functions that need PV/non-PV knowledge
- Add DEBUG asserts to check consistency (e.g., child.isPv equals (parent.isPv && firstLegal))
- Complete context propagation throughout search
- Tests: Targeted unit tests for context propagation in synthetic tree
- **Rollback**: Revert helper function changes
- **SPRT**: Optional with toggle ON to verify complete propagation

**C++ Developer Notes:**
- **Register Optimization**: Pass NodeContext in registers (x86-64 passes structs ≤8 bytes in RDI/RSI). This avoids stack operations.
- **Inline Child Context Creation**:
  ```cpp
  ALWAYS_INLINE NodeContext makeChildContext(const NodeContext& parent, bool firstMove) {
      NodeContext child;
      child.excluded = NO_MOVE;
      child.flags = (firstMove && parent.isPv()) ? 0x01 : 0x00; // Only PV for first move
      return child;
  }
  ```
- **LazySMP Consideration**: NodeContext is thread-local by design (stack allocated). No synchronization needed between threads.
- **Prefetch Hints**: If context contains frequently accessed data, consider prefetch hints for child node preparation.

**Stage 6b.1-6b.3 Completion Checklists:**
```
=== STAGE 6b.1 COMPLETE ===
Branch: feature/phase6-stage-6b1
Commit: <sha>
Bench: <count>
Files Changed: 1 (src/search/negamax.cpp)
Toggle State: UseSearchNodeAPIRefactor=false
SPRT: Optional with toggle ON
Rollback: git revert <commit-sha>

=== STAGE 6b.2 COMPLETE ===
Branch: feature/phase6-stage-6b2
Commit: <sha>
Bench: <count>
Files Changed: 2 (src/search/qsearch.cpp, src/search/qsearch.h)
Toggle State: UseSearchNodeAPIRefactor=false
SPRT: Not required
Rollback: git revert <commit-sha>

=== STAGE 6b.3 COMPLETE ===
Branch: feature/phase6-stage-6b3
Commit: <sha>
Bench: <count>
Files Changed: 2-3 (helper files)
Toggle State: UseSearchNodeAPIRefactor=false
SPRT: Optional with toggle ON
Rollback: git revert <commit-sha>
```

6c — Replace Stack-Based Excluded-Move Checks with Parameter (NoOp)
- Replace `SearchInfo::isExcluded(ply, move)` usage at search loop with direct check against `ctx.excluded`.
- Keep SearchInfo’s excluded fields present but unused (deprecated) for now; add TODO to remove after rollout.
- Ensure no behavior change by leaving `ctx.excluded = NO_MOVE` from all call sites.
- Tests: confirm zero diff in node counts on a fixed suite; unit test ensures excluded move is respected when set (dev-only test harness).

**C++ Developer Notes:**
- **Branchless Comparison**: For excluded move checking, use branchless code:
  ```cpp
  // Instead of: if (move == ctx.excluded) continue;
  // Use: bool isExcluded = (move == ctx.excluded);
  //      searchMove = searchMove & ~isExcluded; // Mask out excluded
  ```
- **NO_MOVE Optimization**: Since NO_MOVE is typically 0, checking `ctx.excluded && move == ctx.excluded` can short-circuit.
- **Hot Path Optimization**: Place excluded check after more selective filters (e.g., after legality) to minimize comparisons.
- **Cache Line Awareness**: Keep NodeContext in same cache line as other frequently accessed search parameters.

**Stage 6c Completion Checklist:**
```
=== STAGE 6c COMPLETE ===
Branch: feature/phase6-stage-6c
Commit: <sha>
Bench: <count> (must match baseline exactly)
Files Changed: 1-2 (src/search/negamax.cpp, src/search/search_info.h)
Toggle State: EnableExcludedMoveParam=false
SPRT: Required with toggle ON to verify no behavior change
Rollback: git revert <commit-sha>
Node Count Test: Must show zero diff on fixed position suite
```

6d — Add Verification Helper (Disabled)
- Implement `verify_exclusion(Board&, ctx, depth, alpha, beta, limits, tt)` that performs a reduced-depth, narrow-window search intended for singular/multicut verification.
- Not called anywhere by default; provide minimal telemetry counters (compiled out in Release).
- Tests: compile-only; a DEV test can call it directly with excluded move to verify windowing and depth reductions, but keep out of normal search.

**C++ Developer Notes:**
- **Template-based Feature Toggle**: Use templates to compile out entirely when disabled:
  ```cpp
  template<bool EnableSingular = false>
  ALWAYS_INLINE eval::Score verify_exclusion_impl(Board& board, NodeContext ctx, ...) {
      if constexpr (!EnableSingular) {
          return eval::Score::zero(); // Compiled out entirely
      }
      // Actual implementation
  }
  ```
- **Depth Reduction Strategy**: Use bit shifts for fast depth reduction: `verifyDepth = depth >> 1` (half depth).
- **Narrow Window Optimization**: Pre-calculate narrow window bounds to avoid runtime arithmetic.
- **Stack Allocation**: Allocate verification-specific data on stack to avoid heap allocation overhead.
- **LazySMP Note**: Verification searches are independent per thread, no shared state needed.

**Stage 6d Completion Checklist:**
```
=== STAGE 6d COMPLETE ===
Branch: feature/phase6-stage-6d
Commit: <sha>
Bench: <count> (unchanged - function not called)
Files Changed: 2 (src/search/singular_extension.cpp, src/search/singular_extension.h)
Toggle State: N/A (compiled but not called)
SPRT: Not required (dead code)
Rollback: git rm src/search/singular_extension.*
Dev Test: Manual test with forced call to verify_exclusion
```

6e — TT Probe/Store Hygiene Review (NoOp Behavior)
- Audit TT store paths to ensure correct bounds for: static-null prune, null-move cutoff, and future excluded verification.
- Add comments and small guards to prevent overwriting deeper move-carrying entries with shallow NO_MOVE heuristic entries.
- No default behavior changes; toggles allow turning on stricter guards for A/B.
- Tests: unit tests for TT replacement behavior remain green; bench parity.

**C++ Developer Notes:**
- **TT Entry Extension**: Consider adding excluded move tracking to TTEntry:
  ```cpp
  struct TTEntry {
      // ... existing fields ...
      uint16_t excludedMove : 15;  // Excluded move (if any)
      uint16_t hasExcluded : 1;     // Flag if entry has excluded move
  };
  ```
  This allows caching singular extension results without pollution.
- **Replacement Policy**: Prioritize entries WITH moves over NO_MOVE entries:
  ```cpp
  bool shouldReplace = (newEntry.move != NO_MOVE) ||
                       (oldEntry.move == NO_MOVE) ||
                       (newEntry.depth > oldEntry.depth);
  ```
- **Memory Ordering**: Use relaxed atomic operations for TT access in LazySMP (no strict ordering needed).
- **Cache Line False Sharing**: Ensure TTEntry remains 16-byte aligned to prevent false sharing between threads.

**Stage 6e Completion Checklist:**
```
=== STAGE 6e COMPLETE ===
Branch: feature/phase6-stage-6e
Commit: <sha>
Bench: <count> (must match baseline)
Files Changed: 1-2 (src/tt/transposition_table.cpp, comments only)
Toggle State: N/A (documentation/guards only)
SPRT: Not required (comment changes)
Rollback: git revert <commit-sha>
TT Test: Verify replacement policy unit tests pass
```

6f — PV Clarity and Root/QS Safety (NoOp)
- Centralize PV handling via NodeContext; ensure qsearch never receives excluded moves and root remains unchanged.
- Add DEBUG asserts that rank-aware gates and pruning never trigger when ctx.excluded != NO_MOVE (future-proofing) and never at PV/root nodes when not intended.
- Tests: sanity checks over search traces; verify no asserts in DEBUG on standard suites.

**C++ Developer Notes:**
- **Compile-time Safety**: Use static_assert to ensure qsearch never gets excluded moves:
  ```cpp
  template<typename = std::enable_if_t<true>>
  eval::Score qsearch(Board& board, NodeContext ctx, ...) {
      static_assert(sizeof(ctx) > 0, "Context required");
      assert(ctx.excluded == NO_MOVE); // Runtime check in debug
  }
  ```
- **Root Node Optimization**: Special-case root node handling to avoid context overhead:
  ```cpp
  if (ctx.isRoot()) [[unlikely]] {  // C++20 attribute for branch prediction
      // Special root handling
  }
  ```
- **PV Collection**: Use separate PV collection path that bypasses excluded move logic entirely.
- **LazySMP Root Synchronization**: Root move selection must be synchronized across threads (use atomic for best move).

**Stage 6f Completion Checklist:**
```
=== STAGE 6f COMPLETE ===
Branch: feature/phase6-stage-6f
Commit: <sha>
Bench: <count> (must match baseline)
Files Changed: 2-3 (assert additions in search files)
Toggle State: DEV_ASSERT_SearchAPI=true (DEBUG only)
SPRT: Not required (assert-only changes)
Rollback: git revert <commit-sha>
Assert Test: Run DEBUG build on test suite, verify no assertions
```

6g — Integration + Rollout Steps
- Keep 6a–6f under `UseSearchNodeAPIRefactor=false` by default.
- Provide a DEV mode where it is true but ctx.excluded = NO_MOVE everywhere, to validate the new API under real search.
- Release bench validation (toggle OFF: 2350511 nodes @ 1.84M nps, toggle ON with `EnableExcludedMoveParam=false`: 2350511 nodes @ 1.75M nps; default runtime toggle now ON with 2350511 nodes @ 1.76M nps).
- Acceptance: once DEV runs show parity and no stability issues, set default to false and proceed to the next feature phase that uses the API.

**C++ Developer Notes:**
- **Feature Toggle Implementation**: Use constexpr for compile-time optimization:
  ```cpp
  constexpr bool USE_NODE_CONTEXT =
      #ifdef USE_SEARCH_NODE_API_REFACTOR
          true
      #else
          false
      #endif;

  if constexpr (USE_NODE_CONTEXT) {
      // New API path - compiled in/out entirely
  } else {
      // Legacy path
  }
  ```
- **Zero-Cost Abstraction**: When disabled, ensure complete removal via dead code elimination.
- **Incremental Rollout**: Use function pointers for runtime switching during A/B testing:
  ```cpp
  using SearchFunc = eval::Score(*)(Board&, int, int, ...);
  SearchFunc activeSearch = useNewAPI ? negamax_new : negamax_legacy;
  ```
- **Performance Monitoring**: Add performance counters (rdtsc) to measure overhead:
  ```cpp
  #ifdef PERF_MONITORING
  uint64_t start = __rdtsc();
  // ... search ...
  contextOverhead += __rdtsc() - start;
  #endif
  ```

**Stage 6g Completion Checklist:**
```
=== STAGE 6g COMPLETE ===
Branch: feature/phase6-stage-6g
Commit: <sha>
Bench: <count> (with toggle OFF must match baseline)
Files Changed: 1 (src/uci/uci.cpp for toggle default)
Toggle State: UseSearchNodeAPIRefactor=true (for testing)
SPRT: REQUIRED with bounds [-3.00, 3.00] with toggle ON
Rollback: git revert <commit-sha> && set toggle to false
Final Test: Extended run with toggle ON, ctx.excluded always NO_MOVE
```

**Overall Rollback Strategy:**
If regression detected at any stage:
1. Identify failing stage from commit history
2. `git checkout <last-good-sha>` (stage before regression)
3. `git checkout -b phase6-recovery-<stage>`
4. Document failure mode in feature_status.md
5. Either fix forward or abandon problematic stage
6. Can skip stages if dependencies allow (e.g., skip 6d if not needed yet)

Risks & Mitigations
- Risk: hidden behavior drift from PV propagation differences → Mitigate with asserts and A/B logs of PV move counts and PVS re-search rate.
- Risk: TT performance impact due to extra parameter threading → Keep inline-friendly APIs; verify bench parity.
- Risk: API churn affecting quiescence → Keep QS path explicitly orthogonal; asserts + tests.

**C++ Developer Notes:**
- **Performance Risk Mitigation**:
  - Use `perf record` and `perf stat` to measure cache misses and branch mispredictions before/after.
  - Target < 0.5% performance regression when features disabled.
  - Profile with both GCC and Clang to ensure optimizations work across compilers.
- **LazySMP Risk Mitigation**:
  - NodeContext is thread-local (stack allocated), eliminating race conditions.
  - TT access uses lock-free algorithms with relaxed atomics.
  - No shared mutable state in NodeContext itself.
- **Memory Layout Verification**:
  ```cpp
  static_assert(sizeof(NodeContext) <= 8, "NodeContext too large");
  static_assert(alignof(NodeContext) <= 8, "NodeContext over-aligned");
  static_assert(std::is_trivially_copyable_v<NodeContext>, "Must be POD");
  ```
- **Compiler Optimization Hints**:
  - Mark hot path functions with `__attribute__((hot))` (GCC/Clang).
  - Use `__builtin_expect` for branch prediction hints where profiling shows benefit.
  - Consider PGO (Profile-Guided Optimization) for production builds.

Validation & SPRT Guidance

**SPRT Protocol for Each Stage:**
1. Implement stage with toggle OFF (if applicable)
2. Commit with format: `feat: [Stage 6X.Y] - description - bench XXXXX`
3. Push to feature branch immediately
4. Run local validation:
   - `make clean && make`
   - `echo "bench" | ./seajay` (verify count matches)
   - Run unit tests if any
   - Perft unchanged
5. If stage adds/uses toggle:
   - Test locally with toggle ON
   - Request human SPRT run if behavior might change
   - Use bounds [-3.00, 3.00] for NoOp verification
   - Use bounds [-5.00, 5.00] for actual feature enablement
6. Wait for SPRT results before proceeding to next stage
7. Document in feature_status.md

**Stage-Specific SPRT Requirements:**
- 6a.1: No SPRT (header only)
- 6a.2: No SPRT with toggle OFF
- 6b.1-6b.3: Optional SPRT with toggle ON
- 6c: REQUIRED SPRT with toggle ON (excluded move logic)
- 6d: No SPRT (dead code)
- 6e: No SPRT (comments/guards)
- 6f: No SPRT (asserts only)
- 6g: REQUIRED SPRT with toggle ON (full integration test)

Follow-ups (post Phase 6)
- Phase SE1 (later): Implement true singular extensions using 6d helper.
- Phase MC1/PC1 (later): Multi-cut and ProbCut built on the same API with their own toggles and SPRTs.

**C++ Implementation Architecture Recommendations:**

1. **Header Organization**:
   ```cpp
   // search/node_context.h - Core types
   namespace seajay::search {
       struct alignas(8) NodeContext { /* ... */ };

       // Factory functions for context creation
       ALWAYS_INLINE NodeContext makeRootContext();
       ALWAYS_INLINE NodeContext makeChildContext(const NodeContext&, bool firstMove);
       ALWAYS_INLINE NodeContext makeExcludedContext(const NodeContext&, Move excluded);
   }
   ```

2. **Negamax Overloading Strategy**:
   ```cpp
   // Phase 1: Add overload, keep original
   eval::Score negamax(Board&, NodeContext, int depth, ...);
   eval::Score negamax(Board&, int depth, ...) {  // Legacy wrapper
       return negamax(board, makeRootContext(), depth, ...);
   }

   // Phase 2: Migrate all call sites
   // Phase 3: Remove legacy signature
   ```

3. **Compile-time Feature Matrix**:
   ```cpp
   template<bool UseSingular, bool UseMultiCut, bool UseProbCut>
   class SearchConfig {
       static constexpr bool singular = UseSingular;
       static constexpr bool multiCut = UseMultiCut;
       static constexpr bool probCut = UseProbCut;
   };

   using DefaultConfig = SearchConfig<false, false, false>;
   using SingularConfig = SearchConfig<true, false, false>;
   ```

4. **LazySMP Integration Points**:
   - NodeContext: Thread-local, no changes needed
   - TT Access: Already using relaxed atomics
   - Root Sync: Add `std::atomic<Move> globalBestMove` for thread coordination
   - Search Stack: Keep per-thread SearchInfo instances

5. **Performance Validation Checklist**:
   - [ ] sizeof(NodeContext) <= 8 bytes
   - [ ] NodeContext is trivially copyable
   - [ ] No dynamic allocations in hot path
   - [ ] Branch predictor friendly (< 2% misprediction rate)
   - [ ] L1 cache miss rate < 5% in search loop
   - [ ] Zero overhead when features disabled (verified with assembly)
   - [ ] Inline expansion verified for critical functions

6. **Testing Infrastructure**:
   ```cpp
   // test/search/node_context_test.cpp
   TEST(NodeContext, SizeAndAlignment) {
       static_assert(sizeof(NodeContext) <= 8);
       static_assert(alignof(NodeContext) == 8);
       static_assert(std::is_trivially_copyable_v<NodeContext>);
   }

   TEST(NodeContext, RegisterPassing) {
       // Verify ABI passes in registers with inline assembly check
   }
   ```
