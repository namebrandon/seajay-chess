# UCI Score Conversion Plan for SeaJay

## Overview
SeaJay currently outputs negamax scores (from side-to-move perspective) directly to UCI, which violates the UCI protocol standard. This document outlines the plan to convert internal scores to UCI-compliant White's perspective while maintaining negamax scoring internally.

## UCI Protocol Requirements

### Standard Score Reporting
- **UCI Specification**: All scores must be from White's perspective
- **Positive scores**: White is winning
- **Negative scores**: Black is winning
- **Independent of**: Which side is to move

### Current SeaJay Behavior (Non-Compliant)
- Reports raw negamax scores (side-to-move perspective)
- Positive score = current player winning
- This confuses GUIs expecting White's perspective

## Implementation Plan

### 1. Core Conversion Function

Create a centralized conversion function in `uci.cpp`:

```cpp
// Convert internal negamax score to UCI (White's perspective)
int toUciScore(eval::Score internalScore, Color sideToMove) {
    // Internal score is from side-to-move perspective
    // UCI needs White's perspective
    int cp = internalScore.to_cp();
    return (sideToMove == WHITE) ? cp : -cp;
}

// Helper for mate scores
std::string formatUciScore(eval::Score internalScore, Color sideToMove) {
    if (internalScore.isMate()) {
        int mateIn = internalScore.mateIn();
        // Adjust mate score for perspective
        if (sideToMove == BLACK) mateIn = -mateIn;
        return "mate " + std::to_string(mateIn);
    } else {
        return "cp " + std::to_string(toUciScore(internalScore, sideToMove));
    }
}
```

### 2. Locations Requiring Conversion

#### A. Search Info Output (`negamax.cpp`)
- **File**: `/workspace/src/search/negamax.cpp`
- **Functions to modify**:
  - `sendIterationInfo()` - Line ~1665
  - `sendCurrentSearchInfo()` - Line ~1624
  - Any place using `InfoBuilder::appendScore()`

