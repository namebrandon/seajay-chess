# GoogleTest Setup for SeaJay Chess Engine

## Overview

SeaJay uses GoogleTest for comprehensive unit testing. Since GoogleTest libraries are platform-specific, we maintain separate builds for Linux and macOS to ensure tests work correctly on both platforms.

## What the Tests Validate

### 1. Core Tests (No GoogleTest Required)
- **test_board**: Board representation, piece placement, move making/unmaking
- **test_fen_comprehensive**: FEN parsing and generation
- **test_fen_edge_cases**: Edge cases in FEN handling
- **test_board_safety**: Board state consistency and validation
- **test_en_passant_check_evasion**: En passant bug regression test
- **test_material_evaluation**: Material evaluation correctness
- **test_negamax**: Basic negamax search functionality

### 2. GoogleTest-Based Tests
- **test_alphabeta_validation**: Comprehensive validation that alpha-beta pruning produces identical results to basic negamax while reducing nodes searched by 90%+

## Platform-Specific Setup

### For Linux Development
GoogleTest is already set up in `/workspace/external/googletest/` and works out of the box.

### For macOS Development

#### One-Time Setup
Run this script on your macOS machine to build GoogleTest for ARM64:
```bash
./tools/scripts/setup_googletest_macos.sh
```

This will:
1. Download GoogleTest v1.14.0 source code
2. Build it specifically for macOS ARM64 architecture
3. Install libraries to `external/googletest-macos/`
4. Configure CMake to automatically detect and use these libraries

#### Building with Tests
After setting up GoogleTest for macOS:
```bash
./tools/scripts/build_for_macos.sh
```

The build will now include all tests, including the GoogleTest-based alpha-beta validation.

## Directory Structure

```
external/
├── googletest/           # Linux GoogleTest libraries
│   ├── lib/
│   │   ├── libgtest.a
│   │   └── libgtest_main.a
│   └── googletest/
│       └── include/
└── googletest-macos/     # macOS GoogleTest libraries (after setup)
    ├── lib/
    │   ├── libgtest.a
    │   └── libgtest_main.a
    └── include/
```

## How It Works

The CMakeLists.txt automatically detects the platform:
- **On Linux**: Uses `external/googletest/`
- **On macOS**: Looks for `external/googletest-macos/`
  - If not found, skips GoogleTest tests with a warning
  - Suggests running the setup script

## Benefits

1. **Platform Independence**: Each platform uses its own native libraries
2. **No Cross-Compilation Issues**: Avoids linker errors from mixed architectures
3. **Optional Testing**: Main engine builds even without GoogleTest
4. **Comprehensive Validation**: Tests ensure engine correctness across platforms

## Running Tests

### After Building
```bash
# Run all tests
cd build-macos  # or build/ on Linux
ctest

# Run specific test
./bin/test_alphabeta_validation

# Run with verbose output
ctest -V
```

### Test Output Example
```
Test project /Users/brandon/Documents/repos/seajay-chess/build-macos
    Start 1: board_tests
1/5 Test #1: board_tests ..................... Passed    0.01 sec
    Start 2: fen_comprehensive_tests
2/5 Test #2: fen_comprehensive_tests ......... Passed    0.02 sec
    Start 3: alphabeta_validation_tests
3/5 Test #3: alphabeta_validation_tests ...... Passed    1.23 sec
...
100% tests passed, 0 tests failed out of 5
```

## Troubleshooting

### "macOS GoogleTest not found" Warning
Run: `./tools/scripts/setup_googletest_macos.sh`

### Linker Errors About Archive Files
This means you're trying to use Linux libraries on macOS. Run the setup script.

### Tests Not Building
Check that GoogleTest is installed for your platform using the appropriate setup script.

## Why Platform-Specific Libraries?

GoogleTest libraries (`.a` files) contain compiled machine code that is specific to:
- **Architecture**: x86_64 (Linux) vs ARM64 (Apple Silicon)
- **Operating System**: Linux vs macOS system calls
- **ABI**: Application Binary Interface differences

Using the wrong platform's libraries causes linker errors like:
```
ld: archive member '/' not a mach-o file
```

This setup ensures each platform uses the correct native libraries for successful compilation and testing.