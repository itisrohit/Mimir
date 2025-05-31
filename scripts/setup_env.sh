#!/bin/bash
set -e

echo "🚀 Setting up Mimir development environment..."

# Check if required tools are installed
command -v g++ >/dev/null 2>&1 || { echo "❌ g++ is required but not installed."; exit 1; }
command -v make >/dev/null 2>&1 || { echo "❌ make is required but not installed."; exit 1; }

echo "✅ C++ compiler found: $(g++ --version | head -n1)"
echo "✅ Make found: $(make --version | head -n1)"

# Create necessary directories
mkdir -p .data/sessions   
mkdir -p logs
mkdir -p src/session

# Set executable permissions for scripts
chmod +x scripts/*.sh

echo "✅ Development environment setup complete!"
echo "📁 Data will be stored in ./.data/sessions/"
echo "Run 'make' to build the project"