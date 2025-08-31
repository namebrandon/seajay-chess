# SPSA UCI Endgame PST Tuning Guide

## Overview
This document outlines the approach for SPSA tuning of PST (Piece-Square Table) endgame values in SeaJay using OpenBench. Given the regression observed with manual tuning attempts, SPSA provides a data-driven approach to finding optimal mg/eg differentials.

## Current State
- **Infrastructure**: Phase interpolation system implemented and working (+46.50 ELO)
- **Values**: Most pieces have identical mg/eg values (except King)
- **Problem**: Manual tuning attempts resulted in -8.76 ELO regression
- **Solution**: Use SPSA to find optimal endgame differentials

## Implementation Strategy

### Phase 1: Minimal Endgame Differential Tuning (Recommended Start)

Start by tuning only the endgame values for key squares, keeping middlegame values fixed. This reduces parameter count and focuses on finding optimal mg/eg differentials.

#### SPSA Input Configuration

```
# Pawn Endgame PST - Central files only (D & E columns)
# Current values based on commit 34dbdd2
pawn_eg_r3_d, int, 10, 0, 30, 2, 0.002
pawn_eg_r3_e, int, 10, 0, 30, 2, 0.002
pawn_eg_r4_d, int, 25, 10, 50, 3, 0.002
pawn_eg_r4_e, int, 25, 10, 50, 3, 0.002
pawn_eg_r5_d, int, 40, 20, 70, 4, 0.002
pawn_eg_r5_e, int, 40, 20, 70, 4, 0.002
pawn_eg_r6_d, int, 60, 30, 100, 5, 0.002
pawn_eg_r6_e, int, 60, 30, 100, 5, 0.002
pawn_eg_r7_center, int, 90, 50, 150, 6, 0.002

# Knight Endgame PST - Simplified zones
knight_eg_center, int, 15, 5, 25, 2, 0.002
knight_eg_extended, int, 10, 0, 20, 2, 0.002
knight_eg_edge, int, -25, -40, -10, 2, 0.002
knight_eg_corner, int, -40, -50, -20, 3, 0.002

# Bishop Endgame PST - Diagonal control focus
bishop_eg_long_diag, int, 20, 10, 35, 2, 0.002
bishop_eg_center, int, 15, 5, 25, 2, 0.002
bishop_eg_edge, int, -5, -15, 5, 2, 0.002

# Rook Endgame PST - Activity-based
rook_eg_7th, int, 25, 15, 40, 3, 0.002
rook_eg_active, int, 10, 5, 20, 2, 0.002
rook_eg_passive, int, 5, 0, 15, 2, 0.002

# Queen Endgame PST - Simplified
queen_eg_center, int, 10, 5, 20, 2, 0.002
queen_eg_active, int, 5, 0, 15, 2, 0.002
queen_eg_back, int, -5, -10, 5, 2, 0.002
```

### Phase 2: Full PST Tuning (After Phase 1 Success)

Once the simplified approach proves successful, expand to more granular control:

```
# Pawn PST - All central and near-central files
# Format: p_[mg/eg]_r[rank]_[file]
p_mg_r3_c, int, 5, 0, 15, 2, 0.002
p_eg_r3_c, int, 10, 0, 25, 2, 0.002
p_mg_r3_d, int, 10, 5, 20, 2, 0.002
p_eg_r3_d, int, 15, 5, 30, 2, 0.002
p_mg_r3_e, int, 10, 5, 20, 2, 0.002
p_eg_r3_e, int, 15, 5, 30, 2, 0.002
p_mg_r3_f, int, 5, 0, 15, 2, 0.002
p_eg_r3_f, int, 10, 0, 25, 2, 0.002

# Continue for ranks 4-7...
# Note: Enforce symmetry in implementation to reduce parameters
```

## UCI Implementation Requirements

### 1. Add UCI Options Handler

```cpp
// In src/uci/uci.cpp, add to the option parsing:

void UCI::setOption(const std::string& name, const std::string& value) {
    // Existing options...
    
    // SPSA PST Tuning Options
    if (name.find("pawn_eg_") == 0 || name.find("knight_eg_") == 0 || 
        name.find("bishop_eg_") == 0 || name.find("rook_eg_") == 0 || 
        name.find("queen_eg_") == 0) {
        updatePSTFromUCI(name, std::stoi(value));
        return;
    }
}
```

### 2. Create PST Update System

```cpp
// In src/evaluation/pst.h, add:

class PST {
public:
    // Existing methods...
    
    // SPSA tuning interface
    static void updateEndgameValue(PieceType pt, Square sq, int value) {
        // Update the endgame component of the PST
        s_pstTables[pt][sq].eg = Score(value);
    }
    
    static void updateFromUCIParam(const std::string& param, int value);
    
private:
    // Make tables non-const for tuning
    static std::array<std::array<MgEgScore, 64>, 6> s_pstTables;
};
```

### 3. Parameter Mapping Implementation

