# Countermoves Recovery - Action Plan Summary

## Your Strategy Summary

✅ **Approach:** Micro-phasing to isolate regression root cause  
✅ **Method:** Break CM3 into 3-4 smaller phases, test each via OpenBench  
✅ **Goal:** Identify exact cause, then decide: fix or rewrite with Ethereal approach  
✅ **Context:** Known invalid move bug exists (~1-2/500 games) but likely unrelated  

## Short-Term Action Plan

### Today (Integration Branch Created)
1. ✅ Created `integration/countermoves-micro-phasing` branch
2. ✅ Documented micro-phasing strategy
3. ✅ Updated integration branches tracker

### Next Steps (Ready to Execute)

#### Step 1: Reset to Known Good State
```bash
git reset --hard 40490c6  # CM2: Shadow updates, no regression
```

#### Step 2: Implement Micro-Phases

**CM3.1 - UCI Option Only**
- Add CountermoveBonus UCI option
- NO move ordering changes
- Commit: "feat: Add countermove bonus UCI option only (CM3.1) - bench X"
- OpenBench test → Expect 0 ELO

**CM3.2 - Lookup Without Bonus**
- Add countermove retrieval in move_ordering
- Score remains 0 (no actual bonus)
- Commit: "feat: Add countermove lookup without bonus (CM3.2) - bench X"
- OpenBench test → Expect 0 to -1 ELO

**CM3.3 - Minimal Bonus**
- Apply bonus of 100 (tiny compared to history's 8192)
- Commit: "feat: Apply minimal countermove bonus 100 (CM3.3) - bench X"
- OpenBench test → Expect 0 to +1 ELO

**CM3.4 - Incremental Increases**
- Test 500, 1000, 2000, 4000, 8000
- Stop at first regression
- Identify breaking point

#### Step 3: Analyze Results

**Decision Matrix:**
- CM3.1 fails → UCI bug
- CM3.2 fails → Indexing/lookup bug  
- CM3.3 fails → Integration bug
- CM3.4 fails at X → Scoring conflict at threshold X
- All pass → Original had compound bugs

#### Step 4: Resolution

**If bug isolated:** Fix specific issue  
**If design flaw:** Implement Ethereal's simpler approach  
**If unclear:** Add more diagnostic micro-phases  

## Key Points

1. **Each micro-phase MUST be tested** before proceeding
2. **Stop immediately** at first regression to debug
3. **Document everything** for future reference
4. **Invalid move bug** is noted but likely separate issue
5. **Patience required** - 2-3 days for full analysis

## Commands to Start

```bash
# You're already on integration/countermoves-micro-phasing
# Reset to CM2
git reset --hard 40490c6

# Verify it builds and get bench
./build.sh
echo "bench" | ./bin/seajay | grep "Benchmark complete"

# Start implementing CM3.1
```

## Success Criteria

✅ Root cause identified to specific code change  
✅ Clear fix or rewrite path determined  
✅ No compound bugs hiding issues  
✅ Countermoves working with +25-40 ELO gain  

---

**Ready to proceed?** The micro-phasing approach will systematically isolate the issue.