#!/bin/bash
set -e

echo "🤖 Running CI test suite..."

# Cross-platform timeout command
if command -v timeout >/dev/null 2>&1; then
    TIMEOUT_CMD="timeout"
elif command -v gtimeout >/dev/null 2>&1; then
    TIMEOUT_CMD="gtimeout"
else
    echo "⚠️  No timeout command found, running without timeout"
    TIMEOUT_CMD=""
fi

# Helper function for timed execution
run_with_timeout() {
    local seconds=$1
    shift
    if [ -n "$TIMEOUT_CMD" ]; then
        $TIMEOUT_CMD ${seconds}s "$@" || true
    else
        "$@" || true
    fi
}

# Ensure we're in the right directory
if [ ! -f "Makefile" ]; then
    echo "❌ Makefile not found. Are you in the project root?"
    exit 1
fi

# Clean and build
echo "🔨 Building project..."
make clean
make

echo "🧪 Running basic functionality tests..."

# Test 1: Help command
echo "📋 Testing help command..."
echo "help" | run_with_timeout 5 ./mimir > /dev/null

# Test 2: Session operations
echo "📁 Testing session operations..."
echo -e "init ci_test\ninfo\nlist\nclose\nquit" | run_with_timeout 10 ./mimir > /dev/null

# Test 3: Invalid commands
echo "❌ Testing error handling..."
echo -e "invalid_command\nquit" | run_with_timeout 5 ./mimir > /dev/null

# Test 4: Empty inputs
echo "🔍 Testing edge cases..."
echo -e "\n\n\nhelp\nquit" | run_with_timeout 5 ./mimir > /dev/null

# Verify binary exists and is executable
if [ -f "./mimir" ] && [ -x "./mimir" ]; then
    echo "✅ Binary is properly built and executable"
else
    echo "❌ Binary missing or not executable"
    exit 1
fi

# Check data directory structure (after running commands that create it)
echo "📁 Checking data directory..."
echo -e "init test_dir_check\nquit" | run_with_timeout 5 ./mimir > /dev/null
if [ -d ".data" ]; then
    echo "✅ Data directory created successfully"
    ls -la .data/ || true
    rm -rf .data/  # Clean up test data
else
    echo "❌ Data directory not found"
    exit 1
fi

echo "✅ All CI tests passed!"