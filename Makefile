CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
TARGET = mimir
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/session/SessionManager.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Platform-specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Linux uses libstdc++ by default, may need filesystem library
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

# Clean up build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -rf .data/   

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -DDEBUG
debug: $(TARGET)

.PHONY: all clean run debug