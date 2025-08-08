# SeaJay Chess Engine

A modern chess engine developed using C++20, featuring NNUE evaluation and advanced search techniques.

**Author:** Brandon Harris

## Project Status

Currently in Phase 1: Foundation and Move Generation

For detailed development planning, see:
- [Master Project Plan](project_docs/SeaJay%20Chess%20Engine%20Development%20-%20Master%20Project%20Plan.md) - Comprehensive development roadmap
- [Project Status](project_docs/project_status.md) - Current progress and implementation notes

## Development Philosophy

- **Single Feature Focus**: Each stage implements one feature with complete validation
- **Statistical Validation**: All improvements validated through SPRT testing
- **Performance First**: Optimized C++20 implementation with careful attention to speed
- **Hybrid Architecture**: Combines bitboard and mailbox representations

## Building

### Requirements
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- Git

### Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/seajay-chess.git
cd seajay-chess

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j

# Run the engine (once Phase 1 Stage 3 is complete)
./bin/seajay
```

## Project Structure

```
seajay-chess/
├── .devcontainer/        # Dev container configuration
│   ├── Dockerfile        # Container image definition
│   ├── devcontainer.json # VSCode container settings
│   └── post-create.sh    # Setup script
├── src/                  # Source code
│   ├── core/            # Core chess logic (board, moves, position)
│   ├── search/          # Search algorithms (negamax, alpha-beta)
│   ├── nnue/            # Neural network evaluation
│   │   └── simd/        # SIMD optimizations
│   └── uci/             # UCI protocol interface
├── tests/               # Test suites
│   ├── perft/          # Move generation tests
│   ├── unit/           # Unit tests
│   └── positions/      # Test positions (EPD/FEN)
├── tools/              # Development tools
│   ├── scripts/        # Build and test scripts
│   ├── configs/        # Tool configurations
│   ├── tuner/         # Parameter tuning
│   └── datagen/       # Training data generation
├── networks/          # NNUE network files
├── docs/             # Documentation
│   ├── development/  # Development notes and journal
│   ├── api/         # API documentation
│   └── benchmarks/  # Performance tracking
├── bench/           # Benchmarking positions
└── project_docs/    # Project planning documents
    ├── SeaJay Chess Engine Development - Master Project Plan.md
    └── project_status.md
```

## Development Phases

1. **Phase 1**: Foundation and Move Generation
2. **Phase 2**: Basic Search and Evaluation
3. **Phase 3**: Essential Optimizations
4. **Phase 4**: NNUE Integration
5. **Phase 5**: Advanced Search Techniques
6. **Phase 6**: Refinements and Optimization
7. **Phase 7**: Custom NNUE Training

See `project_docs/` for detailed development plan.

## Testing

Testing infrastructure will be set up in Phase 1, Stage 5, including:
- Perft validation
- SPRT testing with fast-chess
- Unit tests
- Tactical test suites

## AI-Assisted Development

SeaJay is being developed through a unique collaboration between human expertise and AI assistance, leveraging the best of both worlds to create a high-quality chess engine.

### Development Partners

**Primary AI Assistant: Claude (Anthropic)**
- Architecture design and planning
- Code generation and implementation
- Bug detection and fixing
- Performance optimization suggestions
- Testing strategy development

**Specialized AI Agents:**
- **chess-engine-expert**: Deep expertise in chess engine algorithms, move generation, search techniques, evaluation functions, and NNUE implementation. Assists with debugging perft results, optimizing search functions, and implementing advanced techniques.
- **dev-diary-chronicler**: Documents the development journey in narrative form, capturing both technical progress and the human experience of building a chess engine from scratch.

### Development Methodology

1. **Human-Guided Direction**: Brandon Harris provides project vision, makes architectural decisions, and validates all implementations
2. **AI-Powered Implementation**: Claude assists with code generation, ensuring modern C++ best practices and chess engine conventions
3. **Collaborative Problem-Solving**: Complex bugs and optimizations are solved through iterative human-AI dialogue
4. **Transparent Attribution**: All AI contributions are acknowledged in commit messages and documentation

### Benefits of This Approach

- **Accelerated Development**: AI assistance speeds up boilerplate code and common pattern implementation
- **Knowledge Transfer**: AI brings expertise from analyzing thousands of chess engines
- **Code Quality**: Consistent style and modern C++ practices throughout
- **Documentation**: Comprehensive documentation generated alongside code
- **Learning Journey**: The development process itself becomes educational content

### Ethical Considerations

- All AI-generated code is reviewed and understood before integration
- The project maintains transparency about AI involvement
- The goal is to demonstrate effective human-AI collaboration in software development
- Performance claims will be independently verified through SPRT testing

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines. We welcome contributions and explicitly encourage documenting any AI assistance used.

## License

SeaJay Chess Engine is licensed under the **GNU General Public License v3.0** (GPL-3.0).

### What This Means

- ✅ **Free to use** for personal, educational, and research purposes
- ✅ **Open source** - you can view, modify, and distribute the code
- ✅ **Copyleft** - any derivatives must also be GPL-3.0
- 📧 **Commercial licensing** available - contact Brandon Harris for commercial use

### Dual Licensing Model

SeaJay uses a dual-licensing model:
1. **GPL-3.0** for open-source community use
2. **Commercial License** for proprietary applications (contact for terms)

This model ensures the engine remains free for the chess community while allowing sustainable development through commercial licensing.

### Contributing

Contributors must agree to the [Contributor License Agreement (CLA)](CLA.md) which allows the project to maintain dual-licensing flexibility. See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## Acknowledgments

### Development Team
- **Lead Developer**: Brandon Harris
- **AI Development Partner**: Claude (Anthropic)
- **Specialized Agents**: chess-engine-expert, dev-diary-chronicler

### Technical References
- Chess Programming Wiki for algorithmic references
- Stockfish team for NNUE pioneering
- Fast-chess and Cute Chess for testing infrastructure
- The broader chess programming community for shared knowledge