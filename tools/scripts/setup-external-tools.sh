#!/bin/bash

# SeaJay Chess Engine - External Tools Setup Script
# This script downloads and builds external tools needed for testing and development

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
EXTERNAL_DIR="$PROJECT_ROOT/external"

echo "SeaJay Chess Engine - External Tools Setup"
echo "==========================================="
echo "Project root: $PROJECT_ROOT"
echo ""

# Create external directories
echo "Creating external directories..."
mkdir -p "$EXTERNAL_DIR"/{engines,testers,books,tablebases,networks}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
echo "Checking prerequisites..."
if ! command_exists git; then
    echo "Error: git is required but not installed."
    exit 1
fi

if ! command_exists make; then
    echo "Error: make is required but not installed."
    exit 1
fi

if ! command_exists g++; then
    if ! command_exists clang++; then
        echo "Error: C++ compiler (g++ or clang++) is required but not installed."
        exit 1
    fi
fi

# Download and build Stockfish
echo ""
echo "Setting up Stockfish..."
if [ ! -d "$EXTERNAL_DIR/engines/stockfish" ]; then
    echo "Cloning Stockfish repository..."
    git clone --depth 1 https://github.com/official-stockfish/Stockfish.git "$EXTERNAL_DIR/engines/stockfish"
    
    echo "Building Stockfish..."
    cd "$EXTERNAL_DIR/engines/stockfish/src"
    
    # Detect architecture for Stockfish build
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        if [[ $(uname -m) == "arm64" ]]; then
            make -j build ARCH=apple-silicon
        else
            make -j build ARCH=x86-64-modern
        fi
    else
        # Linux/Unix
        make -j build ARCH=x86-64-modern
    fi
    
    echo "Stockfish built successfully!"
else
    echo "Stockfish already exists, skipping..."
fi

# Download and build fast-chess
echo ""
echo "Setting up fast-chess..."
if [ ! -d "$EXTERNAL_DIR/testers/fast-chess" ]; then
    echo "Cloning fast-chess repository..."
    git clone --depth 1 https://github.com/Disservin/fast-chess.git "$EXTERNAL_DIR/testers/fast-chess"
    
    echo "Building fast-chess..."
    cd "$EXTERNAL_DIR/testers/fast-chess"
    make -j
    
    echo "fast-chess built successfully!"
else
    echo "fast-chess already exists, skipping..."
fi

# Download opening books
echo ""
echo "Setting up opening books..."
cd "$EXTERNAL_DIR/books"

# 8moves_v3.pgn for testing
if [ ! -f "8moves_v3.pgn" ]; then
    echo "Downloading 8moves_v3.pgn..."
    # Note: Replace with actual URL when available
    echo "Warning: 8moves_v3.pgn URL not configured. Please download manually from:"
    echo "  https://github.com/official-stockfish/books"
    echo "  and place in: $EXTERNAL_DIR/books/"
else
    echo "8moves_v3.pgn already exists, skipping..."
fi

# Create a simple 4-move opening book for very early testing
if [ ! -f "4moves_test.pgn" ]; then
    echo "Creating simple test opening book..."
    cat > 4moves_test.pgn << 'EOF'
[Event "Test Opening"]
[Result "*"]

1. e4 e5 *

[Event "Test Opening"]
[Result "*"]

1. d4 d5 *

[Event "Test Opening"]
[Result "*"]

1. e4 c5 *

[Event "Test Opening"]
[Result "*"]

1. d4 Nf6 *
EOF
    echo "Test opening book created."
fi

# Download a default NNUE network (for Phase 4)
echo ""
echo "Setting up NNUE networks..."
if [ ! -f "$EXTERNAL_DIR/networks/nn-0000000000a0.nnue" ]; then
    echo "Note: NNUE networks will be downloaded in Phase 4"
    echo "  Networks can be obtained from:"
    echo "  https://tests.stockfishchess.org/nns"
else
    echo "NNUE network already exists, skipping..."
fi

# Create configuration files
echo ""
echo "Creating configuration files..."

# Create engine configuration
cat > "$PROJECT_ROOT/tools/configs/engines.json" << EOF
{
  "engines": {
    "seajay": {
      "path": "./bin/seajay",
      "options": {}
    },
    "stockfish": {
      "path": "./external/engines/stockfish/src/stockfish",
      "options": {
        "Threads": 1,
        "Hash": 64
      }
    }
  }
}
EOF

# Create SPRT testing configuration
cat > "$PROJECT_ROOT/tools/configs/sprt-config.json" << EOF
{
  "default": {
    "elo0": 0,
    "elo1": 5,
    "alpha": 0.05,
    "beta": 0.05,
    "time_control": "8+0.08",
    "games": 30000,
    "concurrency": 4,
    "book": "./external/books/4moves_test.pgn"
  },
  "major_feature": {
    "elo0": 0,
    "elo1": 10,
    "alpha": 0.05,
    "beta": 0.05
  },
  "non_regression": {
    "elo0": -3,
    "elo1": 0,
    "alpha": 0.05,
    "beta": 0.05
  }
}
EOF

echo ""
echo "Setup complete!"
echo ""
echo "Summary:"
echo "--------"
echo "✓ Directory structure created"
echo "✓ Stockfish installed at: $EXTERNAL_DIR/engines/stockfish/"
echo "✓ fast-chess installed at: $EXTERNAL_DIR/testers/fast-chess/"
echo "✓ Configuration files created"
echo ""
echo "Note: Some resources (opening books, NNUE networks) need manual download."
echo "See project documentation for details."