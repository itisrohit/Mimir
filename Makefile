CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET = mimir
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/session/SessionManager.cpp \
          $(SRCDIR)/document_processor/Chunker.cpp \
          $(SRCDIR)/config/ConfigManager.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Platform-specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Linux uses libstdc++ by default
    LDFLAGS += -lstdc++fs
endif
ifeq ($(UNAME_S),Darwin)
    # macOS uses libc++
    CXXFLAGS += -stdlib=libc++
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