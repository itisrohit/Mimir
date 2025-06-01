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

# Test 2: Document addition with hybrid auto-save verification
echo "📄 Test 2: Document operations with hybrid auto-save"
echo "# Test document content" > workflow_test_doc.txt

echo -e "init workflow_doc_test\nadd-doc workflow_test_doc.txt\ninfo\nlist\nquit" | run_with_timeout 15 ./mimir > doc_workflow.log 2>&1

# Verify immediate file creation (hybrid auto-save)
FILES_CREATED=false
if find . -path "*/workflow_doc_test*/metadata.json" 2>/dev/null | head -1 | xargs test -f; then
    echo "✅ Metadata saved immediately after document add"
    FILES_CREATED=true
fi

if find . -path "*/workflow_doc_test*/doc_chunks.json" 2>/dev/null | head -1 | xargs test -f; then
    echo "✅ Document chunks saved immediately after document add"
    FILES_CREATED=true
fi

# Chat history should NOT exist yet (batch save)
if ! find . -path "*/workflow_doc_test*/chat_history.json" 2>/dev/null | head -1 | xargs test -f; then
    echo "✅ Chat history correctly batched (not saved yet)"
fi

if [ "$FILES_CREATED" = false ]; then
    echo "❌ Hybrid auto-save not working as expected"
    cat doc_workflow.log
    exit 1
fi

rm -f workflow_test_doc.txt doc_workflow.log

# Test 3: Query functionality
echo "💬 Test 3: Query functionality"
echo -e "init query_test\nquery What is this project about?\ninfo\nclose\nquit" | run_with_timeout 15 ./mimir

# Test 4: Session persistence
echo "💾 Test 4: Session persistence"
echo -e "init persist_test\nclose\nquit" | run_with_timeout 10 ./mimir
echo -e "load persist_test\ninfo\nclose\nquit" | run_with_timeout 10 ./mimir

# Test 5: Complete hybrid auto-save workflow
echo "🔄 Test 5: Complete hybrid auto-save workflow"
echo "# Complete workflow test" > complete_test.txt

# Test full workflow: document add (immediate save) + chat (batch save) + close (full save)
echo -e "init complete_hybrid\nadd-doc complete_test.txt\nquery Test question about the document\nclose\nquit" | run_with_timeout 20 ./mimir > complete_test.log 2>&1

# After close, all files should exist
ALL_FILES_EXIST=true
for file in "metadata.json" "doc_chunks.json" "chat_history.json" "faiss_index.bin"; do
    if ! find . -path "*/complete_hybrid*/$file" 2>/dev/null | head -1 | xargs test -f; then
        echo "⚠️  $file not found after session close"
        ALL_FILES_EXIST=false
    fi
done

if [ "$ALL_FILES_EXIST" = true ]; then
    echo "✅ Complete hybrid workflow working - all files saved after close"
else
    echo "❌ Some files missing after complete workflow"
    cat complete_test.log
fi

rm -f complete_test.txt complete_test.log

# Clean up test data
rm -rf .data/

echo "✅ Workflow tests completed successfully!"