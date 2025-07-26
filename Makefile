# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I./include -I./src -I/opt/homebrew/include -I/opt/homebrew/Cellar/onnxruntime/1.22.1/include/onnxruntime
LDFLAGS = -L/opt/homebrew/lib

# Directories
SRCDIR = src
OBJDIR = obj

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.cpp) \
          $(wildcard $(SRCDIR)/config/*.cpp) \
          $(wildcard $(SRCDIR)/document_processor/*.cpp) \
          $(wildcard $(SRCDIR)/embedding/*.cpp) \
          $(wildcard $(SRCDIR)/session/*.cpp)

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
TARGET = mimir

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -Wl,-rpath,/opt/homebrew/lib -lonnxruntime -lsentencepiece -o $(TARGET)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJECTS) $(TARGET) *.dSYM

# Test target
test:
	./scripts/test_embedding_pipeline.sh

.PHONY: all clean test