#!/bin/bash

# Container configuration script for SeaJay Chess Engine development
# This script sets up the development environment with necessary tools

set -e  # Exit on error

echo "================================================"
echo "SeaJay Chess Engine - Container Configuration"
echo "================================================"

# Install Claude Code (Claude CLI) via npm
echo ""
echo "Installing Claude Code CLI..."
sudo npm install -g @anthropic-ai/claude-code

# Verify installation
echo ""
echo "Verifying Claude Code installation..."
claude --version

# Create yolo script for running Claude without permission checks
echo ""
echo "Creating yolo script for quick Claude access..."
cat > /workspace/yolo.sh << 'EOF'
#!/bin/bash
claude --dangerously-skip-permissions "$@"
EOF
chmod +x /workspace/yolo.sh
echo "yolo.sh script created and made executable"

echo ""
echo "================================================"
echo "Container configuration complete!"
echo "================================================"