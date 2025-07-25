CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g $(STD_LIB_FLAG) -I/opt/homebrew/include
TARGET = mimir
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/session/SessionManager.cpp \
          $(SRCDIR)/document_processor/Chunker.cpp \
          $(SRCDIR)/config/ConfigManager.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Detect platform and compiler
UNAME_S := $(shell uname -s)
CXX_VERSION := $(shell $(CXX) --version)

# Detect compiler and set stdlib flag if needed
ifeq ($(findstring clang,$(CXX_VERSION)),clang)
    STD_LIB_FLAG = -stdlib=libc++
else
    STD_LIB_FLAG =
endif

# Compiler-specific flags
ifeq ($(findstring clang,$(CXX_VERSION)),clang)
    # Clang-specific flags
    CXXFLAGS += -stdlib=libc++
    LDFLAGS += -stdlib=libc++
else ifeq ($(findstring g++,$(CXX_VERSION)),g++)
    # GCC-specific flags - remove the incorrect -lstdc++ from CXXFLAGS
    # GCC uses libstdc++ by default, no need to specify
else
    # Default/unknown compiler
    $(warning Unknown compiler: $(CXX_VERSION))
endif

# Platform-specific flags
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lstdc++fs
endif
ifeq ($(UNAME_S),Darwin)
    # Only add filesystem library if using GCC on macOS
    ifeq ($(findstring g++,$(CXX_VERSION)),g++)
        LDFLAGS += -lstdc++fs
    endif
endif

# Compiler selection logic for cross-platform compatibility
ifeq ($(shell uname),Darwin)
  # On macOS, prefer Homebrew's clang++ if available
  HOMEBREW_CLANG := /opt/homebrew/opt/llvm/bin/clang++
  ifeq ($(shell test -x $(HOMEBREW_CLANG) && echo yes),yes)
    CXX := $(HOMEBREW_CLANG)
  else
    CXX := clang++
  endif
else
  # On Linux/Nix, prefer clang++ if available, else g++
  ifeq ($(shell command -v clang++ >/dev/null 2>&1 && echo yes),yes)
    CXX := clang++
  else
    CXX := g++
  endif
endif

CXXFLAGS = -std=c++17 -Wall -Wextra -g $(STD_LIB_FLAG) -I/opt/homebrew/include

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

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

# Remove Homebrew-specific include/lib paths
# Use only system/Nix includes and libraries
CXX ?= g++
CXX_VERSION := $(shell $(CXX) --version)
ifeq ($(findstring clang,$(CXX_VERSION)),clang)
    STD_LIB_FLAG = -stdlib=libc++
else
    STD_LIB_FLAG =
endif

CXXFLAGS = -std=c++17 -Wall -Wextra -g $(STD_LIB_FLAG) -I/opt/homebrew/include

src/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/session/SessionManager.o: src/session/SessionManager.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/document_processor/Chunker.o: src/document_processor/Chunker.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/config/ConfigManager.o: src/config/ConfigManager.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

mimir: src/main.o src/session/SessionManager.o src/document_processor/Chunker.o src/config/ConfigManager.o
	$(CXX) $^ $(STD_LIB_FLAG) -L/opt/homebrew/lib -lcpr -o $@

.PHONY: embedding-server
embedding-server:
	@echo "Starting embedding server..."
	@source venv/bin/activate && uvicorn embedding_server:app --host 127.0.0.1 --port 8000