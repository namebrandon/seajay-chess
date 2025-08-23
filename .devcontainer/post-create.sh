#!/bin/bash

# SeaJay Chess Engine - Post-Create Setup Script
# This script runs after the devcontainer is created to set up the development environment

set -e

echo "=========================================="
echo "SeaJay Chess Engine - Environment Setup"
echo "=========================================="
echo ""

# Clear VS Code server cache to avoid extension conflicts between machines
echo "Clearing VS Code server cache to avoid cross-platform issues..."
rm -rf /home/developer/.vscode-server/extensions/* 2>/dev/null || true
rm -rf /home/developer/.vscode-server-insiders/extensions/* 2>/dev/null || true
echo "✓ VS Code cache cleared"
echo ""

# Configure git to match host settings (if available)
if [ -n "$GIT_USER_NAME" ]; then
    git config --global user.name "$GIT_USER_NAME"
fi
if [ -n "$GIT_USER_EMAIL" ]; then
    git config --global user.email "$GIT_USER_EMAIL"
fi

# Set up git editor and diff tool for VSCode
git config --global core.editor "code --wait"
git config --global diff.tool "vscode"
git config --global difftool.vscode.cmd 'code --wait --diff $LOCAL $REMOTE'
git config --global merge.tool "vscode"
git config --global mergetool.vscode.cmd 'code --wait $MERGED'

# Create build directory if it doesn't exist
if [ ! -d "/workspace/build" ]; then
    echo "Creating build directory..."
    mkdir -p /workspace/build
fi

# Set up external tools directory
if [ ! -d "/workspace/external" ]; then
    echo "Setting up external tools..."
    /workspace/tools/scripts/setup-external-tools.sh
else
    echo "External tools directory already exists."
fi

# Download test suites
echo ""
echo "Downloading chess test suites..."
mkdir -p /workspace/tests/positions

# Arasan 2023 Test Suite
if [ ! -f "/workspace/tests/positions/arasan2023.epd" ]; then
    echo "Downloading Arasan 2023 test suite..."
    wget -q -O /workspace/tests/positions/arasan2023.epd \
        https://www.arasanchess.org/arasan2023.epd
    echo "  ✓ Arasan 2023 test suite downloaded"
else
    echo "  ✓ Arasan 2023 test suite already exists"
fi

# Strategic Test Suite (STS)
if [ ! -f "/workspace/tests/positions/sts.zip" ]; then
    echo "Downloading Strategic Test Suite (STS)..."
    wget -q -O /workspace/tests/positions/sts.zip \
        https://github.com/fsmosca/STS-Rating-Program/raw/master/STS1-STS15_LAN_v3.zip
    unzip -q /workspace/tests/positions/sts.zip -d /workspace/tests/positions/sts/
    rm /workspace/tests/positions/sts.zip
    echo "  ✓ Strategic Test Suite downloaded and extracted"
else
    echo "  ✓ Strategic Test Suite already exists"
fi

# Eigenmann Rapid Engine Test (ERET)
if [ ! -f "/workspace/tests/positions/eret.epd" ]; then
    echo "Downloading ERET test suite..."
    # Note: Update URL if needed
    echo "  ! ERET download URL needs verification"
    # wget -q -O /workspace/tests/positions/eret.epd http://www.mathieupage.com/eret.epd
else
    echo "  ✓ ERET test suite already exists"
fi

# Create perft test positions file
if [ ! -f "/workspace/tests/positions/perft.txt" ]; then
    echo "Creating perft test positions..."
    cat > /workspace/tests/positions/perft.txt << 'EOF'
# Standard perft test positions
# Format: FEN | depth | expected_nodes
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1|6|119060324
r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|5|193690690
8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1|6|11030083
r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1|5|15833292
rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8|5|89941194
r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10|5|164075551
EOF
    echo "  ✓ Perft test positions created"
fi

# Set up ccache configuration
echo ""
echo "Configuring ccache..."
ccache --set-config max_size=5G
ccache --set-config compression=true
ccache --set-config compiler_check=content

# Create a simple build script
if [ ! -f "/workspace/build.sh" ]; then
    cat > /workspace/build.sh << 'EOF'
#!/bin/bash
# Quick build script for SeaJay

BUILD_TYPE=${1:-Release}
mkdir -p build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
ninja
echo "Build complete! Binary location: /workspace/bin/seajay"
EOF
    chmod +x /workspace/build.sh
    echo "  ✓ Build script created at /workspace/build.sh"
fi

# Initialize Claude configuration directory
mkdir -p /home/developer/.config/claude

# Display setup summary
echo ""
echo "=========================================="
echo "Environment Setup Complete!"
echo "=========================================="
echo ""
echo "Quick reference:"
echo "  • Build directory: /workspace/build"
echo "  • External tools: /workspace/external"
echo "  • Test positions: /workspace/tests/positions"
echo "  • Quick build: /workspace/build.sh [Debug|Release]"
echo ""
echo "Useful aliases available:"
echo "  • rebuild - Rebuild the project"
echo "  • perft - Run perft tests"
echo "  • bench - Run benchmarks"
echo "  • sprt - Run SPRT testing"
echo ""
echo "To get started with Phase 1:"
echo "  1. Create initial source files in /workspace/src/core/"
echo "  2. Run: /workspace/build.sh Debug"
echo "  3. Begin implementing board representation"
echo ""
echo "Happy coding! ♟️"