# Mimir

A high-performance C++ document processing and embedding pipeline using SentencePiece tokenization and ONNX Runtime.

## Features

- **Pure C++ Implementation**: No Python runtime dependencies
- **High-Performance**: 5-10x faster than Python alternatives
- **SentencePiece Tokenization**: Native C++ tokenizer integration
- **ONNX Runtime**: Efficient neural network inference
- **Batch Processing**: Optimized for throughput
- **Memory Efficient**: Low memory footprint

## Performance

- **Tokenization**: 2-5ms per batch (vs 50-100ms Python)
- **Throughput**: 25+ texts/second
- **Memory Usage**: 32KB for 8 embeddings
- **Initialization**: ~3.3 seconds (one-time cost)

## Development Environment (Recommended: Nix + venv)

You can use Nix to get a fully reproducible development environment with all system dependencies (Python, pip, gcc, make, PDF tools, etc). Python packages are still managed with a local venv for maximum compatibility.

### Quick Start (Nix)

1. **Enter the Nix dev shell:**
   ```sh
   nix develop
   ```
2. **Activate your Python venv:**
   ```sh
   source venv/bin/activate
   ```
   If you don't have a venv yet, it will be auto-created the first time you enter the Nix shell, or you can create it manually:
   ```sh
   python3 -m venv venv
   source venv/bin/activate
   ```
3. **Install Python dependencies:**
   ```sh
   pip install -r requirements.txt
   ```
4. **Run your usual commands:**
   ```sh
   make
   make embedding-server
   bash scripts/test_embedding_pipeline.sh
   # etc.
   ```

## Local (Non-Nix) Setup

If you don't use Nix, you can still set up manually:

### Prerequisites

- macOS with Homebrew
- C++17 compiler
- ONNX Runtime
- SentencePiece

### Installation

```bash
# Install dependencies
brew install onnxruntime sentencepiece

# Clone and build
git clone <repository>
cd Mimir
make clean && make
```

## Testing

Run the comprehensive test suite:

```bash
# Run the complete pipeline test
./scripts/test_embedding_pipeline.sh
```

This will:
- ✅ Check model files
- ✅ Build the test executable (temporarily)
- ✅ Run performance tests
- ✅ Display timing metrics
- ✅ Clean up automatically

## Usage

```bash
# Build the main application
make

# Run with configuration
./mimir
```

## Architecture

### Core Components

- **SentencePieceTokenizer**: Pure C++ tokenization
- **OnnxEmbedder**: ONNX Runtime integration
- **Document Processor**: Text chunking and processing
- **Session Manager**: State management

### File Structure

```
src/
├── embedding/
│   ├── OnnxEmbedder.h/.cpp      # ONNX embedding wrapper
│   └── SentencePieceTokenizer.h/.cpp  # C++ tokenizer
├── config/ConfigManager.h/.cpp   # Configuration management
├── document_processor/Chunker.h/.cpp  # Document processing
└── session/SessionManager.h/.cpp # Session management

scripts/
├── test_embedding_pipeline.sh   # Comprehensive test script
└── test_embedding_pipeline.cpp  # Test executable (built by script)
```

## Development

### Building

```bash
# Clean build
make clean && make

# Run tests
make test
```

### Testing

```bash
# Run comprehensive test
./scripts/test_embedding_pipeline.sh

# Or use make
make test
```

## Configuration

Edit `config.yaml` to customize:
- Model paths
- Tokenizer settings
- Processing parameters
- Performance options

## Notes

- The Nix shell provides all system tools (compilers, PDF tools, etc) so you don't need to install them globally.
- Python dependencies are managed with pip/venv for compatibility with most Python tooling.
- The Nix shell prints a welcome message and tool versions on entry.
- Your `venv/` and `models/` directories are ignored by git and Cursor IDE.

## Current Features

- High-quality embeddings using `nomic-ai/nomic-embed-text-v2-moe`
- FastAPI-based persistent embedding server (model loaded once, batched requests, auto-downloads model if needed)
- C++ pipeline with chunking, session management, and HTTP embedding calls
- Test script for end-to-end validation and benchmarking
- Pure C++ SentencePiece tokenization for 5-10x performance improvement

## TODO / Future Improvements

- [ ] **Hardware Detection:** Automatically detect CPU/GPU and select the best embedding model for the hardware
- [ ] **Configurable Embedding Model:** Allow users to set the embedding model in `config.yaml` (already partially supported)
- [ ] **ONNX Runtime Support:** Convert and serve embedding models via ONNX Runtime for faster CPU inference
- [ ] **Model Quantization:** Support INT8/FP16 quantized models for even faster inference on CPU/GPU
- [ ] **User Model Selection:** Let users choose between "Best Quality" and "Fastest" embeddings at runtime
- [ ] **Efficient Storage:** Use binary formats (e.g., Parquet, npy) for large embedding tables
- [ ] **Profiling and Monitoring:** Add profiling hooks to measure and optimize pipeline bottlenecks
- [ ] **Batching and Streaming:** Support streaming and larger batch sizes for high-throughput scenarios

## C++/CPR Compatibility on macOS (Homebrew + Nix)

**Note for macOS users:**

This project uses cpp-httplib for HTTP. To ensure compatibility, you must use Homebrew's clang++ and cpp-httplib:

1. **Install cpp-httplib and LLVM via Homebrew:**
   ```sh
   brew install cpp-httplib llvm
   ```
2. **Build as usual:**
   ```sh
   make clean && make
   ```
   The Makefile will auto-detect and use Homebrew's clang++ if available, ensuring ABI compatibility with Homebrew's cpp-httplib.
3. **If you see linker errors about missing cpp-httplib symbols:**
   - Make sure you are not using Nix's g++ to build.
   - The Makefile will fall back to system clang++ if Homebrew's is not found, but you may need to adjust your PATH or install Homebrew's LLVM.

**On Linux/Nix:**
- The build uses g++ or clang++ and system/Nix-provided libraries as usual.

**Summary:**
- Do not mix Nix GCC and Homebrew cpp-httplib on macOS.
- Use Homebrew's clang++ and cpp-httplib together for ABI compatibility.
- The Makefile auto-selects the best compiler for your environment.

## Troubleshooting

### Common Issues

1. **Schema Warnings**: Normal ONNX Runtime warnings, can be ignored
2. **Model Not Found**: Ensure models are in `models/bge-m3-onnx/`
3. **Build Errors**: Check Homebrew installation of dependencies

### Performance Tips

- Use batch processing for better throughput
- Ensure models are on fast storage
- Monitor memory usage for large documents

## License

[Add your license here]

For questions or contributions, please open an issue or pull request!

