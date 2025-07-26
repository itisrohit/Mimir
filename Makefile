# Platform and compiler detection
UNAME_S := $(shell uname -s)
CXX ?= g++
CXX_VERSION := $(shell $(CXX) --version)

# Platform-specific flags
ifeq ($(UNAME_S),Darwin)
    # macOS (Homebrew)
    HOMEBREW_CLANG := /opt/homebrew/opt/llvm/bin/clang++
    ifeq ($(shell test -x $(HOMEBREW_CLANG) && echo yes),yes)
        CXX := $(HOMEBREW_CLANG)
    else
        CXX := clang++
    endif
    STD_LIB_FLAG = -stdlib=libc++
    CPPFLAGS = -I/opt/homebrew/include -I./include -I./src
    LDFLAGS = -L/opt/homebrew/lib
else
    # Linux/Nix
    STD_LIB_FLAG =
    CPPFLAGS = -I./include -I./src
    LDFLAGS =
endif

# ONNX Runtime and Tokenizers paths (update these paths as needed)
ONNX_INCLUDE ?= /opt/homebrew/include/onnxruntime
ONNX_LIB ?= /opt/homebrew/lib
TOKENIZERS_INCLUDE ?= /usr/local/include
TOKENIZERS_LIB ?= /usr/local/lib

# Add ONNX and Tokenizers to include and library paths
CPPFLAGS += -I$(ONNX_INCLUDE) -I$(TOKENIZERS_INCLUDE)
LDFLAGS += -L$(ONNX_LIB) -L$(TOKENIZERS_LIB)

CXXFLAGS = -std=c++17 -Wall -Wextra -g $(STD_LIB_FLAG) $(CPPFLAGS)
TARGET = mimir
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/session/SessionManager.cpp \
          $(SRCDIR)/document_processor/Chunker.cpp \
          $(SRCDIR)/config/ConfigManager.cpp \
          $(SRCDIR)/embedding/OnnxEmbedder.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the target executable (without external libraries for testing)
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -ltokenizers_cpp -ltokenizers_c -lonnxruntime -o $(TARGET)

# Build with ONNX Runtime and Tokenizers (when libraries are installed)
$(TARGET)-full: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -ltokenizers_cpp -ltokenizers_c -lonnxruntime -o $(TARGET)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ONNX embedding test target (without external libraries for testing)
test-onnx: scripts/test_onnx_embedding_pipeline.cpp $(SRCDIR)/embedding/OnnxEmbedder.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o test_onnx_embedding_pipeline \
		scripts/test_onnx_embedding_pipeline.cpp $(SRCDIR)/embedding/OnnxEmbedder.cpp

# ONNX embedding test target (with external libraries)
test-onnx-full: scripts/test_onnx_embedding_pipeline.cpp $(SRCDIR)/embedding/OnnxEmbedder.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -ltokenizers_cpp -ltokenizers_c -lonnxruntime -o test_onnx_embedding_pipeline \
		scripts/test_onnx_embedding_pipeline.cpp $(SRCDIR)/embedding/OnnxEmbedder.cpp

# Create config file if it doesn't exist
config:
	@if [ ! -f config.yaml ]; then \
		echo "📋 Creating default config.yaml..."; \
		cp config.yaml.example config.yaml; \
	fi

# Clean up build files
clean:
	rm -f $(OBJECTS) $(TARGET) test_onnx_embedding_pipeline
	rm -rf .data/   

# Run with config initialization
run: $(TARGET) config
	./$(TARGET)

# Python embedding pipeline dependencies
.PHONY: install-embed-deps
install-embed-deps:
	python3.11 -m venv venv
	source venv/bin/activate && pip install --upgrade pip
	source venv/bin/activate && pip install -r requirements.txt

# ONNX Runtime and Tokenizers installation help
.PHONY: help-onnx
help-onnx:
	@echo "📋 ONNX Runtime and Tokenizers C++ Installation:"
	@echo "1. Install ONNX Runtime C++:"
	@echo "   - Download from: https://github.com/microsoft/onnxruntime/releases"
	@echo "   - Or build from source: https://github.com/microsoft/onnxruntime"
	@echo ""
	@echo "2. Install HuggingFace Tokenizers C++:"
	@echo "   - Build from source: https://github.com/huggingface/tokenizers/tree/main/bindings/cpp"
	@echo "   - Or use package manager if available"
	@echo ""
	@echo "3. Update ONNX_INCLUDE, ONNX_LIB, TOKENIZERS_INCLUDE, TOKENIZERS_LIB in Makefile"
	@echo "   to point to your installation paths"
	@echo ""
	@echo "4. Run: make test-onnx"