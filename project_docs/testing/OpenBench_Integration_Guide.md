# OpenBench Integration Guide

## Overview

This document describes the process of making SeaJay chess engine compatible with OpenBench testing framework by creating special testing branches from historical commits.

## Problem Statement

OpenBench requires engines to:
1. Build using GNU Make interface (`make EXE=name CXX=compiler`)
2. Support command-line bench execution (`./engine bench`)
3. Access specific commit versions by SHA hash

SeaJay originally used:
- CMake build system (not compatible with OpenBench)
- UCI-only bench command (no command-line support)
- Historical commits without build compatibility

## Solution: Testing Branches Strategy

### Concept

Instead of modifying the main development history, we create parallel "testing branches" that add OpenBench compatibility to specific historical commits while preserving the original development timeline.

### Naming Convention

Testing branches follow the pattern: `openbench/stageX`

Examples:
- `openbench/stage11` - Based on Stage 11 commit + OpenBench compatibility
- `openbench/stage12` - Based on Stage 12 commit + OpenBench compatibility

### Implementation Process

#### Step 1: Identify Historical Commits

Target commits for testing (examples):
```
f2ad9b5f - Stage 11: "docs: Complete Stage 11 checklist and documentation updates"
ffa0e442 - Stage 12: "feat: Stage 12 Transposition Tables - FINAL COMPLETION"
```

#### Step 2: Create Testing Branches

For each historical commit:

```bash
# Checkout the original commit
git checkout f2ad9b5f4bf40dd895088cd56b304262c76814e2

# Create new testing branch
git checkout -b openbench/stage11

# Add OpenBench compatibility (see below)
# ...

# Commit changes
git commit -m "build: Add OpenBench Makefile to Stage 11

Retroactively adding OpenBench-compatible Makefile to enable
testing of Stage 11 in OpenBench framework.

Original commit: f2ad9b5f4bf40dd895088cd56b304262c76814e2"

# Push to remote
git push origin openbench/stage11
```

#### Step 3: Add OpenBench Compatibility

Two components are required:

##### A. OpenBench-Compatible Makefile

Create `/workspace/Makefile` that wraps the CMake build system:

```makefile
# OpenBench-compatible Makefile for SeaJay Chess Engine
# This wraps the CMake build system for OpenBench integration

.PHONY: all clean

# OpenBench variables
CXX ?= g++
EXE ?= seajay
EVALFILE ?=

# Build configuration
BUILD_DIR = openbench-build
CMAKE_BUILD_TYPE = Release
QSEARCH_MODE = PRODUCTION

# Compiler flags for optimization
CXXFLAGS = -O3 -march=native -flto
CMAKE_CXX_FLAGS = $(CXXFLAGS)

all:
	@echo "Building SeaJay for OpenBench..."
	@echo "Compiler: $(CXX)"
	@echo "Output: $(EXE)"
	@mkdir -p $(BUILD_DIR)
	
	# Configure with CMake
	cmake -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_CXX_FLAGS="$(CMAKE_CXX_FLAGS)" \
		-DQSEARCH_MODE=$(QSEARCH_MODE) \
		-DEVALFILE="$(EVALFILE)"
	
	# Build the binary
	$(MAKE) -C $(BUILD_DIR) -j seajay
	
	# Copy to expected location for OpenBench
	@if [ -f "$(BUILD_DIR)/seajay" ]; then \
		cp -f "$(BUILD_DIR)/seajay" "$(EXE)"; \
		strip "$(EXE)"; \
		echo "Successfully built $(EXE)"; \
	else \
		echo "Error: Binary not found at $(BUILD_DIR)/seajay" >&2; \
		exit 1; \
	fi

clean:
	@echo "Cleaning OpenBench build..."
	@rm -rf "$(BUILD_DIR)"
	@rm -f "$(EXE)" seajay test-binary
	@echo "Clean complete"
```

##### B. Command-Line Bench Support

Modify the engine to support `./seajay bench` invocation:

**1. Add public method to UCIEngine (src/uci/uci.h):**
```cpp
class UCIEngine {
public:
    UCIEngine();
    void run();
    
    /**
     * Run benchmark from command line (for OpenBench compatibility)
     */
    void runBenchmark(int depth = 0);

private:
    // ... existing members
};
```

