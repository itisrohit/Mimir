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

# ONNX Runtime and Extensions paths (update these paths as needed)
ONNX_INCLUDE ?= /opt/homebrew/include/onnxruntime
ONNX_LIB ?= /opt/homebrew/lib
ONNXEXT_LIB ?= /opt/homebrew/lib

# Add ONNX to include and library paths
CPPFLAGS += -I$(ONNX_INCLUDE)
LDFLAGS += -L$(ONNX_LIB) -L$(ONNXEXT_LIB)

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

# Build the target executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -Wl,-rpath,/opt/homebrew/lib -lonnxruntime -lonnxruntime_extensions -o $(TARGET)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ONNX embedding test target
TEST_EMBED = test_onnx_embedding_pipeline
TEST_EMBED_SRC = src/embedding/test_embedding_pipeline.cpp

$(TEST_EMBED): $(TEST_EMBED_SRC) $(SRCDIR)/embedding/OnnxEmbedder.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-rpath,/opt/homebrew/lib -lonnxruntime -lonnxruntime_extensions -o $(TEST_EMBED) \
		$(TEST_EMBED_SRC) $(SRCDIR)/embedding/OnnxEmbedder.cpp

.PHONY: test-onnx
# Build and run the ONNX embedding pipeline test

test-onnx: $(TEST_EMBED)
	./$(TEST_EMBED)

# Create config file if it doesn't exist
config:
	@if [ ! -f config.yaml ]; then \
		echo "📋 Creating default config.yaml..."; \
		cp config.yaml.example config.yaml; \
	fi

# Clean up build files
clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_EMBED)
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

.PHONY: all clean run config test-onnx