

<div align="center">
  <img src="docs/assets/seajay-logo.png" alt="SeaJay Chess Engine Logo" width="200">
</div>


# SeaJay 

SeaJay is a UCI-compatible chess engine developed as an exploration of human-AI collaboration in software development. Named after a fictional hybrid of a Bluejay (known for memory and complex planning) and a Seagull (adaptable problem-solvers), SeaJay represents a fusion of human vision and AI implementation.

Unlike traditional chess engines focused on competitive performance, SeaJay prioritizes learning, transparency, and responsible AI development practices. Every line of code is AI-generated under human guidance, creating a unique development paradigm where human judgment shapes direction while AI handles execution.

We recognize that AI-assisted development represents a different approach from traditional programming methods. While this differs from the conventional path of directly applying years of C++ expertise and domain knowledge, effectively directing AI systems through complex chess engine logic presents its own unique challenges. Successfully guiding AI to implement sophisticated algorithms and handle edge cases requires developing a distinct skillset around prompt engineering, iterative refinement, and systematic validation. We believe both traditional and AI-assisted approaches have their respective merits and challenges in chess engine development.

SeaJay has been under active development since August 2025, and the rough ELO estimate as of September 2025 is ~2450.



## Core Principles

#### Transparent Attribution

Every line of code in SeaJay is AI-generated, and we openly acknowledge this. We believe in complete transparency about AI's role in development, celebrating human-AI collaboration as a core feature of the project. 

#### Human-Guided Intelligence

While AI writes the code, human judgment defines the vision, sets boundaries, and makes critical decisions about architecture. The human partner acts as architect, reviewer, and philosopher, ensuring purposeful development within ethical constraints.

#### Learning Through Collaboration

SeaJay prioritizes exploration and understanding over competitive performance. The goal isn't the strongest engine, but understanding what emerges when human creativity partners with machine execution. Success is measured in knowledge gained and shared.

#### Ethical Development Boundaries

We commit to responsible practices by:

- Being honest about failures and limitations
- Not using AI to circumvent understanding
- Ensuring the human partner maintains enough comprehension to take responsibility for the code
- Ensuring the AI isn't simply plagarizing other engine's code



## Features

- UCI protocol support
- A/B pruning with iterative deepening
- Q-search
- Magic Bitboards
- Transposition Tables
- Various hashing / caching strategies
- Killer / History move heuristics
- LMR / Null-move pruning
- MVV-LVA
- PVS



## Current State

Active development is messy. I've done my best to keep main neat, but the code is currently a bit disorganized and things are not as clean as I'd like. Many features and capabilities are in a "I'll fix that later" state. If you're wondering if you should test out SeaJay against stockfish, the answer is probably not. The following items have not yet been implemented, though are on the roadmap. The priority is sound functionality before expanding to widely.

- Multi-threaded / Multi-core support
- Tablebases
- Opening book support
- NNUE



## Building SeaJay

SeaJay is written in C++ and can be compiled on Windows, Linux, and macOS.

### Prerequisites

 - C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2019+)
  - CMake 3.16 or later
  - Make (for OpenBench and Linux/macOS builds)

### Compilation

#### Linux/macOS

```bash
  git clone https://github.com/nameBrandon/seajay-chess.git
  cd seajay-chess
  ./build.sh           # Release build (default)
  ./build.sh Debug     # Debug build with sanitizers
```
  The binary will be created at ./bin/seajay

#### Windows

  Option 1: MinGW/MSYS2 with Make
  ```bash
  git clone https://github.com/nameBrandon/seajay-chess.git
  cd seajay-chess
  mingw32-make         # Or just 'make' if using MSYS2
  ```
  Option 2: Visual Studio with CMake

  ```bash
  git clone https://github.com/nameBrandon/seajay-chess.git
  cd seajay-chess
  mkdir build
  cd build
  cmake -G "Visual Studio 17 2022" ..
  cmake --build . --config Release
  ```

Or open the solution in Visual Studio and build.

#### OpenBench Specific

```bash
  git clone https://github.com/nameBrandon/seajay-chess.git
  cd seajay-chess
  make                 # Uses AVX2/BMI2 optimizations

  Or with custom compiler:
  make CXX=clang++ EXE=seajay-ob
```

####  Build Notes

  - Local builds use conservative flags for maximum compatibility
  - OpenBench builds use AVX2/BMI2 optimizations for modern CPUs (2013+)
  - The build system automatically detects and uses appropriate optimizations
  - Debug builds include address sanitizer and debug symbols
  - Release builds are fully optimized for performance

## Usage

SeaJay supports the UCI protocol and can be used with any UCI-compatible chess GUI such as:

- Arena
- Cute Chess
- HIARCS
- ChessBase / Fritz
- Scid vs PC

### Basic Commands

```
uci           # Initialize UCI mode
isready       # Check if engine is ready
position fen [FEN] # Set position from FEN
position startpos moves [moves] # Set position from startpos
go depth [N]  # Search to depth N
go movetime [T] # Search for T milliseconds
go infinite   # Search until stop command
stop          # Stop searching
quit          # Exit the engine
```



## Development

SeaJay is tested with Andrew Grant's [OpenBench](https://github.com/AndyGrant/OpenBench) for performance validation. Development is ongoing with regular updates focusing on:

- Search improvements and optimizations
- Evaluation function tuning
- Time management refinements
- Bug fixes and stability improvements



## Terms of Use

SeaJay is free and open-source software. You can redistribute it and/or modify it under the terms of the GNU General Public License version 3.



## Acknowledgments

- The chess programming community for extensive documentation and resources, especially the [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page).
- **Andrew Grant** for the OpenBench framework for testing infrastructure. OpenBench alone has provided a level of rigor and testing history that would've been impossible to achieve without it, it's a truly brilliant project and has been indispensable. Thanks also to Andrew and company for suffering through numerous basic questions on Discord.
- UCI protocol designers **Stefan Meyer-Kahlen** and **Rudolf Huber**.
- Open-source engines -  In particular, thanks to the authors of :
  - [Publius](https://github.com/nescitus/publius)
  - [4ku](https://github.com/kz04px/4ku)
  - [stash](https://github.com/mhouppin/stash-bot/tree/9328141bc001913585fb76e6b38efe640eff2701)
  - [Laser](https://github.com/jeffreyan11/laser-chess-engine)
  - [Ethereal](https://github.com/AndyGrant/Ethereal)
  - [Stockfish](https://github.com/official-stockfish/Stockfish)
  - [Winter](https://github.com/rosenthj/Winter)




## Contact

For questions, suggestions, or discussions about SeaJay and AI-assisted development:

- Open an issue on GitHub
- Join the discussions in the Issues section
- Find me on Discord on the Engine Programming or OpenBench servers.

------

