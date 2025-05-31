CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -stdlib=libc++
TARGET = mimir
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/session/SessionManager.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

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