CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET = mimir
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/session/SessionManager.cpp \
          $(SRCDIR)/document_processor/Chunker.cpp \
          $(SRCDIR)/config/ConfigManager.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Detect platform and compiler
UNAME_S := $(shell uname -s)
CXX_VERSION := $(shell $(CXX) --version 2>/dev/null | head -n1)

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

.PHONY: all clean run config