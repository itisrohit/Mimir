# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I./src -I/opt/homebrew/include -I/opt/homebrew/Cellar/onnxruntime/1.22.1/include/onnxruntime
LDFLAGS = -L/opt/homebrew/lib

# Directories
SRCDIR = src
BUILDDIR = dist
OBJDIR = $(BUILDDIR)/obj

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.cpp) \
          $(wildcard $(SRCDIR)/config/*.cpp) \
          $(wildcard $(SRCDIR)/document_processor/*.cpp) \
          $(wildcard $(SRCDIR)/embedding/*.cpp) \
          $(wildcard $(SRCDIR)/session/*.cpp)

# Object files (in build directory)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Target executable (in build directory)
TARGET = $(BUILDDIR)/mimir

# Default target
all: $(TARGET)

# Create build directories
$(OBJDIR):
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/config
	mkdir -p $(OBJDIR)/document_processor
	mkdir -p $(OBJDIR)/embedding
	mkdir -p $(OBJDIR)/session

# Build the target executable
$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CXX) $(OBJECTS) $(LDFLAGS) -Wl,-rpath,/opt/homebrew/lib -lonnxruntime -lsentencepiece -o $(TARGET)
	@echo "✅ Build complete! Executable: $(TARGET)"

# Compile source files to object files in build directory
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run target (executes the built binary)
run: $(TARGET)
	@echo "🚀 Running Mimir..."
	$(TARGET)

# Clean target (removes build directory)
clean:
	rm -rf $(BUILDDIR)
	rm -rf ./.data
	rm -f test_*.txt
	rm -f *.log
	@echo "🧹 Clean complete!"

# Deep clean target (removes everything including external dependencies)
distclean: clean
	rm -rf external/
	rm -rf venv/
	rm -rf models/
	rm -f *.tmp
	rm -f *.cache
	@echo "🧹 Deep clean complete!"

# Test target (uses built binary)
test: $(TARGET)
	@echo "🧪 Running tests..."
	./scripts/test_full_pipeline.sh

# Install target (creates symlink in current directory)
install: $(TARGET)
	@echo "📦 Installing Mimir..."
	ln -sf $(TARGET) ./mimir
	@echo "✅ Mimir installed! Run with: ./mimir"

# Uninstall target (removes symlink)
uninstall:
	rm -f ./mimir
	@echo "🗑️ Mimir uninstalled!"

# Show build info
info:
	@echo "📋 Build Information:"
	@echo "  Source directory: $(SRCDIR)"
	@echo "  Build directory: $(BUILDDIR)"
	@echo "  Object directory: $(OBJDIR)"
	@echo "  Target executable: $(TARGET)"
	@echo "  Source files: $(words $(SOURCES))"
	@echo "  Object files: $(words $(OBJECTS))"

.PHONY: all clean distclean test run install uninstall info