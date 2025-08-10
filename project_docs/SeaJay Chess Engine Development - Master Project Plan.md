# SeaJay Chess Engine Development - Master Project Plan

## Project Overview

This comprehensive project plan outlines a methodical, modular approach to building a competitive chess engine incorporating modern NNUE evaluation. The plan emphasizes incremental development with continuous statistical validation, ensuring each component contributes measurably to engine strength before proceeding to the next phase.

## Development Philosophy

**Single Feature Focus**: Each development stage implements exactly one feature with complete testing and validation before proceeding to the next stage.

**Statistical Validation**: From Phase 2 onwards, all improvements must pass Sequential Probability Ratio Test (SPRT) validation to ensure genuine strength gains.

**Early Playability**: The engine will be playable through standard chess GUIs from Phase 1, Stage 3, enabling continuous testing and motivation throughout development.

**Hybrid Architecture**: Combines bitboard and mailbox representations from inception to optimize both move generation speed and position evaluation efficiency.

**Performance First**: Given that chess engine success is often a function of speed, we prioritize performant solutions, implementing in C++ and utilizing appropriate optimization techniques throughout development.

**Historical Journaling**: We track our development efforts, doing our best to create a journal or diary so that others who wish to follow along, or better understand our efforts and review what decisions we made and what steps we took during this development effort. 

## Phase 1: Foundation and Move Generation

### Objective

Establish a complete, bug-free board representation and move generation system that correctly implements all chess rules, integrated with minimal UCI protocol for GUI compatibility.

### Core Components

**Stage 1 - Board Representation**

- Hybrid bitboard-mailbox data structure implementation
- 64-square mailbox array for O(1) piece lookup
- 12 bitboards for piece positions (6 piece types × 2 colors)
- Occupied and empty square bitboards
- Initial Zobrist key infrastructure for future hashing
- Basic ray-based sliding piece move generation (temporary implementation)
  - Simple loop-based rook/bishop/queen move generation
  - Will be replaced with magic bitboards in Phase 3
- **Milestone**: Unit tests pass for all board operations

**Stage 2 - Position Management**

- Forsyth-Edwards Notation (FEN) parser implementation
- Board display and debugging functions
- Position loading and validation
- Game state tracking (castling rights, en passant square, fifty-move rule, move counters)
- **Milestone**: Round-trip testing (board to FEN to board) passes for standard positions

**Stage 3 - Basic UCI and Legal Moves**

- Minimal UCI protocol implementation:
  - `uci`: Engine identification and capability advertisement
  - `isready`: Synchronization confirmation
  - `position`: Board setup from FEN or moves
  - `go`: Search initiation with time controls
- Pseudo-legal move generation for all piece types
- Legal move filtering (king safety validation)
- Move format: 16-bit representation (6 bits from, 6 bits to, 4 bits promotion)
- Random move selection for gameplay
- **Milestone**: Successfully plays complete games in Arena or Cute Chess GUI

**Stage 4 - Special Moves and Validation**

- Castling rights tracking and execution (including Chess960 consideration)
- En passant capture implementation with legality validation
- Pawn promotion handling (Queen, Rook, Bishop, Knight)
- Make/unmake move with complete state restoration
- Implement perft (performance test) function with divide capability
- **Milestone**: Pass all perft validation tests (see Testing Requirements)

**Stage 5 - Testing Infrastructure**

- Install fast-chess tournament manager
- Implement `bench` command for performance testing (initially node counting)
- Create automated testing scripts for continuous validation
- Configure SPRT testing parameters
- Set up opening book (8moves_v3.pgn or similar balanced book)
- **Milestone**: Complete successful SPRT test run (engine vs itself)

### Perft Testing Requirements

All positions must pass perft tests to specified depths before proceeding:

**Standard Test Positions:**

1. **Starting Position**: `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`
   - Depth 5: 4,865,609 nodes
   - Depth 6: 119,060,324 nodes

2. **Kiwipete**: `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -`
   - Depth 4: 4,085,603 nodes
   - Depth 5: 193,690,690 nodes

