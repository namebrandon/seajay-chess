# UCI Score Conversion Regression Analysis

## Executive Summary
A 30-40 ELO regression was introduced between commits 855c4b9 and 7f646e4 when implementing UCI score conversion from negamax (side-to-move) perspective to UCI standard (White's perspective). After analysis, including the attempted fix in commit c75a2e4, the regression persists.

## Investigation Timeline

### Initial Problem (Commit 7f646e4)
- Implemented UCI score conversion using `board.sideToMove()` 
- This was wrong because `board.sideToMove()` changes during search as moves are made/unmade
- Caused scores to flip perspective incorrectly during search

### Attempted Fix (Commit c75a2e4)  
- Added `rootSideToMove` to SearchData structure
- Properly initialized at search start
- Changed info output calls to use `info.rootSideToMove` instead of `board.sideToMove()`
- **BUT THE REGRESSION PERSISTS**

## Current Analysis - Why the Fix Didn't Work

After examining the current code state, the fix appears to be properly implemented:
1. ✅ `rootSideToMove` is stored in SearchData at search start
2. ✅ All info output calls now use `info.rootSideToMove`
3. ✅ The same `info` structure is passed through all recursive calls
4. ✅ The conversion logic itself is mathematically correct

## Hypothesis: The REAL Problem

Since the obvious fix has been applied but the regression persists, we need to look deeper. The issue is likely one of the following:

### 1. **Score Recording/Comparison Issue** (MOST LIKELY)
The problem may be in how scores are being recorded and compared internally:

- In `recordInfoSent()`, the score is stored as-is: `m_scoreAtLastInfo = score`
- In `shouldSendInfo()`, score changes are detected: `abs((score - m_scoreAtLastInfo).value()) >= SCORE_CHANGE_THRESHOLD`
- **PROBLEM**: If scores are being converted to UCI perspective BEFORE being stored/compared, the internal negamax logic could be affected

### 2. **Double Conversion Issue**
Check if scores are being converted multiple times:
- Once in InfoBuilder::appendScore()
- Perhaps again somewhere else?
- This could cause scores to flip incorrectly

### 3. **Aspiration Window Contamination**
The aspiration window logic might be using UCI-converted scores instead of raw negamax scores:
- Check if the displayed scores are somehow feeding back into the search
- The alpha/beta bounds must remain in negamax perspective

### 4. **Time Management Impact**
Time management decisions might be based on UCI-converted scores:
- Score trends used for time allocation
- Fail-high/fail-low detection

## Deep Dive: The Real Culprit

Looking at the InfoBuilder implementation:

```cpp
InfoBuilder& InfoBuilder::appendScore(eval::Score score, Color sideToMove, ScoreBound bound) {
    // Convert from negamax (side-to-move perspective) to UCI (White's perspective)
    if (score.is_mate_score()) {
        // ... mate handling ...
        return appendMateScore(mateIn, sideToMove);
    } else {
        // Convert centipawn score to White's perspective
        int cp = score.to_cp();
        if (sideToMove == BLACK) {
            cp = -cp;  // Negate for Black to get White's perspective
        }
        return appendCentipawnScore(cp, bound);
    }
}
```

**This looks correct for displaying scores, but...**

## THE CRITICAL ISSUE: Score Feedback Loop

After careful analysis, I believe the issue is in `iterativeInfo->recordInfoSent(iterativeInfo->bestScore)`:

```cpp
// Line 153 and 1081
iterativeInfo->recordInfoSent(iterativeInfo->bestScore);  // Recording NEGAMAX score
```

But in `shouldSendInfo()`:
```cpp
bool shouldSendInfo(bool checkScoreChange = false) const {
    // ...
    if (checkScoreChange && m_scoreAtLastInfo != eval::Score::zero()) {
        if (abs((bestScore - m_scoreAtLastInfo).value()) >= SCORE_CHANGE_THRESHOLD) {
            return true;  // Force update on significant score change
        }
    }
}
```

**The score being checked (bestScore) is the internal negamax score, which is correct.**

## Alternative Theory: The Eval Command

The eval command was also modified in commit 7f646e4. Let's check if the static evaluation is somehow affecting the search:

```cpp
void UCIEngine::handleEval() {
    // UCI Score Conversion: Convert to White's perspective if Black to move
    eval::EvalBreakdown uciBreakdown = breakdown;
    if (m_board.sideToMove() == BLACK) {
        // Negate all components for White's perspective
        uciBreakdown.total = -uciBreakdown.total;
        // ... etc ...
    }
}
```

**This only affects display, shouldn't impact search.**

## Most Likely Culprit: Transposition Table Contamination

One possibility not yet explored: Are UCI-converted scores somehow getting stored in the transposition table?

The TT should only store raw negamax scores, but if converted scores are being stored, this would cause severe search problems.

## Recommended Next Steps

1. **Add Debug Logging**
   - Log the actual score values at each info output
   - Verify scores are only converted for display, never internally

2. **Check Transposition Table**
   - Ensure no UCI-converted scores are stored in TT
   - Verify TT retrieval doesn't involve any conversion

3. **Test Without Info Output**
   - Temporarily disable all UCI info output during search
   - See if regression disappears (would confirm it's related to info output)

4. **Compare Move Selection**
   - Run same position with both versions
   - Check if different moves are selected at root
   - This would indicate search tree corruption

5. **Verify InfoBuilder Isolation**
   - Ensure InfoBuilder doesn't modify the original score
   - Check for any reference/pointer issues

## Conclusion

The regression persists despite the apparent fix because:
1. The obvious fix (using rootSideToMove) has been applied correctly
2. The conversion logic itself is mathematically sound
3. Something else is being affected by the UCI conversion

The most likely issue is that somewhere in the code, UCI-converted scores are leaking back into the internal search logic, possibly through:
- Transposition table storage
- Aspiration window bounds
- Time management decisions
- Move ordering heuristics

The next step should be to add extensive debug logging to track exactly where and how scores are being used throughout the search.