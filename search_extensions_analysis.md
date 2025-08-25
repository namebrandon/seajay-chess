# Search Extensions Analysis: Reference Engines vs SeaJay

## Executive Summary

After analyzing three reference engines (Publius, 4ku, and Stash-bot), SeaJay is missing several critical search extensions that are standard in competitive chess engines. The most important missing extension is **check extensions**, which all three reference engines implement unconditionally. SeaJay also lacks singular extensions, which two of the three reference engines use for deeper searches.

## Detailed Comparison Table

| Extension Type | Publius | 4ku | Stash-bot | SeaJay | Priority |
|---------------|---------|-----|-----------|---------|----------|
| **Check Extensions** | ✅ Always +1 ply | ✅ Always +1 ply | ✅ Always +1 ply | ❌ Missing | **CRITICAL** |
| **Singular Extensions** | ❌ No | ❌ No | ✅ +1-3 ply (depth≥7) | ❌ Missing | HIGH |
| **Multi-cut Pruning** | ❌ No | ❌ No | ✅ Yes | ❌ Missing | MEDIUM |
| **Negative Extensions** | ❌ No | ❌ No | ✅ -1 ply | ❌ Missing | LOW |
| **Recapture Extensions** | ❌ No | ❌ No | ❌ No | ❌ Missing | LOW |
| **Passed Pawn Extensions** | ❌ No | ❌ No | ❌ No | ❌ Missing | LOW |

## 1. Check Extensions (CRITICAL - Implement First)

### Implementation Details Across Engines

#### Publius (publius_search.cpp)
```cpp
// Lines 60-61, 141, 318, 486
// Check extension applied in THREE places:
// 1. Root search (line 61)
if (new_depth && bd->isInCheck(bd->to_move)) ++new_depth;

// 2. PV nodes (line 141)
if (in_check) ++depth;

// 3. Cut/All nodes (lines 318, 486)
if (in_check) ++depth;
```

#### 4ku (4ku_main.cpp)
```cpp
// Lines 651-653
// Simple unconditional check extension
const i32 in_check = is_attacked(pos, lsb(pos.colour[0] & pos.pieces[King]));
depth += in_check;
```

#### Stash-bot (stash_search.c)
```cpp
// Lines 892-895
// Check extension as part of extension logic
else if (gives_check) {
    extension = 1;
}
```

### SeaJay Implementation Gap
SeaJay has the infrastructure to detect checks (`weAreInCheck` variable on line 193) but **never extends the search depth** when in check. This is a fundamental oversight that significantly weakens tactical play.

### Recommended Implementation for SeaJay
```cpp
// After line 193 in negamax.cpp
bool weAreInCheck = inCheck(board);

// Add check extension immediately after
if (weAreInCheck) {
    depth++;  // Unconditional check extension
}
```

## 2. Singular Extensions (HIGH Priority)

### Implementation in Stash-bot
Stash-bot implements sophisticated singular extensions (lines 849-891):

```c
// Conditions for singular extension:
// - depth >= 7
// - TT move exists with LOWER bound
// - Not already in singular search
// - Score not near mate

if (depth >= 7 && currmove == tt_move && !ss->excluded_move 
    && (tt_bound & LOWER_BOUND) && i16_abs(tt_score) < VICTORY 
    && tt_depth >= depth - 3) {
    
    // Singular search at reduced depth
    const Score singular_beta = tt_score - 10 * depth / 16;
    const i16 singular_depth = depth / 2 + 1;
    
    // Extension decision:
    if (singular_score < singular_beta) {
        // Double extension for very singular moves
        if (!pv_node && singular_beta - singular_score > 14 
            && ss->double_extensions <= 10) {
            extension = 2 + (!tt_noisy && singular_beta - singular_score > 120);
        } else {
            extension = 1;
        }
    }
}
```

### Recommended Implementation for SeaJay
Start with a simpler version and tune later:
```cpp
// Add after TT probe (around line 320)
if (depth >= 6 && ttMove != NO_MOVE && !isPvNode) {
    // Simplified singular extension
    eval::Score singularBeta = ttScore - eval::Score(50);
    int singularDepth = depth / 2;
    
    // Do reduced search excluding TT move
    eval::Score singularScore = /* search without ttMove */;
    
    if (singularScore < singularBeta) {
        depth++;  // Extend singular moves
    }
}
```

## 3. Extension Application Points

### Where Extensions Are Applied

| Engine | Application Point | Notes |
|--------|------------------|-------|
| **Publius** | Before move generation | Check extension applied early, affects all moves |
| **4ku** | After TT probe, before move loop | Single check extension point |
| **Stash-bot** | Per-move basis in move loop | Most flexible, allows different extensions per move |
| **SeaJay** | Currently none | Needs implementation |

## 4. Extension Amounts

### Check Extensions
- **Universal standard**: +1 ply
- **Applied**: Always when in check
- **No conditions**: No depth requirements or other restrictions

### Singular Extensions (Stash-bot)
- **Base**: +1 ply for singular moves
- **Double**: +2 plies for very singular moves (margin > 14)
- **Triple**: +3 plies for extremely singular moves (margin > 120 and quiet)

## 5. Implementation Priority and Impact

### Priority 1: Check Extensions (Immediate)
**Impact**: 50-100 Elo gain expected
- Prevents horizon effect in tactical sequences
- Ensures checks are fully resolved
- Standard in every competitive engine

### Priority 2: Singular Extensions (Next Phase)
**Impact**: 20-40 Elo gain expected
- Identifies critical moves that must be played
- Prevents missing key defensive/offensive moves
- More complex but worthwhile

### Priority 3: Multi-cut Pruning (Future)
**Impact**: 10-20 Elo gain
- Reduces search effort when multiple moves beat beta
- Optimization rather than accuracy improvement

## 6. Specific Implementation Recommendations for SeaJay

### Phase 1: Basic Check Extension
```cpp
// In negamax.cpp, after line 193
if (weAreInCheck && depth <= MAX_PLY - ply) {
    depth++;  // Unconditional check extension
    info.checkExtensions++;  // Track statistics
}
```

### Phase 2: Singular Extension Framework
1. Add excluded move to search stack
2. Implement singular search (search with move excluded)
3. Add extension decision logic
4. Track singular extension statistics

### Phase 3: Extension Limiting
- Maximum extensions per path (prevent explosion)
- Double extension limiting (Stash-bot's approach)
- Depth-based extension reduction

## 7. Testing Strategy

### Verification Tests
1. **Tactical test suites**: Should see improvement in tactical puzzle solving
2. **Mate finding**: Should find mates 1-2 plies earlier
3. **Time-to-depth**: Will be slower but more accurate
4. **Perft with extensions**: Verify correctness

### Performance Metrics to Track
- Extensions per search
- Average depth reached
- Tactical puzzle success rate
- ELO gain in self-play

## Conclusion

SeaJay's lack of check extensions is a critical weakness that should be addressed immediately. This is not an optimization but a fundamental search feature that all competitive engines implement. The implementation is straightforward and the benefits are substantial.

After implementing check extensions, singular extensions should be the next priority as they provide significant strength improvement in identifying critical moves. The Stash-bot implementation provides a good reference for a sophisticated approach, though SeaJay should start with a simpler version.

The absence of these extensions likely explains some of SeaJay's tactical weaknesses and depth deficit compared to other engines. Implementing check extensions alone should provide an immediate and noticeable improvement in playing strength.