3. **Position 3**: `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -`
   - Depth 5: 674,624 nodes
   - Depth 6: 11,030,083 nodes

4. **Position 4**: `r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1`
   - Depth 4: 422,333 nodes
   - Depth 5: 15,833,292 nodes

5. **Position 5**: `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8`
   - Depth 4: 2,103,487 nodes
   - Depth 5: 89,941,194 nodes

6. **Position 6**: `r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10`
   - Depth 4: 3,894,594 nodes
   - Depth 5: 164,075,551 nodes

**Edge Case Positions:**

- **Illegal EP Move 1**: `3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1`
- **Illegal EP Move 2**: `8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1`
- **EP Capture Checks**: `8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1`
- **Short Castling Check**: `5k2/8/8/8/8/8/8/4K2R w K - 0 1`
- **Long Castling Check**: `3k4/8/8/8/8/8/8/R3K3 w Q - 0 1`
- **Castle Rights**: `r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1`

### Phase Completion Criteria

- All perft tests pass to depth 6+ with 100% accuracy
- GUI compatibility confirmed with multiple interfaces
- SPRT testing infrastructure operational
- Zero compiler warnings at maximum warning level
- Code committed to version control with comprehensive documentation

## Phase 2: Basic Search and Evaluation

### Objective

Implement fundamental search algorithms and evaluation functions to transition from random play to intelligent move selection, with each improvement statistically validated.

### Core Components

**Stage 6 - Material Evaluation**

- Piece value implementation using centipawn scale (100 cp = 1 pawn):
  - Pawn = 100
  - Knight = 320
  - Bishop = 330
  - Rook = 500
  - Queen = 900
- Single-ply move evaluation
- Best capture selection based on material gain
- **SPRT Validation**: Material evaluation vs random mover
- **Expected Outcome**: 100% win rate against random play
- **Milestone**: Engine consistently captures hanging pieces

**Stage 7 - Negamax Search**

- Negamax framework implementation (cleaner than traditional minimax)
- Multi-ply recursive search
- Fixed depth search (4 ply initially)
- Basic time management (5% of remaining time per move)
- Search statistics tracking (nodes, time, branching factor)
- Alpha-beta parameters added but not activated (preparation for Stage 8)
- No move ordering (natural generation order)
- Static evaluation at leaf nodes (no quiescence)
- **SPRT Validation**: 1-ply vs 4-ply search
- **Expected Outcome**: +200-300 Elo improvement
- **Milestone**: Finds mate-in-1 and mate-in-2 positions reliably

**Stage 8 - Alpha-Beta Pruning**

- Activate alpha-beta cutoffs (framework from Stage 7)
- Basic move ordering (captures first using isCapture())
- Fail-soft vs fail-hard consideration
- Search tree reduction without affecting move selection
- Beta cutoff statistics tracking
- **SPRT Validation**: Plain negamax vs alpha-beta pruning
- **Expected Outcome**: 5x search speed improvement at same strength
- **Milestone**: Reaches depth 6 in under 1 second from starting position

**Stage 9 - Positional Evaluation**

- Piece-square table implementation (values from Chess Programming Wiki)
- Combined material and positional scoring
- Symmetric evaluation (same result regardless of side to move)
- Tapered evaluation preparation for endgame transitions
- **SPRT Validation**: Material-only vs material+positional
- **Expected Outcome**: +150-200 Elo improvement
- **Milestone**: Demonstrates piece development and center control

### Phase Completion Criteria

- Engine strength approximately 1500 Elo
- All SPRT tests show positive results
- Search reaches 6+ ply in middlegame positions
- Basic tactical test suite (subset of WAC) performance validated

## Phase 3: Essential Optimizations

### Objective

Implement critical performance optimizations that form the foundation of all competitive chess engines, achieving approximately 2100 Elo strength.

### Core Components

**Stage 10 - Magic Bitboards for Sliding Pieces**

