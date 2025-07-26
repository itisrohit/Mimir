#!/bin/bash
set -e

# Modern setup script for Mimir (cross-platform)
echo "🚀 Setting up Mimir development environment..."

# Detect platform
UNAME=$(uname)
if [[ "$UNAME" == "Darwin" ]]; then
    PLATFORM="macos"
elif [[ "$UNAME" == "Linux" ]]; then
    PLATFORM="linux"
else
    PLATFORM="other"
fi

# 1. Check for C++ compiler and Make
command -v g++ >/dev/null 2>&1 || { echo "❌ g++ is required but not installed."; exit 1; }
command -v make >/dev/null 2>&1 || { echo "❌ make is required but not installed."; exit 1; }
echo "✅ C++ compiler found: $(g++ --version | head -n1)"
echo "✅ Make found: $(make --version | head -n1)"

# 2. Install system dependencies
if [[ "$PLATFORM" == "macos" ]]; then
    echo "🔹 Installing dependencies with Homebrew..."
    brew install cmake onnxruntime rust ninja || true
elif [[ "$PLATFORM" == "linux" ]]; then
    echo "🔹 Installing dependencies with apt..."
    sudo apt update
    sudo apt install -y build-essential cmake python3 python3-pip git ninja-build || true
    if ! command -v rustc >/dev/null 2>&1; then
        echo "🔹 Installing Rust..."
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
        export PATH="$HOME/.cargo/bin:$PATH"
    fi
    echo "🔹 ONNX Runtime: Please download prebuilt binaries from https://onnxruntime.ai/ if not available in your package manager."
else
    echo "⚠️  Please install CMake, ONNX Runtime, Rust, and Ninja manually for your platform."
fi

# 3. Build tokenizers-cpp if not installed
if ! ldconfig -p 2>/dev/null | grep -q tokenizers_cpp && ! ls /usr/local/lib/libtokenizers_cpp* 1>/dev/null 2>&1 && ! ls /opt/homebrew/lib/libtokenizers_cpp* 1>/dev/null 2>&1; then
    echo "🔹 Building mlc-ai/tokenizers-cpp..."
    git clone https://github.com/mlc-ai/tokenizers-cpp.git || true
    cd tokenizers-cpp
    git submodule update --init --recursive
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc || sysctl -n hw.ncpu)
    sudo make install
    cd ../..
else
    echo "✅ tokenizers-cpp already installed."
fi

# 4. Download Jina model and tokenizer
if [ -f scripts/download_jina_v3_with_check.py ]; then
    echo "🔹 Downloading Jina v3 model and tokenizer..."
    python3 scripts/download_jina_v3_with_check.py
else
    echo "⚠️  Model download script not found. Please download the model and tokenizer manually."
fi

# 5. Set script permissions
chmod +x scripts/*.sh

echo "✅ Development environment setup complete!"
echo "👉 Next: Run 'make clean && make' to build the project."
echo "👉 To test ONNX embedding: 'make test-onnx-full && ./test_onnx_embedding_pipeline'"