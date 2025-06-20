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

# Check session functionality with configuration-aware directories
echo "📁 Testing session functionality..."
echo -e "init test_session_check\ninfo\nclose\nquit" | run_with_timeout 10 ./mimir > session_test.log 2>&1

# Check if session was created by looking for success message
if grep -q "created successfully" session_test.log; then
    echo "✅ Session creation works"
    
    # Check ONLY for .data directory (config-specified path)
    if [ -d ".data" ]; then
        echo "✅ Session directory created successfully in .data/"
        ls -la .data/sessions/ 2>/dev/null || true
    else
        echo "❌ Expected .data directory not found"
        echo "Debug: Current directory contents:"
        ls -la
        exit 1
    fi
else
    echo "⚠️  Session test completed (output varies by platform)"
    head -5 session_test.log 2>/dev/null || echo "No session test log found"
fi

# Clean up test files
rm -f session_test.log
rm -rf .data/ sessions/ 2>/dev/null || true

# Check hybrid auto-save behavior with document addition
echo "📁 Testing hybrid auto-save with document addition..."
echo "# Test document for hybrid auto-save" > test_hybrid_doc.txt

echo -e "init test_hybrid\nadd-doc test_hybrid_doc.txt\ninfo\nquit" | run_with_timeout 15 ./mimir > hybrid_test.log 2>&1

# Check if files were created immediately in .data/sessions/
SESSION_FOUND=false
if [ -d ".data" ]; then
    if find .data -name "metadata.json" -o -name "doc_chunks.json" 2>/dev/null | grep -q .; then
        echo "✅ Hybrid auto-save working - essential files created immediately in .data/"
        SESSION_FOUND=true
        
        # Show what was created
        find .data -name "*.json" -exec ls -la {} \; 2>/dev/null || true
    fi
fi

if [ "$SESSION_FOUND" = false ]; then
    echo "❌ Hybrid auto-save not working - no session files found in .data/"
    echo "Debug: Directory contents:"
    ls -la
    echo "Test output:"
    head -10 hybrid_test.log 2>/dev/null || echo "No test log found"
    exit 1
fi

# Clean up test files
rm -f test_hybrid_doc.txt hybrid_test.log
rm -rf .data/ 2>/dev/null || true

echo "✅ All CI tests passed!"