- Pre-calculated attack table generation for rooks and bishops
  - Magic number generation or use of proven magic constants
  - Attack table initialization at startup
  - Memory layout optimization for cache efficiency
- Replace ray-based move generation with magic lookups
  - Rook attack generation via magic bitboards
  - Bishop attack generation via magic bitboards
  - Queen attacks as combination of rook and bishop
- Comprehensive validation suite:
  - Perft tests must maintain 100% accuracy
  - Attack generation validation for all 64 squares
  - Blocker permutation testing (all possible occupancy patterns)
  - Edge case validation (pieces on board edges/corners)
  - Performance benchmarking vs ray-based approach
- **Validation Requirements**:
  - Generate and verify all 4096 blocker patterns for each rook square
  - Generate and verify all 512 blocker patterns for each bishop square
  - Cross-reference attacks with known correct implementations
  - Memory usage under 2MB for all magic tables combined
- **SPRT Validation**: Ray-based vs Magic bitboards
- **Expected Outcome**: 3-5x speedup in move generation
- **Milestone**: >1M positions/second in perft tests

**Stage 11 - Move Ordering**

- Most Valuable Victim - Least Valuable Attacker (MVV-LVA) implementation
- Capture move prioritization for optimal alpha-beta performance
- Static Exchange Evaluation (SEE) preparation
- **SPRT Validation**: Random ordering vs MVV-LVA
- **Expected Outcome**: +50-100 Elo improvement
- **Milestone**: 30% reduction in nodes searched

**Stage 12 - Transposition Tables**

- Zobrist hashing implementation (64-bit keys)
- Hash table with replacement strategy:
  - Initial size: 64MB (configurable)
  - Always-replace or two-tier scheme
  - Depth-preferred replacement
- Store entry components:
  - Zobrist key (for collision detection)
  - Best move
  - Search depth
  - Score (exact, lower bound, or upper bound)
  - Bound type (EXACT, ALPHA, BETA)
  - Age counter for long searches
- **SPRT Validation**: Without TT vs with TT
- **Expected Outcome**: +100-150 Elo improvement
- **Milestone**: 50% reduction in search nodes for complex positions

**Stage 13 - Iterative Deepening**

- Progressive depth search (1, 2, 3... until time limit)
- Previous iteration best move extraction
- Aspiration window preparation
- Time allocation algorithm:
  - Sudden death time controls
  - Increment time controls
  - Move time limits
- **SPRT Validation**: Fixed depth vs iterative deepening
- **Expected Outcome**: +50-100 Elo improvement
- **Milestone**: Zero time losses in 1000 game test

**Stage 14 - Quiescence Search**

- Capture sequence extension to avoid horizon effect
- Stand-pat evaluation (static eval as lower bound)
- Delta pruning for obviously losing captures
- Maximum quiescence depth limiting (typically 8-16 ply)
- Include checks and promotions in quiescence
- **SPRT Validation**: Without vs with quiescence search
- **Expected Outcome**: +100-150 Elo improvement
- **Milestone**: No tactical oversights in quiet positions

### Phase Completion Criteria

- Engine strength approximately 2000-2100 Elo
- Magic bitboard implementation fully validated:
  - All perft tests maintain 100% accuracy
  - Attack generation validated for all squares and occupancy patterns
  - 3-5x performance improvement in move generation confirmed
  - Memory usage for magic tables under 2MB
- Consistent tactical accuracy in test positions
- Efficient time management across all time controls
- Memory usage within design parameters (under 128MB default)
- Performance benchmarks: >1M NPS on modern hardware (improved from >500K due to magic bitboards)

## Phase 4: NNUE Integration

### Objective

Integrate Efficiently Updatable Neural Network evaluation using pre-trained networks, achieving a massive strength increase to approximately 2800 Elo.

### Core Components

**Stage 15 - Network Infrastructure**

- NNUE file format parser (.nnue binary format)
- Weight matrix loading and storage:
  - Input layer weights (HalfKAv2_hm architecture)
  - Hidden layer weights
  - Output layer weights
