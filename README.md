

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

- October 2025 focus: the queen-sack tactical remediation (QS-series) and the broader evaluation bias initiative. QS2 is now in-progress with reinforced history bonuses and contact-check replay in search; telemetry reruns are queued to confirm coverage ≥12/20 before shifting to QS3 evaluation reinforcements.
- Latest Release build benchmark: `bench 2501279` nodes on the reference workstation (Release build via `./build.sh Release`, single-threaded bench command `echo "bench" | ./bin/seajay`).

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

#### Windows (MSYS2 MinGW/UCRT64)

SeaJay currently builds reliably on Windows through the MSYS2 UCRT64 environment. These steps assume a fresh MSYS2 install.

1. Launch **MSYS2 UCRT64** (not MSYS or MINGW32).
2. Install the required toolchain packages (safe to re-run to pick up updates):
   ```bash
   pacman -S --needed \
       base base-devel git cmake python \
       mingw-w64-ucrt-x86_64-{binutils,cmake,gcc,gcc-libgfortran,gdb,gdb-multiarch,make,ninja,pkgconf,tools-git,winpthreads,winstorecompat-git}
   ```
   The development machine that validated the Windows build also had the following packages available:
   ```
   base 2022.06-1
   base-devel 2024.11-1
   cmake 4.1.1-1
   filesystem 2025.05.08-2
   gcc 15.2.0-1
   git 2.51.0-1
   mingw-w64-ucrt-x86_64-binutils 2.45-2
   mingw-w64-ucrt-x86_64-crt-git 13.0.0.r167.g2e31630bc-1
   mingw-w64-ucrt-x86_64-gcc 15.2.0-8
   mingw-w64-ucrt-x86_64-gcc-libgfortran 15.2.0-8
   mingw-w64-ucrt-x86_64-gdb 16.3-1
   mingw-w64-ucrt-x86_64-gdb-multiarch 16.3-1
   mingw-w64-ucrt-x86_64-headers-git 13.0.0.r167.g2e31630bc-1
   mingw-w64-ucrt-x86_64-libmangle-git 13.0.0.r167.g2e31630bc-1
   mingw-w64-ucrt-x86_64-libwinpthread 13.0.0.r167.g2e31630bc-1
   mingw-w64-ucrt-x86_64-make 4.4.1-3
   mingw-w64-ucrt-x86_64-ninja 1.13.1-1
   mingw-w64-ucrt-x86_64-pkgconf 1~2.5.1-1
   mingw-w64-ucrt-x86_64-tools-git 13.0.0.r167.g2e31630bc-1
   mingw-w64-ucrt-x86_64-winpthreads 13.0.0.r167.g2e31630bc-1
   mingw-w64-ucrt-x86_64-winstorecompat-git 13.0.0.r167.g2e31630bc-1
   mingw-w64-x86_64-cmake 4.1.1-1
   mingw-w64-x86_64-gcc 15.2.0-8
   mingw-w64-x86_64-make 4.4.1-3
   mingw-w64-x86_64-ninja 1.13.1-1
   msys2-runtime 3.6.4-1
   python 3.12.11-1
   ```
3. Build with the provided helper script (wraps CMake/Make for you):
   ```bash
   git clone https://github.com/nameBrandon/seajay-chess.git
   cd seajay-chess
   ./build.sh Release   # or ./build.sh Debug
   ```
   The script emits the final binary to `bin/seajay.exe` so the layout matches Linux/macOS installs.

❗ At this time Visual Studio/MSVC builds are not supported. The project leans on GCC/MinGW behaviour and the MSVC toolchain currently fails on a number of headers (e.g. `__attribute__((always_inline))` usage in `magic_bitboards.h`). Use the MSYS2 MinGW route above instead of Visual Studio.

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
  - [Perseus](https://github.com/TheRealGioviok/Perseus-Engine)
  - [Ethereal](https://github.com/AndyGrant/Ethereal)
  - [Stockfish](https://github.com/official-stockfish/Stockfish)
  - [Winter](https://github.com/rosenthj/Winter)




## Contact

For questions, suggestions, or discussions about SeaJay and AI-assisted development:

- Open an issue on GitHub
- Join the discussions in the Issues section
- Find me on Discord on the Engine Programming or OpenBench servers.

------
