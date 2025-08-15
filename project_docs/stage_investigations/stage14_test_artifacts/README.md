# Stage 14 Test Artifacts and Build Scripts

## Permanent Build Scripts (in /workspace/)

These scripts were created for Stage 14 and remain as permanent utilities:

### build_testing.sh
- **Purpose**: Build Stage 14 with 10K node limit for rapid testing
- **Usage**: `./build_testing.sh`
- **Mode**: QSEARCH_TESTING with 10K quiescence node limit
- **Use Case**: Development iteration and debugging

### build_tuning.sh  
- **Purpose**: Build Stage 14 with 100K node limit for parameter tuning
- **Usage**: `./build_tuning.sh`
- **Mode**: QSEARCH_TUNING with 100K quiescence node limit
- **Use Case**: Parameter experimentation and A/B testing

### build_production.sh
- **Purpose**: Build Stage 14 with unlimited nodes for competition
- **Usage**: `./build_production.sh` or `./build.sh production`
- **Mode**: QSEARCH_PRODUCTION with no limits
- **Use Case**: SPRT testing and competitive play

## Test Files Created During Investigation

The following test files were created during Stage 14 development to investigate the "illegal move" issue:

- `test_stage14_illegal_move.cpp` - Main test reproducing the SPRT position
- `test_illegal_king_move.cpp` - Alternative test implementation  
- `test_illegal_bug.cpp` - Initial investigation test
- `test_c10_illegal_move_bug.cpp` - C10-specific validation

**Result**: All tests confirmed SeaJay's move generation is correct. The reported "illegal move" was determined to be a fastchess GUI issue or misreporting.

## Build Mode Selection

Stage 14 introduces build mode selection for different development phases:

```bash
# Quick mode selection
./build.sh testing      # 10K limit
./build.sh tuning       # 100K limit  
./build.sh production   # No limits (default)

# Or dedicated scripts
./build_testing.sh      # Testing mode
./build_tuning.sh       # Tuning mode
./build_production.sh   # Production mode
```

The engine displays its mode at UCI startup to prevent confusion.

## Archive Note

These artifacts document the extensive testing and validation performed during Stage 14 development, including the resolution of multiple critical issues (ENABLE_QUIESCENCE flag, C9 delta catastrophe, illegal move investigation).