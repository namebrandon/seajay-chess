# UCI Threads Option Implementation Plan for SeaJay

## Executive Summary
This document outlines the implementation plan for adding the UCI "Threads" option to SeaJay chess engine. While multi-threading is not yet implemented, this stub implementation will ensure OpenBench compatibility and prepare the infrastructure for future parallelization.

## Requirements

### Functional Requirements
1. Accept and parse the UCI "Threads" option via `setoption` command
2. Display the option in UCI initialization with proper format
3. Accept values from 1 to 1024 (per Stockfish standard)
4. Currently enforce single-threaded operation (always use 1 thread internally)
5. Store the requested value for future use when multi-threading is implemented

### UCI Protocol Compliance
- **Format**: `option name Threads type spin default 1 min 1 max 1024`
- **Command**: `setoption name Threads value <n>`
- **Default**: 1 thread
- **Range**: 1-1024 (accept all values, but internally use only 1)

## Current State Analysis

### Existing UCI Implementation
- Location: `/workspace/src/uci/uci.h` and `/workspace/src/uci/uci.cpp`
- UCI options are declared in `handleUCI()` method (lines 64-119 in uci.cpp)
- Options are parsed in `handleSetOption()` method (lines 607+ in uci.cpp)
- Current options include:
  - Various search parameters (LMR, Null Move, Quiescence)
  - Transposition table settings
  - Aspiration window parameters
  - SEE modes and pruning options

### Pattern to Follow
The existing implementation uses:
1. Member variable in UCIEngine class to store option value
2. Output option declaration in `handleUCI()`
3. Parse and store value in `handleSetOption()`
4. Optional info string output for confirmation

## Implementation Details

### Phase 1: Infrastructure (No functional change)
**Files to modify:**
1. `src/uci/uci.h`
   - Add member variable: `int m_threads = 1;`
   - Add comment explaining stub nature

2. `src/uci/uci.cpp` - `handleUCI()` method
   - Add Threads option declaration after line 96 (Hash option)
   - Format: `std::cout << "option name Threads type spin default 1 min 1 max 1024" << std::endl;`
   - Add comment: `// Multi-threading stub for OpenBench compatibility`

### Phase 2: Option Handling
**File to modify:**
1. `src/uci/uci.cpp` - `handleSetOption()` method
   - Add new else-if block after existing option handlers
   - Parse integer value
   - Store in `m_threads` variable
   - Output info string confirming the setting
   - Add warning if value > 1: "Multi-threading not yet implemented, using 1 thread"

### Phase 3: Documentation and Testing
1. Add inline documentation explaining:
   - This is a stub implementation for OpenBench compatibility
   - Multi-threading will be implemented in a future stage
   - Currently always uses 1 thread regardless of setting

2. Test cases:
   - `setoption name Threads value 1` - Should accept silently
   - `setoption name Threads value 4` - Should accept with warning
   - `setoption name Threads value 1024` - Should accept with warning
   - `setoption name Threads value 0` - Should be ignored (invalid)
   - `setoption name Threads value 1025` - Should be ignored (out of range)

## Code Changes

### 1. uci.h additions (around line 73, after countermove bonus)
```cpp
// Multi-threading support (stub for OpenBench compatibility)
int m_threads = 1;  // Number of threads requested (currently always uses 1)
```

### 2. uci.cpp - handleUCI() additions (after line 96)
```cpp
// Multi-threading option (stub for OpenBench compatibility)
std::cout << "option name Threads type spin default 1 min 1 max 1024" << std::endl;
```

### 3. uci.cpp - handleSetOption() additions (in the option handling section)
```cpp
// Handle Threads option (multi-threading stub)
else if (optionName == "Threads") {
    try {
        int threads = std::stoi(value);
        if (threads >= 1 && threads <= 1024) {
            m_threads = threads;
            if (threads == 1) {
                std::cerr << "info string Threads set to 1" << std::endl;
            } else {
                std::cerr << "info string Threads set to " << threads 
                          << " (multi-threading not yet implemented, using 1 thread)" << std::endl;
            }
        } else {
            std::cerr << "info string Invalid Threads value: " << value 
                      << " (must be between 1 and 1024)" << std::endl;
        }
    } catch (...) {
        std::cerr << "info string Invalid Threads value: " << value << std::endl;
    }
}
```

## Testing Plan

### Manual Testing via UCI
```bash
# Test 1: Check option is listed
echo "uci" | ./bin/seajay | grep Threads
# Expected: option name Threads type spin default 1 min 1 max 1024

# Test 2: Set to 1 thread
echo -e "uci\nsetoption name Threads value 1\nquit" | ./bin/seajay
# Expected: info string Threads set to 1

# Test 3: Set to multiple threads
echo -e "uci\nsetoption name Threads value 4\nquit" | ./bin/seajay
# Expected: info string Threads set to 4 (multi-threading not yet implemented, using 1 thread)

# Test 4: Invalid values
echo -e "uci\nsetoption name Threads value 0\nquit" | ./bin/seajay
# Expected: info string Invalid Threads value: 0 (must be between 1 and 1024)

echo -e "uci\nsetoption name Threads value 2000\nquit" | ./bin/seajay
# Expected: info string Invalid Threads value: 2000 (must be between 1 and 1024)
```

### OpenBench Compatibility Test
After implementation, verify that OpenBench can:
1. Detect the Threads option
2. Set it to various values without errors
3. Run matches successfully regardless of thread setting

## Future Considerations

### When Multi-threading is Implemented
The `m_threads` variable will be used to:
1. Create worker thread pool in search initialization
2. Implement parallel search algorithms (Lazy SMP or similar)
3. Manage thread-safe transposition table access
4. Coordinate move ordering across threads
5. Implement thread-specific data structures

### Migration Path
1. Current: Stub accepts value but uses 1 thread
2. Future Stage: Implement thread pool infrastructure
3. Future Stage: Add parallel search with proper work distribution
4. Future Stage: Optimize thread synchronization and communication

## Risk Assessment

### Low Risk
- Simple addition to existing UCI infrastructure
- No impact on search or evaluation
- Follows established patterns in codebase
- Easily testable

### Mitigation
- Clear documentation that this is a stub
- Warning messages when threads > 1 requested
- No changes to actual search behavior

## Implementation Checklist

- [ ] Add m_threads member variable to uci.h
- [ ] Add Threads option output in handleUCI()
- [ ] Implement parsing in handleSetOption()
- [ ] Add appropriate comments and documentation
- [ ] Test all value ranges (valid and invalid)
- [ ] Verify OpenBench compatibility
- [ ] Update commit message with proper format
- [ ] Run bench and include in commit message

## Commit Message Template
```
feat: Add UCI Threads option stub for OpenBench compatibility

- Implements UCI Threads option (1-1024 range)
- Currently stub implementation (always uses 1 thread)
- Required for OpenBench test compatibility
- Prepares infrastructure for future multi-threading

bench: [node-count]
```

## Conclusion
This implementation provides the minimal changes needed to support the UCI Threads option for OpenBench compatibility while maintaining single-threaded operation. The stub is designed to be easily extended when multi-threading is implemented in a future development stage.