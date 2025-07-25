# Mimir

## Overview
Mimir is a document processing and embedding pipeline that supports high-quality semantic search and retrieval. It currently uses the state-of-the-art `nomic-ai/nomic-embed-text-v2-moe` embedding model for best-in-class embedding quality.

---

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

---

## Local (Non-Nix) Setup

If you don't use Nix, you can still set up manually:

```sh
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

---

## Notes
- The Nix shell provides all system tools (compilers, PDF tools, etc) so you don't need to install them globally.
- Python dependencies are managed with pip/venv for compatibility with most Python tooling.
- The Nix shell prints a welcome message and tool versions on entry.
- Your `venv/` and `models/` directories are ignored by git and Cursor IDE.

---

## Current Features
- High-quality embeddings using `nomic-ai/nomic-embed-text-v2-moe`
- FastAPI-based persistent embedding server (model loaded once, batched requests, auto-downloads model if needed)
- C++ pipeline with chunking, session management, and HTTP embedding calls
- Test script for end-to-end validation and benchmarking

---

## TODO / Future Improvements
- [ ] **Hardware Detection:** Automatically detect CPU/GPU and select the best embedding model for the hardware
- [ ] **Configurable Embedding Model:** Allow users to set the embedding model in `config.yaml` (already partially supported)
- [ ] **ONNX Runtime Support:** Convert and serve embedding models via ONNX Runtime for faster CPU inference
- [ ] **Model Quantization:** Support INT8/FP16 quantized models for even faster inference on CPU/GPU
- [ ] **User Model Selection:** Let users choose between "Best Quality" and "Fastest" embeddings at runtime
- [ ] **Efficient Storage:** Use binary formats (e.g., Parquet, npy) for large embedding tables
- [ ] **Profiling and Monitoring:** Add profiling hooks to measure and optimize pipeline bottlenecks
- [ ] **Batching and Streaming:** Support streaming and larger batch sizes for high-throughput scenarios

---

For questions or contributions, please open an issue or pull request!

## C++/CPR Compatibility on macOS (Homebrew + Nix)

**Note for macOS users:**

This project uses the CPR C++ library for HTTP. To ensure compatibility, you must use Homebrew’s clang++ and CPR:

1. **Install CPR and LLVM via Homebrew:**
   ```sh
   brew install cpr llvm
   ```
2. **Build as usual:**
   ```sh
   make clean && make
   ```
   The Makefile will auto-detect and use Homebrew’s clang++ if available, ensuring ABI compatibility with Homebrew's CPR.
3. **If you see linker errors about missing CPR symbols:**
   - Make sure you are not using Nix’s g++ to build.
   - The Makefile will fall back to system clang++ if Homebrew's is not found, but you may need to adjust your PATH or install Homebrew's LLVM.

**On Linux/Nix:**
- The build uses g++ or clang++ and system/Nix-provided libraries as usual.

**Summary:**
- Do not mix Nix GCC and Homebrew CPR on macOS.
- Use Homebrew’s clang++ and CPR together for ABI compatibility.
- The Makefile auto-selects the best compiler for your environment.

