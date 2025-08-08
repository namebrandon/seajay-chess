# SeaJay Chess Engine - Deferred Items Tracker

**Purpose:** Track items deferred from earlier stages and items being deferred to future stages  
**Last Updated:** December 2024  

## Items FROM Stage 1 TO Stage 2

After reviewing Stage 1 implementation, the following was already completed but needs enhancement:

### Already Implemented (But Needs Enhancement):
1. **Basic FEN Parser** (`Board::fromFEN`)
   - ✅ Basic parsing exists
   - ⚠️ Needs: Error reporting with Result<T,E> type
   - ⚠️ Needs: Parse-to-temp-validate-swap pattern
   - ⚠️ Needs: Buffer overflow protection

2. **FEN Generator** (`Board::toFEN`) 
   - ✅ Basic implementation exists
   - ✅ Appears complete

3. **Board Display** (`Board::toString`)
   - ✅ Basic ASCII display exists
   - ⚠️ Could add: Unicode piece display option
   - ⚠️ Could add: Debug display with bitboards

4. **Basic Validation**
   - ✅ `validatePosition()` - basic implementation
   - ✅ `validateKings()` - both kings present
   - ✅ `validatePieceCounts()` - material limits
   - ✅ `validateEnPassant()` - basic EP validation
   - ✅ `validateCastlingRights()` - basic castling validation
   - ⚠️ Missing: Side not to move in check validation
   - ⚠️ Missing: Bitboard/mailbox sync validation
   - ⚠️ Missing: Zobrist validation

### Not Yet Implemented:
5. **Zobrist Rebuild After FEN**
   - Currently incrementally updates during parsing
   - Needs: Complete rebuild from scratch after successful parse

## Items DEFERRED FROM Stage 2 TO Future Stages

### To Stage 4 (Move Generation):
1. **En Passant Pin Validation**
   - Complex validation where EP capture would expose king to check
   - Test positions documented in stage2_position_management_plan.md
   - Reason: Expensive computation, needs attack generation

2. **Triple Check Validation**
   - Validating position doesn't have impossible triple check
   - Reason: Requires attack generation to verify

3. **Full Legality Check**
   - Checking if position is reachable from starting position
   - Reason: Complex retrograde analysis, not critical

### To Stage 3 (UCI):
1. **UCI Position Command**
   - Will use FEN parser to set up positions
   - Dependency: Robust FEN parsing from Stage 2

### To Stage 5 (Testing):
1. **Perft Test Positions**
   - Will use FEN parser to load test positions
   - Dependency: 100% accurate FEN parsing

## Tracking Mechanism

### How We Ensure Nothing Is Forgotten:

1. **Code Comments**: Add TODO comments at relevant locations
```cpp
// TODO(Stage4): Implement en passant pin validation
// See: deferred_items_tracker.md for details
```

2. **Stage Planning Documents**: Each stage plan should review this tracker
```markdown
## Prerequisites Check
- [ ] Review deferred_items_tracker.md for incoming items
- [ ] Update tracker with newly deferred items
```

3. **Test Placeholders**: Create disabled tests for deferred items
```cpp
TEST(Board, DISABLED_EnPassantPinValidation) {
    // TODO(Stage4): Enable when attack generation available
    const char* fen = "8/2p5/3p4/KP5r/8/8/8/8 w - c6 0 1";
    // Test will verify b5xc6 is illegal (exposes king)
}
```

4. **Project Status Updates**: Reference this tracker in project_status.md

## Stage 2 Specific Enhancements Needed

Based on review, Stage 2 needs to:

### High Priority (Core Requirements):
1. Implement Result<T,E> error handling system
2. Add parse-to-temp-validate-swap pattern
3. Add "side not to move in check" validation
4. Implement validateBitboardSync()
5. Implement validateZobrist() with full rebuild
6. Add buffer overflow protection in FEN parsing
7. Create comprehensive test suite with expert positions

### Medium Priority (Improvements):
1. Enhanced error messages with position info
2. Position hash function for testing
3. FEN normalization for testing
4. Debug assertions throughout

### Low Priority (Nice to Have):
1. Unicode piece display option
2. Debug display showing bitboards
3. Performance benchmarks

## Review Schedule

This tracker should be reviewed:
- At the start of each new stage
- During stage completion review
- When adding new deferred items

## Notes

- Items marked with ⚠️ need attention in current stage
- Items marked with ✅ are complete
- TODO comments in code should reference this document
- Keep this document updated as items are completed or deferred