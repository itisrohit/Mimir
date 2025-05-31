#!/bin/bash
set -e

echo "🔨 Building Mimir..."

# Clean previous builds
make clean

# Build the project
make all

echo "✅ Build completed successfully."
echo "Run with: ./mimir"