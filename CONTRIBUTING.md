# Contributing to SeaJay Chess Engine

Thank you for your interest in contributing to SeaJay! This document provides guidelines and standards for contributing to the project.

## Acknowledgments

This project is being developed with significant assistance from:
- **Claude AI** (Anthropic) - Primary development partner providing code generation, architecture decisions, and implementation guidance
- **Claude Code Agents** - Specialized AI agents for chess engine expertise and development documentation

## Development Philosophy

### Single Feature Focus
Each contribution should implement exactly ONE feature or fix ONE bug. This allows for:
- Easier code review
- Cleaner git history  
- Simpler debugging
- Statistical validation (SPRT testing)

### Test-Driven Development
All new features must include:
1. Unit tests where applicable
2. Perft validation for move generation changes
3. SPRT testing for strength improvements (Phase 2+)

## Code Standards

### C++ Guidelines
- **Standard**: C++20
- **Style**: Follows `.clang-format` configuration
- **Naming Conventions**:
  - Classes: `PascalCase` (e.g., `BoardState`)
  - Functions: `camelCase` (e.g., `generateMoves`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_MOVES`)
  - Member variables: `m_camelCase` (e.g., `m_boardState`)

### Performance Considerations
- Prefer `constexpr` for compile-time computations
- Use `[[likely]]/[[unlikely]]` attributes for hot paths
- Implement SIMD optimizations where beneficial
- Profile before optimizing

## Development Process

### 1. Before Starting
- Review the [Master Project Plan](project_docs/SeaJay%20Chess%20Engine%20Development%20-%20Master%20Project%20Plan.md)
- Check [Project Status](project_docs/project_status.md) for current phase/stage
- Ensure your work aligns with the current development stage

### 2. Development Workflow
```bash
# 1. Create a feature branch
git checkout -b feature/stage-X-description

# 2. Make changes following the single feature focus

# 3. Run tests
./build.sh Debug
./bin/seajay perft  # If applicable

# 4. Format code
clang-format -i src/**/*.cpp src/**/*.h

# 5. Commit with meaningful message
git commit -m "Stage X: Implement [specific feature]

- Detailed description of changes
- Performance impact if any
- Test results (perft, SPRT)

Co-authored-by: Claude AI <claude@anthropic.com>"
```

### 3. Testing Requirements

#### Phase 1 (Move Generation)
- Must pass all perft tests to depth 6
- Zero compiler warnings
- No memory leaks (valgrind clean)

#### Phase 2+ (Search/Eval)
- Must pass SPRT test with appropriate parameters
- Include benchmark comparisons
- Document Elo gain

### 4. Documentation
- Update inline comments for complex algorithms
- Add entries to development journal (`docs/development/journal.md`)
- Update `project_status.md` after significant changes

## AI Collaboration Guidelines

When working with Claude AI or other AI assistants:

### Attribution
Always acknowledge AI assistance in commits:
```
Co-authored-by: Claude AI <claude@anthropic.com>
```

### Code Review
- AI-generated code should be reviewed and understood before committing
- Test all AI suggestions thoroughly
- Document any AI-recommended architectural decisions

### Development Journal
Document AI collaboration in the development journal, including:
- Prompts that led to significant breakthroughs
- AI-suggested optimizations that worked/didn't work
- Learning experiences from AI interactions

## Performance Benchmarks

Before submitting performance-related changes:

1. **Run benchmarks**:
   ```bash
   ./bin/seajay bench
   ```

2. **Document results**:
   - Nodes per second (NPS)
   - Time to depth
   - Memory usage

3. **SPRT Testing** (Phase 2+):
   ```bash
   ./tools/scripts/run-sprt.sh new old
   ```

## Pull Request Guidelines

### PR Title Format
```
Phase X, Stage Y: Brief description
```

### PR Description Template
```markdown
## Summary
Brief description of changes

## Implementation Details
- Key changes made
- Algorithms implemented
- Design decisions

## Testing
- [ ] Perft tests pass
- [ ] Unit tests pass
- [ ] SPRT results (if applicable): Elo gain +X
- [ ] No compiler warnings
- [ ] Valgrind clean

## Performance Impact
- NPS before: X
- NPS after: Y
- Memory usage: Z MB

## AI Collaboration
Description of AI assistance received

## Checklist
- [ ] Code follows style guidelines
- [ ] Tests added/updated
- [ ] Documentation updated
- [ ] Project status updated
```

## Version Numbering

We follow semantic versioning with phase alignment:
- `0.1.0` - Phase 1 complete (Move Generation)
- `0.2.0` - Phase 2 complete (Basic Search)
- `0.3.0` - Phase 3 complete (Optimizations)
- `0.4.0` - Phase 4 complete (NNUE)
- `0.5.0` - Phase 5 complete (Advanced Search)
- `0.6.0` - Phase 6 complete (Refinements)
- `1.0.0` - Phase 7 complete (Custom NNUE)

## Getting Help

- Review existing code for patterns and conventions
- Consult the Chess Programming Wiki
- Ask Claude AI for clarification on chess engine concepts
- Check the development journal for past decisions

## License and Legal

### Contributor License Agreement (CLA)

**Important**: By contributing to SeaJay, you must agree to the [Contributor License Agreement (CLA)](CLA.md). This agreement:
- Allows you to retain ownership of your contributions
- Grants the Project Owner (Brandon Harris) the right to use contributions under different licenses
- Enables dual-licensing model (GPL for open source, commercial licenses for businesses)

To accept the CLA, either:
1. Include "I have read and agree to the Contributor License Agreement" in your PR
2. Add `Signed-off-by: Your Name <email>` to your commits
3. Sign the CLA.md file directly

### Project License

SeaJay is licensed under the GNU General Public License v3.0 (GPL-3.0). This means:
- The source code is open and free for personal/educational use
- Any derivatives must also be GPL-3.0
- Commercial use under different terms requires a separate license from the Project Owner

### Why This Model?

This dual-licensing approach:
- Keeps the engine open source for the chess community
- Allows commercial licensing for sustainability
- Protects innovative discoveries that may have commercial value
- Ensures contributors are properly credited

## Recognition

All contributors will be recognized in the project README, including:
- Human contributors
- AI assistants that provided significant help
- Testing and feedback providers

Thank you for contributing to SeaJay Chess Engine!