- Memory allocation for network structures
- Support for multiple network architectures
- **Milestone**: Correctly parse and load Stockfish NNUE file (~20-40MB)

**Stage 16 - Accumulator Implementation**

- HalfKAv2_hm feature extraction:
  - 41024 input features (641 * 64)
  - King-piece relationships
  - Piece-square encoding
- Incremental accumulator updates:
  - Efficient add/remove piece operations
  - Perspective-based evaluation (white/black)
- Make/unmake move integration with minimal overhead
- **Milestone**: Accumulator values match reference implementation

**Stage 17 - Forward Pass**

- Matrix multiplication routines (initially without SIMD)
- ClippedReLU activation function (max(0, min(127, x)))
- Complete inference pipeline:
  - Input layer: 41024 → 512
  - Hidden layers: 512 → 32 → 32
  - Output layer: 32 → 1
- Scaling to centipawn output
- **SPRT Validation**: Classical evaluation vs NNUE
- **Expected Outcome**: +600-800 Elo improvement
- **Milestone**: Evaluation matches reference implementation outputs

**Stage 18 - Performance Optimization**

- SIMD instruction implementation:
  - AVX2 for modern processors
  - SSE for compatibility
  - Fallback scalar implementation
- Accumulator update optimization:
  - Lazy evaluation
  - Differential updates only
- Cache-friendly data structures and access patterns
- **SPRT Validation**: Baseline NNUE vs optimized NNUE
- **Expected Outcome**: +30-50 Elo from speed improvements
- **Milestone**: 2x evaluation speed improvement, >1M NPS

### Phase Completion Criteria

- Engine strength approximately 2800 Elo
- NNUE evaluation fully integrated and optimized
- Support for multiple network files
- Performance metrics: >1M NPS with NNUE
- Maintains compatibility with various network architectures

## Phase 5: Advanced Search Techniques

### Objective

Implement sophisticated search optimizations that distinguish strong engines, targeting 3000+ Elo strength.

### Core Components

**Stage 19 - Principal Variation Search**

- PV node identification and tracking
- Zero-window searches for non-PV nodes
- PV table for move extraction
- Fail-high/fail-low re-searches
- **SPRT Parameters**: elo0=0, elo1=5, alpha=0.05, beta=0.05
- **Expected Outcome**: +20-40 Elo improvement
- **Milestone**: 20% reduction in search nodes

**Stage 20 - Null Move Pruning**

- Null move implementation (passing turn to opponent)
- Adaptive reduction (R=2 or R=3 based on depth)
- Zugzwang detection and handling:
  - Disable in endgames with few pieces
  - Verification search in critical positions
- **SPRT Parameters**: elo0=0, elo1=5
- **Expected Outcome**: +30-50 Elo improvement
- **Milestone**: Improved performance in winning positions

**Stage 21 - Late Move Reductions**

- Depth reduction for moves late in move ordering:
  - Reduction formula based on depth and move number
  - Full depth re-search if reduced search beats alpha
- LMR conditions:
  - Not in check
  - Not giving check
  - Not capturing
  - Sufficient depth remaining
  - Move number > 3-4
- **SPRT Parameters**: elo0=0, elo1=10 (major feature)
- **Expected Outcome**: +50-100 Elo improvement
- **Milestone**: 2x effective search depth increase

**Stage 22 - Killer Move Heuristic**

- Killer move table (2-3 moves per ply)
- Integration with move ordering after hash move
- Killer move verification before use
- **SPRT Parameters**: elo0=0, elo1=5
- **Expected Outcome**: +15-30 Elo improvement
- **Milestone**: Improved cut-off rates in similar positions

**Stage 23 - History Heuristic**

- Butterfly boards for move success tracking
- History scoring and aging mechanism
- Counter-move history consideration
- Integration with LMR and move ordering
- **SPRT Parameters**: elo0=0, elo1=5
- **Expected Outcome**: +20-40 Elo improvement
- **Milestone**: Further move ordering improvement metrics

### Phase Completion Criteria

