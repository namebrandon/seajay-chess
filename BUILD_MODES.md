# SeaJay Build Modes Guide

## Quick Start

SeaJay supports three quiescence search build modes to optimize for different use cases:

### For Development and Testing
```bash
./build_testing.sh    # 10K node limit - fast iteration
```

### For Parameter Tuning
```bash
./build_tuning.sh     # 100K node limit - balanced
```

### For Competition and SPRT
```bash
./build_production.sh # No limits - full strength
```

## Build Mode Details

### TESTING Mode
- **Node Limit:** 10,000 nodes per quiescence search
- **Purpose:** Rapid development iteration
- **Build Time:** Fastest
- **Test Time:** Fastest
- **Use When:**
  - Debugging search issues
  - Developing new features
  - Running quick validation tests
  - Iterating on code changes

### TUNING Mode
- **Node Limit:** 100,000 nodes per quiescence search
- **Purpose:** Finding optimal parameters
- **Build Time:** Same as testing
- **Test Time:** Moderate
- **Use When:**
  - Tuning evaluation parameters
  - Testing parameter trade-offs
  - Validating heuristic changes
  - Running medium-length tests

### PRODUCTION Mode
- **Node Limit:** Unlimited (full quiescence)
- **Purpose:** Maximum playing strength
- **Build Time:** Same as others
- **Test Time:** Slowest (but most accurate)
- **Use When:**
  - Running SPRT tests
  - Playing competitive matches
  - Final validation before release
  - Measuring true engine strength

## Build Methods

### Method 1: Dedicated Scripts (Recommended)
```bash
./build_testing.sh      # Clear and simple
./build_tuning.sh       # No parameters needed
./build_production.sh   # Self-documenting
./build_debug.sh        # Debug with sanitizers
```

### Method 2: Parameterized Script
```bash
./build.sh testing      # Same as build_testing.sh
./build.sh tuning       # Same as build_tuning.sh
./build.sh production   # Same as build_production.sh (default)
```

### Method 3: Direct CMAKE
```bash
cd build
cmake -DQSEARCH_MODE=TESTING ..
cmake -DQSEARCH_MODE=TUNING ..
cmake -DQSEARCH_MODE=PRODUCTION ..
make -j
```

## Verifying Build Mode

### At Build Time
The build scripts display the mode being built:
```
==========================================
Building SeaJay - TESTING MODE
Quiescence Search: 10K node limit
Purpose: Rapid testing and validation
==========================================
```

### From Binary
Use the check_mode.sh script:
```bash
./check_mode.sh           # Check default binary
./check_mode.sh ./bin/seajay  # Check specific binary
```

### At Runtime
The engine displays its mode in UCI:
```bash
echo 'uci' | ./bin/seajay
# Output includes:
# id name SeaJay Stage-13-FINAL (Quiescence: TESTING MODE - 10K limit)
```

## Important Notes

1. **Always verify mode before testing:** Different modes produce different results
2. **Clean builds when switching:** The scripts automatically clean to prevent issues
3. **SPRT requires PRODUCTION:** Never run SPRT tests with limited modes
4. **Document your mode:** When reporting results, always mention which mode was used

## Debug Build

For memory safety and debugging:
```bash
./build_debug.sh
```

This builds with:
- AddressSanitizer
- UndefinedBehaviorSanitizer
- Debug symbols
- TESTING mode (for faster debugging)

## Troubleshooting

### Wrong Results?
Check you're using the right mode:
```bash
./check_mode.sh
```

### Build Errors?
Clean and rebuild:
```bash
rm -rf build/
./build_production.sh  # or your desired mode
```

### Unsure Which Mode?
The engine always shows its mode at startup. If in doubt, run:
```bash
echo 'uci' | ./bin/seajay | head -3
```

## Development Workflow

1. **Initial Development:** Use TESTING mode
   ```bash
   ./build_testing.sh
   # Make changes, test quickly
   ```

2. **Parameter Tuning:** Switch to TUNING mode
   ```bash
   ./build_tuning.sh
   # Find optimal values
   ```

3. **Final Validation:** Use PRODUCTION mode
   ```bash
   ./build_production.sh
   # Run SPRT or competitive tests
   ```

## Mode Selection Guidelines

| Task | Recommended Mode | Reason |
|------|-----------------|---------|
| Bug fixing | TESTING | Fast iteration |
| Feature development | TESTING | Quick validation |
| Parameter search | TUNING | Balance speed/accuracy |
| SPRT testing | PRODUCTION | True strength |
| Tournament play | PRODUCTION | Maximum performance |
| CI/CD testing | TESTING | Fast feedback |
| Release validation | PRODUCTION | Final verification |