#!/bin/bash

# Setup script for machine-specific devcontainer configuration
# Run this on each machine to prepare the local environment

set -e

echo "SeaJay Chess Engine - Machine Setup"
echo "===================================="
echo ""

# Get machine name
if [ -z "$1" ]; then
    echo "Please provide a machine name (e.g., 'macbook' or 'studio')"
    echo "Usage: ./setup-machine.sh <machine-name>"
    exit 1
fi

MACHINE_NAME=$1
echo "Setting up for machine: $MACHINE_NAME"

# Create local Docker volumes directory (outside iCloud)
echo "Creating local Docker cache directories..."
mkdir -p ~/.seajay-docker/{ccache,bash_history,zsh_history,config,vscode-server,vscode-server-insiders}

# Create .env file if it doesn't exist
if [ ! -f ".env" ]; then
    echo "Creating .env file..."
    cp .env.template .env
    # Update machine name in .env
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "s/MACHINE_NAME=.*/MACHINE_NAME=$MACHINE_NAME/" .env
    else
        sed -i "s/MACHINE_NAME=.*/MACHINE_NAME=$MACHINE_NAME/" .env
    fi
    echo "✓ Created .env file - please review and adjust settings"
else
    echo "✓ .env file already exists"
fi

# Add .env to .gitignore if not already there
if ! grep -q "^\.env$" ../.gitignore 2>/dev/null; then
    echo "Adding .env to .gitignore..."
    echo -e "\n# Machine-specific environment\n.env" >> ../.gitignore
fi

echo ""
echo "Setup complete!"
echo ""
echo "Next steps:"
echo "1. Review and edit .devcontainer/.env for your machine's specifications"
echo "2. Ensure Docker Desktop is running with sufficient resources allocated"
echo "3. Open the project in VS Code and rebuild the container"
echo ""
echo "Note: The container uses linux/amd64 platform for chess engine compatibility."
echo "This may run slower on Apple Silicon but ensures consistent behavior."