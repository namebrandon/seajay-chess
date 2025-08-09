# Building SeaJay for macOS

This directory contains scripts to build the SeaJay Chess Engine for macOS systems, particularly Apple Silicon (ARM64) Macs.

## Quick Start

### On macOS (Recommended)

If you're on a Mac, simply run:

```bash
./build_for_macos.sh
```

This will:
1. Detect you're on macOS
2. Configure CMake for Apple Silicon optimization
3. Build the engine
4. Create `seajay-macos` in the `bin-macos/` directory

### On Linux (Cross-compilation)

If you're on Linux and need a macOS binary:

```bash
# First time - see setup instructions
./cross_compile_macos.sh setup

# After osxcross is installed
./build_for_macos.sh
```

## Scripts Overview

### `build_for_macos.sh`
Universal wrapper that detects your platform and runs the appropriate build script.

### `build_macos.sh`
Native macOS build script with Apple Silicon optimizations:
- Uses clang++ with ARM64 optimizations
- Enables Link-Time Optimization (LTO)
- Strips debug symbols for smaller binary
- Tests the binary after building

**Usage:**
```bash
./build_macos.sh         # Normal build
./build_macos.sh clean   # Clean and rebuild
```

### `cross_compile_macos.sh`
Cross-compilation script for building macOS binaries on Linux:
- Requires osxcross toolchain
- Supports both x86_64 and ARM64 targets
- Creates portable macOS binary

**Usage:**
```bash
./cross_compile_macos.sh setup   # Show setup instructions
./cross_compile_macos.sh build   # Build the engine
./cross_compile_macos.sh clean   # Clean build files
```

## Prerequisites

### For macOS Native Build
- Xcode Command Line Tools: `xcode-select --install`
- CMake: `brew install cmake`
- C++20 compatible compiler (included with Xcode)

### For Linux Cross-Compilation
- osxcross toolchain (see setup instructions)
- CMake
- Clang/LLVM

## Output

All scripts create a binary named `seajay-macos` in the `bin-macos/` directory:
- **Location:** `/workspace/bin-macos/seajay-macos`
- **Suffix:** `-macos` to distinguish from Linux binary
- **Optimization:** Fully optimized for release with LTO

## Testing the Binary

### On macOS
```bash
# Test UCI protocol
echo -e "uci\nquit" | ./bin-macos/seajay-macos

# Run benchmark
./bin-macos/seajay-macos bench

# Use with GUI
# Add ./bin-macos/seajay-macos as engine in Arena, Cute Chess, etc.
```

### Copy to Mac from Linux
```bash
# After cross-compilation on Linux
scp bin-macos/seajay-macos user@your-mac:~/
```

## Architecture Support

- **Apple Silicon (M1/M2/M3):** Full native ARM64 optimization
- **Intel Macs:** Supported with x86_64 build
- **Universal Binary:** Not currently created (can be added if needed)

## Optimization Flags

The macOS build uses:
- `-O3`: Maximum optimization
- `-arch arm64`: Target Apple Silicon
- `-mtune=native`: CPU-specific tuning
- `-flto`: Link-time optimization
- `-ffast-math`: Fast floating-point math
- `-DNDEBUG`: Disable debug assertions

## Troubleshooting

### "No C++ compiler found"
Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### "CMake not found"
Install with Homebrew:
```bash
brew install cmake
```

### Cross-compilation fails
Ensure osxcross is properly installed and in PATH:
```bash
export PATH=/path/to/osxcross/target/bin:$PATH
```

### Binary doesn't run on older macOS
The minimum deployment target is macOS 11.0 (Big Sur). For older versions, modify the deployment target in `build_macos.sh`.

## Important Notes

1. **Separate Build Directory:** Uses `build-macos/` to avoid conflicts with Linux development
2. **Binary Naming:** Always creates `seajay-macos` to prevent confusion
3. **No Interference:** These scripts don't affect the main Linux development environment
4. **Testing:** The binary is automatically tested after building
5. **Performance:** Optimized specifically for Apple Silicon when built on ARM64 Macs

## Performance Expectations

On Apple Silicon Macs (M1/M2/M3), expect:
- Similar or better NPS compared to Linux on equivalent hardware
- Efficient power usage due to ARM64 optimization
- Full compatibility with macOS chess GUIs

## Contributing

When modifying these scripts:
1. Test on both macOS and Linux
2. Maintain the `-macos` suffix convention
3. Keep build directories separate
4. Document any new dependencies

---

*These scripts ensure SeaJay can be easily built and tested on macOS without interfering with the primary Linux development environment.*