- Engine strength approximately 3000 Elo
- Search efficiency metrics improved across all categories
- Stable performance at bullet time controls (1+0)
- Advanced search features fully integrated

## Phase 6: Refinements and Optimization

### Objective

Implement advanced pruning techniques and multi-threading support to reach competitive strength of 3100+ Elo.

### Core Components

**Stage 24 - Aspiration Windows**

- Initial window around previous iteration score
- Dynamic window adjustment on fail-high/fail-low
- Multiple re-search strategy
- **SPRT Parameters**: elo0=0, elo1=5
- **Expected Outcome**: +10-20 Elo improvement
- **Milestone**: Reduced search instability

**Stage 25 - Futility Pruning**

- Static evaluation-based pruning near leaf nodes
- Futility margins based on depth
- Extended futility for multiple ply
- Move count-based pruning
- **SPRT Parameters**: elo0=0, elo1=5
- **Expected Outcome**: +15-30 Elo improvement
- **Milestone**: Faster searches in clearly decided positions

**Stage 26 - Singular Extensions**

- Singular move detection through reduced searches
- Extension criteria and depth
- Multi-cut pruning consideration
- **SPRT Parameters**: elo0=0, elo1=5
- **Expected Outcome**: +10-25 Elo improvement
- **Milestone**: Better tactical accuracy in critical positions

**Stage 27 - Multi-threading**

- Lazy SMP (Shared Memory Parallelization) implementation
- Thread-safe transposition table:
  - Lock-less hashing (XOR trick)
  - Atomic operations where necessary
- Thread pool management
- NUMA awareness for large systems
- **SPRT Parameters**: 1 thread vs 4 threads
- **Expected Outcome**: +50-80 Elo on quad-core systems
- **Milestone**: 24-hour stability test passes without crashes

### Phase Completion Criteria

- Engine strength approximately 3100 Elo
- Multi-threaded scaling efficiency above 70%
- All advanced features integrated and validated
- Stable performance in long time control games

## Phase 7: Custom NNUE Training

### Objective

Develop capability to train custom neural networks optimized for the engine's specific search characteristics.

### Core Components

**Stage 28 - Data Generation**

- Self-play game generation at fixed depth (8-12 ply)
- Position extraction with game outcome labeling
- FEN output with evaluation scores
- Data filtering and deduplication
- Target: 1-10 million positions
- **Milestone**: Valid training dataset created and verified

**Stage 29 - Training Infrastructure**

- Bullet trainer installation (Rust-based NNUE trainer)
- Alternative: nnue-pytorch for research flexibility
- Network architecture configuration:
  - Input features (HalfKAv2_hm or custom)
  - Hidden layer sizes
  - Activation functions
- Training hyperparameters:
  - Learning rate schedule
  - Batch size
  - Epoch count
  - Regularization parameters
- **Milestone**: Training pipeline executes successfully

**Stage 30 - Custom Network Development**

- Initial network training on generated data
- Validation set evaluation
- Quantization to int8/int16 for inference
- Network size optimization
- **SPRT Validation**: Reference network vs custom network
- **Expected Outcome**: Comparable or improved strength
- **Milestone**: Custom network integrated and validated

**Stage 31 - Automated Pipeline**

- End-to-end automation scripts:
  - Data generation scheduling
  - Training job management
  - Network validation and testing
  - Deployment automation
- Continuous improvement process
- Performance monitoring and reporting
- A/B testing framework for networks
- **Milestone**: Complete training cycle without manual intervention

### Phase Completion Criteria

- Custom NNUE training capability established
- Engine strength 3200+ Elo with custom network
- Documented training procedures
- Reproducible training results
- Automated improvement pipeline operational

## Testing and Validation Framework

### SPRT Configuration Standards

**Standard Improvement Tests**

- Parameters: elo0=0, elo1=5, alpha=0.05, beta=0.05
- Time Control: 8+0.08 seconds
- Opening Book: 8moves_v3.pgn or equivalent
- Expected games: 5,000-30,000 for resolution

