# Mimir

⚡ The smartest way to talk to your data ⚡

---

## 🚀 Quick Start (Cross-Platform)

> **Recommended:** For most users, run the automated setup script:
> ```bash
> bash scripts/setup_env.sh
> ```
> This will install system dependencies, build tokenizers-cpp, and download the Jina model/tokenizer for you (macOS/Linux). See below for manual steps or if you are using Windows or Nix.
>
> **Nix users:** You can use the provided `flake.nix` and `flake.lock` for reproducible builds. (Nix support is experimental and may require updates.)

### Prerequisites
- C++17 compiler (GCC, Clang, or MSVC)
- Python 3.11+ (for model download/check scripts)
- CMake (for building tokenizers-cpp)
- Git (for submodules)

### 1. Install System Dependencies

#### **macOS (Homebrew):**
```bash
brew install cmake onnxruntime
# For tokenizers-cpp, you need Rust and Ninja:
brew install rust ninja
```

#### **Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake python3 python3-pip git ninja-build
# ONNX Runtime (download prebuilt from https://onnxruntime.ai/ or build from source)
# For tokenizers-cpp, you need Rust:
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

#### **Windows:**
- Install [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
- Install [CMake](https://cmake.org/download/)
- Install [ONNX Runtime for Windows](https://onnxruntime.ai/)
- Install [Rust](https://rustup.rs/)
- Install [Ninja](https://ninja-build.org/)

### 2. Clone and Prepare the Repo
```bash
git clone <repository-url>
cd Mimir
# If using submodules (for tokenizers-cpp):
git submodule update --init --recursive
```

### 3. Download Model and Tokenizer
```bash
# Use the provided script (edit paths as needed):
python3 scripts/download_jina_v3_with_check.py
```

### 4. Build tokenizers-cpp (if not using system package)
```bash
# Clone and build mlc-ai/tokenizers-cpp (see https://github.com/mlc-ai/tokenizers-cpp)
git clone https://github.com/mlc-ai/tokenizers-cpp.git
cd tokenizers-cpp
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
cd ../..
```

### 5. Build Mimir
```bash
make clean && make
```

### 6. Test the ONNX Embedding Pipeline
```bash
make test-onnx-full
./test_onnx_embedding_pipeline
```

---

## 🛠️ Troubleshooting
- **Linker errors for ONNX/tokenizers:** Double-check your `Makefile` include and lib paths. Use `ls /usr/local/lib` and `ls /opt/homebrew/lib` to verify.
- **tokenizers-cpp build errors:** Ensure Rust, Ninja, and CMake are installed. Run `git submodule update --init --recursive` in the tokenizers-cpp directory.
- **ONNX Runtime not found:** Download the prebuilt binaries from [onnxruntime.ai](https://onnxruntime.ai/) or install via package manager.
- **Model output name errors:** Use Netron or the debug printout to find the correct ONNX output name (e.g., `text_embeds`).
- **task_id errors:** The Jina v3 model requires a `task_id` input. This is handled in the C++ code, but if you use a different model, check its input signature.

---

## Features

- **Document Processing**: Support for TXT, PDF, Markdown files with intelligent chunking
- **Semantic Search**: Vector-based document retrieval using embeddings
- **Session Management**: Persistent sessions with auto-save functionality
- **ONNX Embedding Integration**: Direct C++ embedding generation using ONNX Runtime and HuggingFace Tokenizers
- **Cross-Platform**: Works on macOS, Linux, and Windows

## Usage

### Basic Commands

- `init <session_name>` - Create a new session
- `add-doc <file_path>` - Add a document to the current session
- `query <question>` - Ask a question about your documents
- `list` - List all sessions
- `info` - Show current session information
- `export <session_name>` - Export a session
- `quit` - Exit the application

### Example Workflow

```bash
# Start Mimir
./mimir

# Create a session
mimir> init my_documents

# Add documents
mimir> add-doc document1.pdf
mimir> add-doc document2.txt

# Ask questions
mimir> query What is the main topic of the documents?

# Export results
mimir> export my_documents
```

## Architecture

### Core Components

- **DocumentProcessor**: Handles file parsing and chunking
- **OnnxEmbedder**: Generates embeddings using ONNX Runtime
- **SessionManager**: Manages sessions and document storage
- **ConfigManager**: Handles configuration loading and validation

### Embedding Pipeline

1. **Document Chunking**: Text is split into semantic chunks
2. **Tokenization**: Chunks are tokenized using HuggingFace Tokenizers
3. **ONNX Inference**: Embeddings are generated using ONNX Runtime
4. **Storage**: Embeddings are stored with document chunks for retrieval

## Development

### Building from Source

```bash
# Clean build
make clean

# Build with debug symbols
make CXXFLAGS="-g -O0"

# Build with optimizations
make CXXFLAGS="-O3"
```

### Testing

```bash
# Test the embedding pipeline
make test-onnx

# Run integration tests
bash scripts/test_workflow.sh
```

## Configuration

Edit `config.yaml` to customize:

- Document processing settings (chunk size, overlap)
- Embedding model parameters
- Vector database settings
- Performance options

## License

[Add your license information here]

## Contributing

[Add contribution guidelines here]