```cpp
// In src/evaluation/pst.cpp:

void PST::updateFromUCIParam(const std::string& param, int value) {
    // Parse parameter name
    if (param.find("pawn_eg_") == 0) {
        // Extract rank and file info
        if (param == "pawn_eg_r3_d") {
            updateEndgameValue(PAWN, SQ_D3, value);
            updateEndgameValue(PAWN, SQ_E3, value); // Symmetry
        }
        else if (param == "pawn_eg_r4_d") {
            updateEndgameValue(PAWN, SQ_D4, value);
            updateEndgameValue(PAWN, SQ_E4, value); // Symmetry
        }
        // ... continue for all parameters
    }
    else if (param.find("knight_eg_") == 0) {
        if (param == "knight_eg_center") {
            // Update all center squares
            for (Square sq : {SQ_D4, SQ_E4, SQ_D5, SQ_E5}) {
                updateEndgameValue(KNIGHT, sq, value);
            }
        }
        else if (param == "knight_eg_corner") {
            // Update all corner squares
            for (Square sq : {SQ_A1, SQ_H1, SQ_A8, SQ_H8}) {
                updateEndgameValue(KNIGHT, sq, value);
            }
        }
        // ... continue
    }
    // ... other pieces
}
```

### 4. Symmetry Enforcement

```cpp
// Helper to maintain left-right symmetry
void updateSymmetric(PieceType pt, Square sq, int mgValue, int egValue) {
    File f = fileOf(sq);
    Rank r = rankOf(sq);
    
    // Update the square itself
    s_pstTables[pt][sq] = MgEgScore(mgValue, egValue);
    
    // Update symmetric square
    File symFile = File(7 - f);
    Square symSq = makeSquare(symFile, r);
    s_pstTables[pt][symSq] = MgEgScore(mgValue, egValue);
}
```

## OpenBench Configuration

### Test Settings
- **Test Type**: SPSA
- **Base Branch**: main (or stable baseline)
- **Test Branch**: Your SPSA-enabled branch
- **Book**: UHO_4060_v2.epd
- **Time Control**: 10+0.1 (or 60+0.6 for more stable results)
- **SPSA Hyperparameters**:
  - Alpha: 0.602 (default)
  - Gamma: 0.101 (default)
  - A-Ratio: 0.1 (default)
  - Pairs-Per: 8
  - Iterations: 50000 (initial test), 200000+ (full tune)
- **Reporting**: Batched
- **Distribution**: Single

### C_end Selection Guidelines

The step size (C_end) should be approximately 1/20th of the reasonable range:

| Parameter Type | Range | Recommended C_end |
|---------------|-------|-------------------|
| Small positional (-10 to 10) | 20 | 1 |
| Medium positional (0 to 50) | 50 | 2-3 |
| Large positional (0 to 100) | 100 | 5 |
| Passed pawn bonuses (0 to 150) | 150 | 7-8 |

### R_end Guidelines

- Standard value: 0.002
- Can be increased to 0.003 for faster convergence (risk of instability)
- Can be decreased to 0.001 for more stable but slower convergence

## Testing Phases

### 1. Infrastructure Test (1-2 hours)
- Implement UCI parameter handling
- Run short SPSA test (1000 iterations)
- Verify parameters are being updated correctly
- Check that bench changes with different parameters

### 2. Initial Tuning (24-48 hours)
- Use Phase 1 configuration (simplified parameters)
- Run 50000 iterations
- Monitor for convergence
- Check for reasonable value evolution

### 3. Refinement (48-72 hours)
- If Phase 1 successful, expand parameters
- Run 200000+ iterations
- Consider multiple runs with different seeds

### 4. Validation
- Test best parameters in regular SPRT
- Compare against baseline
- Verify improvements are stable

## Monitoring and Debugging

### Signs of Good Convergence
- Parameters stabilize around certain values
- Small oscillations near end of tuning
- Consistent ELO gain in validation

### Signs of Problems
- Parameters hitting min/max bounds
- Wild oscillations throughout tuning
- Parameters moving in opposite directions repeatedly

### Debug Commands
```cpp
// Add UCI command to dump current PST values
else if (token == "dumpPST") {
    for (int pt = PAWN; pt <= QUEEN; ++pt) {
        for (int sq = 0; sq < 64; ++sq) {
            std::cout << "PST[" << pt << "][" << sq << "] = " 
                     << "MgEgScore(" << s_pstTables[pt][sq].mg.value() 
                     << ", " << s_pstTables[pt][sq].eg.value() << ")\n";
        }
    }
}
```

## Expected Outcomes

### Conservative Expectations
- 5-10 ELO gain from optimized endgame differentials
- Better endgame play without regression
- Foundation for future PST improvements

### Optimistic Potential
- 15-20 ELO gain if current values are far from optimal
- Discovered patterns can inform future manual tuning
- May reveal unexpected piece relationships

## Common Pitfalls to Avoid

1. **Too Many Parameters**: Start small, expand gradually
2. **Poor C_end Selection**: Too large causes instability, too small prevents movement
3. **Insufficient Iterations**: PST tuning needs 100k+ games minimum
4. **Ignoring Symmetry**: Left-right symmetry should usually be enforced
5. **Not Validating**: Always SPRT test the final values

## Next Steps After Successful Tuning

1. **Hard-code Best Values**: Replace PST tables with tuned values
2. **Remove UCI Options**: Clean up tuning infrastructure
3. **Document Findings**: Record what patterns emerged
4. **Consider Phase Tuning**: Tune the phase calculation weights
5. **Expand to Other Terms**: Apply similar approach to mobility, king safety, etc.

## References

- OpenBench SPSA Wiki: [Link to documentation]
- Original SPSA Papers: Spall (1998), "Implementation of the SPSA Algorithm"
- Stockfish Tuning Sessions: fishtest.stockfishchess.org (for methodology examples)
- SeaJay PST Implementation: src/evaluation/pst.h

## Version History

- v1.0 (2025-08-31): Initial SPSA tuning guide for PST endgame values
- Based on SeaJay commit 34dbdd2 (stable baseline with +46.50 ELO)