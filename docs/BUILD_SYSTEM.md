# SeaJay Build System Documentation

## Overview
SeaJay uses two build systems optimized for different purposes:
- **Makefile**: For OpenBench testing and production builds
- **CMakeLists.txt + build.sh**: For local development

## Recent Changes (2025-08-29)

### Compiler Optimization Update
The build system has been updated to use automatic CPU capability detection instead of hardcoded instruction sets.

**Key Changes:**
- Switched from `-O2` to `-O3` for better optimization
- Added `-march=native` for automatic CPU detection
- Added `-flto` for link-time optimization
- Removed hardcoded `-mavx2 -mbmi2` flags

**Performance Impact:**
- 14% NPS improvement in initial testing
- Verified working on Ryzen, Xeon, and Core i9 processors

## Build Methods

### 1. OpenBench / Production Build
```bash
make clean
make
```
This uses the Makefile with auto-detection of CPU capabilities.

### 2. Local Development Build
```bash
rm -rf build
./build.sh [Debug|Release|RelWithDebInfo]
```
This uses CMake for easier development workflow.

### 3. Quick Release Build
```bash
rm -rf build && ./build.sh Release
```

## Build Types

### Release
- Full optimizations (`-O3 -march=native -flto`)
- No debug symbols
- Maximum performance
- Use for: Testing, benchmarking, playing

### Debug
- No optimizations (`-O0`)
- Full debug symbols (`-g`)
- Assertions enabled
- Use for: Debugging, development

### RelWithDebInfo
- Optimizations (`-O2`)
- Debug symbols included
- Use for: Profiling, debugging optimized code

## Important Notes

### CPU Instruction Set Support
- The build system now auto-detects CPU capabilities using `-march=native`
- This ensures optimal performance on any x86-64 CPU
- No need to manually specify AVX2, BMI2, SSE4.2, etc.

### OpenBench Compatibility
- OpenBench always uses the Makefile
- Any optimization flags must be added to BOTH Makefile and CMakeLists.txt
- The Makefile is the authoritative build configuration for testing

### Development Environment Limitations
Some development environments may have limited instruction set support:
- Check with: `gcc -march=native -Q --help=target | grep -E "(avx|sse|bmi)"`
- If limited, the auto-detection will use only available instructions

## Troubleshooting

### Build Failures
```bash
# Clean everything and rebuild
make clean
rm -rf build
./build.sh Release
```

### Performance Issues
```bash
# Verify optimization flags are applied
make clean
make VERBOSE=1
```

### Checking Binary Optimization
```bash
# Check if binary uses expected instructions
objdump -d bin/seajay | grep -c -E "(avx|bmi|pext|pdep)"
```

## Benchmarking

Always use Release builds for benchmarking:
```bash
rm -rf build && ./build.sh Release
echo "bench" | ./bin/seajay
```

Expected NPS (as of 2025-08-29):
- ~500K nps on modern CPUs
- Benchmark: 19191913 nodes