**Major Feature Tests**

- Parameters: elo0=0, elo1=10, alpha=0.05, beta=0.05
- Used for: LMR, NNUE integration, major search changes
- Extended testing for high-impact features

**Non-Regression Tests**

- Parameters: elo0=-3, elo1=0, alpha=0.05, beta=0.05
- Ensures refactoring maintains strength
- Used for code cleanup and optimization

### Sample SPRT Command

```bash
fast-chess -engine cmd=./seajay_new name=new \
           -engine cmd=./seajay_old name=old \
           -each tc=8+0.08 -rounds 30000 -repeat \
           -concurrency 4 -recover -randomseed \
           -openings file=8moves_v3.pgn format=pgn \
           -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05
```

### Test Suite Resources

**Tactical Test Suites**

- WAC (Win at Chess): 300 tactical positions
- ECM (Encyclopedia of Chess Middlegames): Positional tests
- STS (Strategic Test Suite): 1500 themed positions
- Arasan test suite: Mixed tactical/positional positions

**Performance Benchmarks**

- Time-to-depth measurements
- Nodes per second tracking
- Time-to-solution for test positions
- Scaling efficiency metrics

### Quality Assurance Gates

Each stage must satisfy the following criteria before proceeding:

1. Feature implementation complete and functional
2. Zero compiler warnings at maximum warning level
3. Perft validation passes (maintained throughout development)
4. SPRT test passes with required confidence (Stages 6+)
5. Code review completed and documented
6. Version control commit with descriptive message
7. Performance benchmarks recorded

### Version Control Strategy

- Feature branches for each stage development
- Stable branch maintained with validated features only
- Tagged releases after each successful phase
- Comprehensive commit documentation including:
  - Feature description
  - SPRT test results
  - Elo gain measurements
  - Performance impact

## Risk Mitigation Strategies

### Common Failure Patterns and Mitigations

**Multiple Concurrent Features**

- Risk: Feature interaction bugs, debugging complexity
- Mitigation: Strict adherence to single-feature stages
- Detection: Code review before integration

**Insufficient Testing**

- Risk: Strength regression, hidden bugs
- Mitigation: Mandatory SPRT validation
- Detection: Automated test suite execution

**Performance Regression**

- Risk: Slower searches, reduced playing strength
- Mitigation: Continuous benchmarking
- Detection: Automated performance monitoring

**Move Generation Bugs**

- Risk: Illegal moves, game crashes
- Mitigation: Continuous perft validation
- Detection: Perft test suite in CI/CD pipeline

**Time Management Failures**

- Risk: Time losses, poor time allocation
- Mitigation: Extensive time control testing
- Detection: Tournament testing under various time controls

## Expected Strength Progression

- Phase 1 Completion: Playing random legal moves (0 Elo)
- Stage 6: ~800 Elo (material evaluation)
- Stage 7: ~1200 Elo (multi-ply search)
- Stage 9: ~1500 Elo (positional awareness)
- Stage 10: ~1500 Elo (magic bitboards - no Elo gain, but 3-5x speedup)
- Stage 12: ~1800 Elo (transposition tables)
- Stage 14: ~2100 Elo (quiescence search)
- Stage 17: ~2800 Elo (NNUE integration)
- Stage 21: ~2950 Elo (late move reductions)
- Stage 23: ~3000 Elo (refined search)
- Stage 27: ~3100 Elo (multi-threading and advanced features)
- Stage 31: ~3200+ Elo (custom NNUE networks)

## Resource Requirements

### Development Environment

- C++ compiler with C++17 support (GCC 9+, Clang 10+, MSVC 2019+)
- Git version control system
- 16GB+ RAM for development and testing
- Multi-core CPU for parallel testing (4+ cores recommended)
- CMake or Makefile build system

### Testing Infrastructure

- fast-chess tournament manager (preferred) or cutechess-cli
- Reference engines for strength comparison:
  - Stockfish (various versions for different strength levels)
  - Ethereal, Laser, or similar for mid-range testing
