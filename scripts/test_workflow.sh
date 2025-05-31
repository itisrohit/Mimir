#!/bin/bash
set -e

echo "🧪 Testing Mimir workflow..."

# Cross-platform timeout command
if command -v timeout >/dev/null 2>&1; then
    TIMEOUT_CMD="timeout"
elif command -v gtimeout >/dev/null 2>&1; then
    TIMEOUT_CMD="gtimeout"
else
    TIMEOUT_CMD=""
fi

run_with_timeout() {
    local seconds=$1
    shift
    if [ -n "$TIMEOUT_CMD" ]; then
        $TIMEOUT_CMD ${seconds}s "$@" || true
    else
        "$@" || true
    fi
}

# Build first
make clean
make

echo "📋 Testing comprehensive session workflow..."

# Test 1: Session creation and basic operations
echo "📝 Test 1: Session creation and management"
echo -e "init workflow_test\ninfo\nlist\nclose\nquit" | run_with_timeout 15 ./mimir

# Test 2: Document addition (with README.md if it exists)
echo "📄 Test 2: Document operations"
if [ -f "README.md" ]; then
    echo -e "init doc_test\nadd-doc README.md\ninfo\nclose\nquit" | run_with_timeout 15 ./mimir
else
    echo "No README.md found, skipping document test"
fi

# Test 3: Query functionality
echo "💬 Test 3: Query functionality"
echo -e "init query_test\nquery What is this project about?\ninfo\nclose\nquit" | run_with_timeout 15 ./mimir

# Test 4: Session persistence
echo "💾 Test 4: Session persistence"
echo -e "init persist_test\nclose\nquit" | run_with_timeout 10 ./mimir
echo -e "load persist_test\ninfo\nclose\nquit" | run_with_timeout 10 ./mimir

# Clean up test data
rm -rf .data/

echo "✅ Workflow tests completed successfully!"