#### B. UCI Eval Command (`uci.cpp`)
- **File**: `/workspace/src/uci/uci.cpp`
- **Function**: `handleEval()` - Line ~1066
- **Changes needed**:
  - Convert total evaluation to White's perspective
  - Keep component breakdown as-is (they're already absolute values)
  - Update the "From White's perspective" display

#### C. InfoBuilder Class (`info_builder.cpp`)
- **File**: `/workspace/src/uci/info_builder.cpp`
- **Method**: `appendScore()`
- **Changes needed**:
  - Add Color parameter to know side-to-move
  - Apply conversion before building string

### 3. Critical Principles

#### NEVER Convert Internally
- Search functions continue using negamax scoring
- Transposition table stores negamax scores
- Move ordering uses negamax scores
- Only convert at the UCI output boundary

#### Maintain Backward Compatibility
- Internal search logic remains unchanged
- No impact on move selection or pruning
- No impact on transposition table hits

### 4. Implementation Phases

#### Phase 1: Infrastructure (No ELO Impact)
- Add conversion functions to `uci.cpp`
- Add Color parameter where needed
- Compile and verify no crashes

#### Phase 2: Convert Search Info Output
- Modify `sendIterationInfo()` to use conversion
- Modify `sendCurrentSearchInfo()` to use conversion
- Test that GUI displays correct perspective

#### Phase 3: Convert Eval Command
- Update `handleEval()` to show White's perspective
- Ensure breakdown components remain clear
- Test with various positions

#### Phase 4: Validation
- Test starting position (should show ~+30cp for White)
- Test positions where Black is winning (should show negative)
- Test mate positions (mate scores from White's view)
- Compare with Stockfish output for same positions

### 5. Testing Strategy

#### Primary Testing Approach: SeaJay Before/After Comparison
**Key Principle**: Compare SeaJay's output BEFORE and AFTER conversion implementation, NOT against Stockfish (due to evaluation differences).

#### Test Matrix

| Test Case | Position | Side to Move | Current SeaJay Output | Expected After Conversion | Conversion Logic |
|-----------|----------|--------------|----------------------|---------------------------|------------------|
| 1 | Startpos | White | +58cp | +58cp | No change (White to move) |
| 2 | After 1.e4 | Black | +15cp | -15cp | Negate (Black to move) |
| 3 | After 1.e4 e5 | White | +39cp | +39cp | No change (White to move) |
| 4 | After 1.e4 e5 2.Nf3 | Black | +10cp | -10cp | Negate (Black to move) |
| 5 | Kiwipete | Black | +198cp | -198cp | Negate (Black to move) |

#### Verification Commands
```bash
# Step 1: Save current SeaJay outputs (BEFORE conversion)
echo "=== BEFORE CONVERSION ===" > before_conversion.txt
echo -e "position startpos\ngo depth 5\nquit" | ./bin/seajay | grep "score cp" >> before_conversion.txt
echo -e "position startpos moves e2e4\ngo depth 5\nquit" | ./bin/seajay | grep "score cp" >> before_conversion.txt
echo -e "position startpos moves e2e4 e7e5\ngo depth 5\nquit" | ./bin/seajay | grep "score cp" >> before_conversion.txt
echo -e "position startpos moves e2e4 e7e5 g1f3\ngo depth 5\nquit" | ./bin/seajay | grep "score cp" >> before_conversion.txt

# Step 2: Implement conversion

# Step 3: Test with SAME positions (AFTER conversion)
echo "=== AFTER CONVERSION ===" > after_conversion.txt
# Run same commands, capture output

# Step 4: Verify conversion logic
# White to move: scores should be UNCHANGED
# Black to move: scores should be NEGATED
```

#### Validation Rules
1. **White to move positions**: Score should remain the same
2. **Black to move positions**: Score should flip sign (positive → negative, negative → positive)
3. **Mate scores**: Follow same rule (White to move unchanged, Black to move negated)
4. **Best move selection**: Should be IDENTICAL (no change to move selection)
5. **Search depth reached**: Should be IDENTICAL (no performance impact)

#### Secondary Validation (Stockfish Comparison)
Only AFTER verifying correct sign conversion, optionally compare with Stockfish to check if signs align:
- Both engines show positive when White is better
- Both engines show negative when Black is better
- Magnitudes will differ due to evaluation differences (this is expected and OK)

### 6. Risk Mitigation

#### Potential Issues
1. **GUI compatibility**: Some GUIs might have adapted to SeaJay's current behavior
2. **User confusion**: Existing users might be confused by score change
3. **Testing tools**: OpenBench or other tools might expect current behavior

#### Mitigation Strategies
1. **Clear documentation**: Update README and UCI docs
2. **Transition period**: Keep the "info string" warning about scoring
3. **Version marking**: Clear version notes about this change
4. **Optional flag**: Consider UCI option to use old behavior (not recommended)

### 7. Example Conversions

| Position | Side to Move | Internal Score | Current UCI Output | Correct UCI Output |
|----------|--------------|----------------|-------------------|-------------------|
| Startpos | White | +35cp | score cp 35 | score cp 35 |
| After e4 | Black | +25cp | score cp 25 | score cp -25 |
| Black winning | White | -300cp | score cp -300 | score cp -300 |
| Black winning | Black | +300cp | score cp 300 | score cp -300 |

### 8. Code Example - InfoBuilder Modification

```cpp
// Current (incorrect)
void InfoBuilder::appendScore(eval::Score score) {
    m_stream << " score cp " << score.to_cp();
}

// Fixed (correct)
void InfoBuilder::appendScore(eval::Score score, Color sideToMove) {
    int uciScore = (sideToMove == WHITE) ? score.to_cp() : -score.to_cp();
    m_stream << " score cp " << uciScore;
}
```

### 9. Automated Test Script

Create `test_uci_conversion.sh`:
```bash
#!/bin/bash

SEAJAY_OLD="./bin/seajay_before_conversion"
SEAJAY_NEW="./bin/seajay"

echo "UCI Score Conversion Test"
echo "========================="

# Test function
test_position() {
    local desc="$1"
    local pos_cmd="$2"
    local side_to_move="$3"
    
    echo -e "\nTest: $desc (${side_to_move} to move)"
    
    # Get old output
    old_score=$(echo -e "$pos_cmd\ngo depth 5\nquit" | $SEAJAY_OLD 2>&1 | grep "info depth 5" | head -1 | grep -oP "score cp \K[-+]?[0-9]+")
    
    # Get new output
    new_score=$(echo -e "$pos_cmd\ngo depth 5\nquit" | $SEAJAY_NEW 2>&1 | grep "info depth 5" | head -1 | grep -oP "score cp \K[-+]?[0-9]+")
    
    echo "  Before: ${old_score}cp"
    echo "  After:  ${new_score}cp"
    
    # Verify conversion
    if [ "$side_to_move" = "White" ]; then
        if [ "$old_score" = "$new_score" ]; then
            echo "  ✓ PASS: White to move, score unchanged"
        else
            echo "  ✗ FAIL: White to move, score should be unchanged!"
        fi
    else
        expected=$((old_score * -1))
        if [ "$new_score" = "$expected" ]; then
            echo "  ✓ PASS: Black to move, score correctly negated"
        else
            echo "  ✗ FAIL: Black to move, expected ${expected}cp, got ${new_score}cp"
        fi
    fi
}

# Run tests
test_position "Starting position" "position startpos" "White"
test_position "After 1.e4" "position startpos moves e2e4" "Black"
test_position "After 1.e4 e5" "position startpos moves e2e4 e7e5" "White"
test_position "After 1.e4 e5 2.Nf3" "position startpos moves e2e4 e7e5 g1f3" "Black"
test_position "After 1.e4 e5 2.Nf3 Nc6" "position startpos moves e2e4 e7e5 g1f3 b8c6" "White"

echo -e "\n========================="
echo "Test Complete"
```

### 10. Validation Checklist

- [ ] Conversion functions implemented
- [ ] Search info outputs White's perspective
- [ ] Eval command outputs White's perspective  
- [ ] Mate scores handled correctly
- [ ] GUI displays match Stockfish for same positions
- [ ] No impact on playing strength
- [ ] Documentation updated
- [ ] Tests pass with new scoring

### 10. Long-term Considerations

#### Future Enhancements
1. **Win probability**: Like Stockfish, could add win% in addition to centipawns
2. **Normalization**: Could normalize scores so 100cp = 50% win probability
3. **Tablebase scores**: When adding tablebase support, ensure correct perspective

#### Maintaining Compliance
1. All future UCI output must use conversion functions
2. Add unit tests to prevent regression
3. Document this requirement in developer guidelines

## Summary

This change is critical for UCI compliance and GUI compatibility. While it requires careful implementation to avoid breaking internal logic, the actual changes are localized to UCI output functions. The key principle is: **convert at the boundary, never internally**.