

<div align="center">
  <img src="docs/assets/seajay-logo.png" alt="SeaJay Chess Engine Logo" width="200">
</div>


# SeaJay 



## Overview

SeaJay is a UCI-compatible chess engine developed as an exploration of human-AI collaboration in software development. Named after a fictional hybrid of a Bluejay (known for memory and complex planning) and a Seagull (adaptable problem-solvers), SeaJay represents a fusion of human vision and AI implementation.

Unlike traditional chess engines focused on competitive performance, SeaJay prioritizes learning, transparency, and responsible AI development practices. Every line of code is AI-generated under human guidance, creating a unique development paradigm where human judgment shapes direction while AI handles execution.

SeaJay's rough ELO estimate as of August 2025 is ~2300.



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

- UCI (Universal Chess Interface) protocol support
- A/B pruning with iterative deepening
- Q-search
- Magic Bitboards
- Transposition Tables
- Zobrist Keys
- Killer / History move heuristics
- LMR / Null-move pruning
- MVV-LVA
- PVS



## Lack of features

If you're wondering if you should test out SeaJay against stockfish, the answer is probably not. The following items have not yet been implemented, though on the roadmap

- Multi-threaded / Multi-core support
- Tablebases
- Opening book support
- NNUE



## Building SeaJay

SeaJay is written in C++ and can be compiled on Windows, Linux, and macOS.

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 6+, or MSVC 2017+)
- Make (for Unix-like systems) or Visual Studio (for Windows)

### Compilation

#### Linux/macOS

```bash
git clone https://github.com/yourusername/seajay.git
cd seajay
make
```

#### Windows

```bash
git clone https://github.com/yourusername/seajay.git
cd seajay
make.exe
```

Or open the solution in Visual Studio and build.



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
- **Andrew Grant** for the OpenBench framework for testing infrastructure. OpenBench alone has provided a level of rigor and testing history that would've been impossible to achieve without it, it's a truly brilliant project and has been indispensable. Thanks to Andrew for suffering through numerous stupid questions on Discord.
- UCI protocol designers **Stefan Meyer-Kahlen** and **Rudolf Huber**.
- Open-source engines - So many to recognize here, I can't underscore enough the impact of having the ability to review examples of "good" for understanding complex (and at times, even simple) concepts. In particular, thanks to the authors of :
  - Pubilus
  - 4ku
  - stash
  - Laser
  - Ethereal
  - Stockfish
  - Winter




## Contact

For questions, suggestions, or discussions about SeaJay and AI-assisted development:

- Open an issue on GitHub
- Join our discussions in the Issues section

------

