# SeaJay Chess Engine - Version History

**Current Version:** 1.5.0-master  
**Release Date:** August 9, 2025  
**Estimated Strength:** ~100 ELO (random play baseline)  
**Status:** Phase 1 Complete, Ready for Phase 2  

## Version Numbering System

Format: **MAJOR.MINOR.PATCH-TAG**
- **MAJOR**: Phase number (1-7)
- **MINOR**: Stage within phase (0-9)
- **PATCH**: Incremental improvements (0-99)
- **TAG**: Feature identifier or status

## Release History

### Phase 1: Foundation and Move Generation

#### v1.5.0-master (Current)
- **Date:** August 9, 2025
- **Milestone:** Phase 1 Complete
- **Features:**
  - Complete testing infrastructure
  - Benchmark command (12 positions)
  - Automated test scripts
  - SPRT framework ready
  - fast-chess integration
- **Performance:** ~7.7M NPS benchmark average
- **Testing:** All regression tests passing

#### v1.4.0
- **Date:** August 9, 2025
- **Stage:** Special Moves and Validation
- **Features:**
  - Castling (including Chess960 ready)
  - En passant captures
  - Pawn promotions (Q/R/B/N)
  - Make/unmake with state restoration
  - Perft implementation
- **Accuracy:** 99.974% perft accuracy
- **Testing:** 24/25 perft tests passing

#### v1.3.0
- **Date:** August 8, 2025
- **Stage:** Basic UCI and Legal Moves
- **Features:**
  - Full UCI protocol implementation
  - Complete legal move generation
  - Move filtering for king safety
  - Pin detection
  - Check evasion
  - Random move selection
- **Compatibility:** Works with all UCI GUIs
- **Testing:** Full UCI protocol implementation

#### v1.2.0
- **Date:** August 2025
- **Stage:** Position Management
- **Features:**
  - Enhanced FEN parser with Result<T,E>
  - Parse-validate-swap pattern
  - Buffer overflow protection
  - Zobrist key management
  - Bitboard sync validation
- **Testing:** 337+ unit tests passing

#### v1.1.0
- **Date:** December 2024
- **Stage:** Board Representation
- **Features:**
  - Hybrid bitboard-mailbox structure
  - 12 piece bitboards
  - Occupied/empty bitboards
  - Basic ray-based move generation
  - Zobrist hashing infrastructure
- **Architecture:** Foundation established

#### v1.0.0
- **Date:** December 2024
- **Stage:** Project Initialization
- **Features:**
  - Project structure created
  - Build system (CMake)
  - Development environment
  - Master Project Plan
- **Status:** Development begun

## Future Versions (Planned)

### Phase 2: Basic Search and Evaluation (~1500 ELO)

#### v2.1.0-material (Planned)
- Material evaluation
- Piece values (P=100, N=320, B=330, R=500, Q=900)
- Single-ply evaluation
- **Expected:** +800 ELO

#### v2.2.0-negamax (Planned)
- Negamax search framework
- 4-ply fixed depth
- Basic time management
- **Expected:** +200-300 ELO

#### v2.3.0-alphabeta (Planned)
- Alpha-beta pruning
- Search tree reduction
- **Expected:** +100-150 ELO

#### v2.4.0-pst (Planned)
- Piece-square tables
- Positional evaluation
- Tapered evaluation
- **Expected:** +150-200 ELO

### Phase 3: Essential Optimizations (~2100 ELO)

#### v3.1.0-magic (Planned)
- Magic bitboards for sliding pieces
- **Expected:** 3-5x speedup

#### v3.2.0-ordering (Planned)
- Move ordering (MVV-LVA)
- **Expected:** +50-100 ELO

#### v3.3.0-tt (Planned)
- Transposition tables
- **Expected:** +100-150 ELO

#### v3.4.0-id (Planned)
- Iterative deepening
- **Expected:** +50-100 ELO

#### v3.5.0-quiescence (Planned)
- Quiescence search
- **Expected:** +100-150 ELO

### Phase 4: NNUE Integration (~2800 ELO)

#### v4.0.0-nnue (Planned)
- NNUE evaluation
- Pre-trained network
- **Expected:** +700 ELO

### Phase 5-7: Advanced Techniques (3200+ ELO)

- Advanced search techniques
- Custom NNUE training
- Endgame tablebases
- Opening book generation

## Performance Milestones

| Version | Phase | NPS | Perft Accuracy | Strength |
|---------|-------|-----|----------------|----------|
| 1.5.0 | 1 | 7.7M | 99.974% | ~100 ELO |
| 2.0.0 | 2 | 500K | 100% | ~1500 ELO |
| 3.0.0 | 3 | >1M | 100% | ~2100 ELO |
| 4.0.0 | 4 | >1M | 100% | ~2800 ELO |
| 7.0.0 | 7 | >2M | 100% | 3200+ ELO |

## Breaking Changes

### v1.0.0 → v2.0.0
- Search function added to UCI
- Evaluation replaces random moves
- Time management implemented

### v2.0.0 → v3.0.0
- Magic bitboards replace ray-based generation
- Transposition table memory requirements

### v3.0.0 → v4.0.0
- NNUE evaluation replaces classical
- Network file requirement
- Memory usage increase

## Deprecation Notices

### Phase 3
- Ray-based move generation (replaced by magic bitboards)

### Phase 4
- Classical evaluation (replaced by NNUE)

## Build Requirements by Version

| Version | C++ Standard | CMake | Compiler | RAM |
|---------|-------------|-------|----------|-----|
| 1.x | C++20 | 3.16+ | GCC 10+ | 256MB |
| 2.x | C++20 | 3.16+ | GCC 10+ | 512MB |
| 3.x | C++20 | 3.16+ | GCC 10+ | 1GB |
| 4.x | C++20 | 3.16+ | GCC 10+ | 2GB |

## Support Policy

- **Current Phase:** Full support and active development
- **Previous Phase:** Bug fixes only
- **Older Phases:** Historical reference only

## Release Notes Format

Each release includes:
1. Version number and date
2. List of changes
3. SPRT test results (Phase 2+)
4. Performance metrics
5. Known issues
6. Upgrade instructions

## Git Tags

All releases are tagged in git:
```bash
git tag -a v1.5.0 -m "Phase 1 Complete - Testing Infrastructure"
git push origin v1.5.0
```

## Changelog Generation

Generate changelog between versions:
```bash
git log --oneline v1.4.0..v1.5.0
```

## Archive Location

Historical versions archived at:
- Source: `/workspace/archive/releases/`
- Binaries: `/workspace/archive/binaries/`
- Test Results: `/workspace/sprt_results/archive/`

---

*This document tracks SeaJay's evolution from foundation to competitive strength. Each version represents validated improvements in the journey to 3200+ ELO.*