**2. Implement the method (src/uci/uci.cpp):**
```cpp
void UCIEngine::runBenchmark(int depth) {
    // Run the benchmark suite directly (for command-line invocation)
    // The benchmark will output to stdout if verbose=true
    auto result = BenchmarkSuite::runBenchmark(depth, true);
    
    // Also send final summary as info string (for OpenBench parsing)
    std::ostringstream oss;
    oss << "Benchmark complete: " << result.totalNodes << " nodes, "
        << std::fixed << std::setprecision(0) << result.averageNps() << " nps";
    sendInfo(oss.str());
}
```

**3. Update main.cpp for command-line arguments:**
```cpp
int main(int argc, char* argv[]) {
    try {
        // Check for command-line arguments first
        if (argc > 1) {
            std::string command(argv[1]);
            
            if (command == "bench") {
                // Run benchmark and exit (for OpenBench compatibility)
                seajay::UCIEngine engine;
                int depth = 0;
                if (argc > 2) {
                    try {
                        depth = std::stoi(argv[2]);
                    } catch (...) {
                        depth = 0;
                    }
                }
                engine.runBenchmark(depth);
                return 0;
            }
        }
        
        // No arguments or unrecognized argument - start normal UCI loop
        seajay::UCIEngine engine;
        engine.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

#### Step 4: Update Commit Hashes

After creating the testing branches, get the new commit hashes:

```bash
git rev-parse openbench/stage11  # e583c031a1b7030b262a4574992acf6138f36903
git rev-parse openbench/stage12  # 7a5321dff7e9d9772252eb774f6defe7ec3a04a3
```

Use these new hashes in OpenBench test configurations instead of the original commit hashes.

## Results

### Before Integration
- **Original Stage 11**: `f2ad9b5f4bf40dd895088cd56b304262c76814e2` ❌ Cannot build in OpenBench
- **Original Stage 12**: `ffa0e442298728ad22acb6e642e4621840a735cd` ❌ Cannot build in OpenBench

### After Integration
- **Testing Stage 11**: `8ed52cf9552152fd780ebcb4d06d41d93dd9f840` ✅ OpenBench compatible
- **Testing Stage 12**: `407839997bd898706752b2eda77f90049775a634` ✅ OpenBench compatible

## Verification

Test the integration locally:

```bash
# Test Makefile build
make clean
make EXE=test-engine CXX=g++ -j4

# Test command-line bench
./test-engine bench

# Expected output:
# Total: 19191913 nodes in 3.844s (4993285 nps)
# info string Benchmark complete: 19191913 nodes, 4993285 nps
```

## Benefits

1. **Preserves History**: Original development commits remain unchanged
2. **Enables Testing**: Historical versions can now be tested in OpenBench
3. **Backward Compatible**: UCI bench command still works normally
4. **Future-Proof**: All new development inherits OpenBench compatibility
5. **Clean Separation**: Testing infrastructure is clearly separate from core development

## OpenBench Configuration

Update your OpenBench test to use the new testing branch commit hashes:

```json
{
  "base_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "8ed52cf9552152fd780ebcb4d06d41d93dd9f840"
  },
  "dev_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess", 
    "commit": "407839997bd898706752b2eda77f90049775a634"
  }
}
```

## Best Practices

1. **Consistent Naming**: Use `openbench/stageX` format for all testing branches
2. **Clear Commit Messages**: Reference the original commit hash in the testing branch commit
3. **Identical Makefiles**: Use the same Makefile across all testing branches for consistency
4. **Local Testing**: Always verify builds and bench commands work before pushing
5. **Documentation**: Keep mapping between original commits and testing commits documented

## Future Stages

To add OpenBench compatibility to additional stages:

1. Identify the target commit hash
2. Create new testing branch: `openbench/stageX`
3. Add the same Makefile and bench support
4. Test locally
5. Push and get new commit hash
6. Update OpenBench configurations

This process can be repeated for any historical commit that needs OpenBench testing capability.

---

*This integration enables SeaJay to participate in automated testing frameworks while maintaining clean development history and backward compatibility.*