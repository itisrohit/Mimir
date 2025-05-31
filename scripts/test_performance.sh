#!/bin/bash
set -e

echo "⚡ Running performance tests..."

# Build optimized version
make clean
CXXFLAGS="-std=c++17 -O2 -DNDEBUG" make

echo "📊 Testing startup time..."
time (echo "quit" | ./mimir > /dev/null)

echo "📊 Testing session creation time..."
time (echo -e "init perf_test\nclose\nquit" | ./mimir > /dev/null)

echo "📊 Testing multiple operations..."
time (echo -e "init perf_test\nlist\ninfo\nclose\nlist\nquit" | ./mimir > /dev/null)

echo "✅ Performance tests completed!"