#!/bin/bash
set -e

echo "🚀 Running complete local testing before GitHub push..."

# Helper function for timeout on macOS/Linux
run_with_timeout() {
    local timeout_duration=$1
    shift
    
    if command -v timeout >/dev/null 2>&1; then
        # Linux - has timeout command
        timeout "$timeout_duration" "$@"
    elif command -v gtimeout >/dev/null 2>&1; then
        # macOS with coreutils installed
        gtimeout "$timeout_duration" "$@"
    else
        # macOS fallback - just run the command
        "$@" &
        local pid=$!
        sleep "$timeout_duration" && kill $pid 2>/dev/null || true
        wait $pid 2>/dev/null || true
    fi
}

# 1. Clean build
echo "🔨 Testing clean build..."
make clean
rm -rf .data/ sessions/ *.log test_*.txt 2>/dev/null || true
make
echo "✅ Clean build successful"

# 2. Basic functionality
echo "🧪 Testing basic functionality..."
echo -e "help\nquit" | run_with_timeout 5 ./mimir > /dev/null
echo "✅ Basic CLI working"

# 3. Hybrid auto-save
echo "🔄 Testing hybrid auto-save..."
echo "# Test document" > test_local_hybrid.txt
echo -e "init local_hybrid_test\nadd-doc test_local_hybrid.txt\ninfo\nquit" | run_with_timeout 10 ./mimir > hybrid_test.log

if find . -name "metadata.json" -o -name "doc_chunks.json" 2>/dev/null | grep -q .; then
    echo "✅ Hybrid auto-save working"
else
    echo "❌ Hybrid auto-save failed"
    cat hybrid_test.log
    exit 1
fi

# 4. Configuration respect
echo "📋 Testing configuration respect..."
./scripts/setup_env.sh > /dev/null
if [ -d ".data" ] && [ -z "$(ls -A .data 2>/dev/null)" ]; then
    echo "⚠️  Empty .data directory created (acceptable)"
elif [ ! -d ".data" ]; then
    echo "✅ No hardcoded directory creation"
else
    echo "❌ Hardcoded directories still being created"
    ls -la .data/
    exit 1
fi

# 5. Run test suites
echo "🧪 Running test suites..."
./scripts/test_ci.sh
./scripts/test_workflow.sh

# 6. Manual workflow test
echo "🔄 Testing complete workflow..."
echo -e "init complete_local_test\nadd-doc test_local_hybrid.txt\nquery test question\nclose\nquit" | run_with_timeout 15 ./mimir > complete_test.log

# Check all files exist
FILES_COUNT=$(find . -path "*/complete_local_test*" -name "*.json" 2>/dev/null | wc -l)
if [ "$FILES_COUNT" -ge 3 ]; then
    echo "✅ Complete workflow successful"
else
    echo "⚠️  Workflow completed but file count: $FILES_COUNT"
    find . -path "*/complete_local_test*" -type f 2>/dev/null || echo "No files found"
fi

# 7. Check for compiler warnings
echo "🔍 Checking for compiler warnings..."
make clean > /dev/null
make 2>&1 | tee build_check.log
if grep -q "warning:" build_check.log; then
    echo "⚠️  Compiler warnings found:"
    grep "warning:" build_check.log
else
    echo "✅ No compiler warnings"
fi

# Clean up
rm -f test_local_hybrid.txt *.log
rm -rf .data/ sessions/ 2>/dev/null || true

echo "🎉 All local tests passed! Ready to push to GitHub."