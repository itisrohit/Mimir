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
    CPPFLAGS = -I/opt/homebrew/include
    LDFLAGS = -L/opt/homebrew/lib
else
    # Linux/Nix
    STD_LIB_FLAG =
    CPPFLAGS =
    LDFLAGS =
endif

CXXFLAGS = -std=c++17 -Wall -Wextra -g $(STD_LIB_FLAG) $(CPPFLAGS)
TARGET = mimir
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/session/SessionManager.cpp \
          $(SRCDIR)/document_processor/Chunker.cpp \
          $(SRCDIR)/config/ConfigManager.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -lcpr -o $(TARGET)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create config file if it doesn't exist
config:
	@if [ ! -f config.yaml ]; then \
		echo "📋 Creating default config.yaml..."; \
		cp config.yaml.example config.yaml; \
	fi

# Clean up build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -rf .data/   

# Run with config initialization
run: $(TARGET) config
	./$(TARGET)

# Python embedding pipeline dependencies
.PHONY: install-embed-deps
install-embed-deps:
	python3.11 -m venv venv
	source venv/bin/activate && pip install --upgrade pip
	source venv/bin/activate && pip install torch transformers sentence-transformers einops

# Run embedding pipeline test (from project root)
.PHONY: embed-test
embed-test:
	source venv/bin/activate && python scripts/embedding_pipeline.py < /dev/null

# Clean up model cache and temp files
.PHONY: clean-models
clean-models:
	rm -rf models/
	rm -rf ~/.cache/huggingface/
	rm -f /tmp/mimir_chunks.json

.PHONY: all clean run config

.PHONY: embedding-server
embedding-server:
	@echo "Starting embedding server..."
	@source venv/bin/activate && uvicorn embedding_server:app --host 127.0.0.1 --port 8000