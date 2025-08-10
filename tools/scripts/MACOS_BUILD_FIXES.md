# macOS Build Fixes - Stage 7 Update

## Problem Identified
Your macOS binary was built on August 9th BEFORE Stage 7 (Negamax Search) was completed. This means it only has material evaluation and plays essentially random moves based on material balance.

## Solution
The build script has been updated to ALWAYS do a clean build by default to prevent stale object files from causing issues.

## How to Build the Latest Version

### Quick Build (Recommended)
```bash
# From project root, this will do a clean build with Stage 7 search
./tools/scripts/build_for_macos.sh
```

### Manual Clean Build  
```bash
# If you want to be extra sure
rm -rf build-macos/ bin-macos/
./tools/scripts/build_for_macos.sh
```

### Verify You Have the Latest
After building, test the new binary:
```bash
echo -e "position startpos\ngo depth 4\nquit" | ./bin-macos/seajay-macos
```

You should see output showing it searches to depth 4:
```
info depth 1 ...
info depth 2 ...
info depth 3 ...
info depth 4 ...
bestmove ...
```

## What Changed in Stage 7
- **4-ply negamax search**: Now looks 4 moves ahead instead of 1
- **Tactical awareness**: Can find checkmates and tactical combinations
- **Iterative deepening**: Searches progressively deeper (1-4 ply)
- **Time management**: Uses 5% of remaining time + 75% of increment
- **+293 Elo improvement**: SPRT validated massive strength gain

## Build Script Improvements
1. **Default clean build**: Script now removes old build directories by default
2. **Override option**: Use `./build_macos.sh noclean` only if you're sure you want to skip cleaning
3. **Build timestamp**: Added timestamp to verify when binary was built
4. **Better error handling**: Script will fail loudly if something goes wrong

## Expected Strength
With Stage 7 complete:
- Should play tactical chess (finds simple tactics)
- Can deliver checkmate with sufficient material
- Estimated ~400 Elo strength (up from ~100 Elo)
- Still no advanced features (no pruning, no evaluation beyond material)

## Next Steps
Stage 8 (Alpha-Beta Pruning) will add:
- Significant search speedup (~80% node reduction)
- Deeper effective search
- Better move ordering
- Target: ~800 Elo strength