- Opening book databases:
  - 8moves_v3.pgn (balanced for weak engines)
  - Perfect2021.pgn or similar for stronger engines
- Endgame tablebases (optional):
  - Syzygy 3-4-5 piece tables
  - Gaviota tablebases as alternative

### NNUE Training Resources

- Bullet training framework (recommended) or nnue-pytorch
- GPU recommended for network training (optional but faster)
- 100GB+ storage for training data
- Training data sources:
  - Self-play games
  - CCRL game database
  - Lichess elite database
  - Leela Chess Zero training data (ODbL license)

### Development Tools

- Profiler (gprof, perf, VTune)
- Debugger (GDB, LLDB, Visual Studio)
- Static analysis tools (clang-tidy, cppcheck)
- Chess GUI for testing (Arena, Cute Chess, ChessBase)

## Success Metrics

- **Correctness**: Perft accuracy 100% at depth 6+ for all test positions
- **Reliability**: Zero crashes in 10,000 game tests
- **Performance**: >1M NPS on modern hardware (Intel i7/AMD Ryzen)
- **Strength**: 3200+ Elo achievable with custom NNUE
- **Efficiency**: <100MB memory usage in typical games
- **Scalability**: >70% efficiency with 4 threads
- **Maintainability**: <10,000 lines of core engine code
- **Testing**: >95% SPRT pass rate for planned features

## References and Resources

### Primary Documentation

- **Chess Programming Wiki**: https://www.chessprogramming.org/
  - Comprehensive reference for all chess programming topics
  - Algorithm descriptions and implementation details
- **UCI Protocol Specification**: https://www.chessprogramming.org/UCI
- **NNUE Documentation**: https://www.chessprogramming.org/NNUE

### Testing Resources

- **Perft Results**: https://www.chessprogramming.org/Perft_Results
- **Test Positions**: https://www.chessprogramming.org/Test-Positions
- **SPRT Mathematics**: https://www.chessprogramming.org/Sequential_Probability_Ratio_Test
- **Rating Lists**: CCRL, CEGT for strength comparisons

### Development Tools

- **fast-chess**: Modern tournament manager with SPRT support
- **OpenBench**: https://github.com/AndyGrant/OpenBench
  - Distributed testing framework
- **Bullet**: https://github.com/jw1912/bullet
  - Efficient NNUE trainer
- **Stockfish**: https://github.com/official-stockfish/Stockfish
  - Reference implementation for many techniques

### Community Resources

- **TalkChess Forums**: http://talkchess.com/
  - Active community of chess programmers
- **Discord Servers**: Chess Programming, Stockfish, Leela Chess
- **GitHub Examples**:
  - Stockfish (C++): State-of-the-art techniques
  - Ethereal (C): Clean implementation
  - Rustic (Rust): Well-documented development journey
  - MadChess (C#): Detailed development blog

### Academic Papers

- **Alpha-Beta Pruning**: Knuth and Moore (1975)
- **Transposition Tables**: Zobrist (1970)
- **Late Move Reductions**: Fruit/Glaurung papers
- **NNUE Architecture**: Yu Nasu (2018)

### Training Data Sources

- **CCRL Games**: http://computerchess.org.uk/ccrl/
- **Lichess Elite Database**: https://database.lichess.org/
- **Leela Training Data**: Available under ODbL license
- **Self-Play Generation**: Recommended for engine-specific optimization

## Conclusion

This project plan provides a systematic approach to developing a competitive chess engine with modern NNUE evaluation. The emphasis on incremental development with continuous validation ensures steady progress while minimizing technical debt and debugging complexity. By following this methodology, the project will deliver a chess engine of master-level strength with well-understood, maintainable code architecture.

The plan's structure allows for flexible pacing while maintaining strict quality gates, ensuring that each component is thoroughly tested and validated before proceeding. With the comprehensive testing framework and detailed specifications provided, developers can confidently build a chess engine that achieves 3200+ Elo strength while maintaining code quality and performance standards.