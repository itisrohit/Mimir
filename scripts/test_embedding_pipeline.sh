#!/bin/bash
set -e

# Embedding Pipeline Integration Test (ONNX version)
# This script tests the full Mimir pipeline using the ONNX embedding backend.
# It creates a large test document, runs the CLI, and verifies all chunks have embeddings.
# Timestamps are printed for each major step.

log_ts() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Print embedding backend from config.yaml (if available)
if grep -q 'model_type:' config.yaml; then
  EMBED_BACKEND=$(grep 'model_type:' config.yaml | awk '{print $2}')
  log_ts "Embedding backend: $EMBED_BACKEND"
fi

# Test file and session names
TEST_DOC="test_embed_doc.txt"
SESSION_NAME="embed_test_session"

# 1. Create a larger test document
log_ts "Creating test document..."
cat > "$TEST_DOC" <<EOF
The quick brown fox jumps over the lazy dog.
A fast brown fox leaps over a sleepy dog.
Quantum mechanics is a fundamental theory in physics.
Artificial intelligence is transforming the world.
Deep learning enables powerful models for vision and language.
The mitochondria is the powerhouse of the cell.
To be or not to be, that is the question.
In the beginning, the universe was created. This has made a lot of people very angry and been widely regarded as a bad move.
The rain in Spain stays mainly in the plain.
All work and no play makes Jack a dull boy.
Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.
Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.
$(for i in {1..100}; do echo "This is line $i of the large test document. The quick brown fox jumps over the lazy dog."; done)
EOF

# 2. Run Mimir CLI to create session, add doc, and close in a single process, timing the whole pipeline
START_TOTAL=$(date +%s)
log_ts "Running Mimir CLI pipeline..."
START_CLI=$(date +%s)
echo -e "init $SESSION_NAME\nadd-doc $TEST_DOC\ninfo\nclose\nquit" | ./mimir
END_CLI=$(date +%s)
END_TOTAL=$(date +%s)

# 3. Find the latest session directory
log_ts "Locating session directory..."
START_FIND=$(date +%s)
SESSION_DIR=$(find .data/sessions -type d -name "${SESSION_NAME}_*" | sort | tail -1)
END_FIND=$(date +%s)
if [ -z "$SESSION_DIR" ]; then
  log_ts "❌ Session directory not found!"
  exit 1
fi

# 4. Check for doc_chunks.json and metadata.json
log_ts "Checking for output files..."
START_CHECK=$(date +%s)
if [ ! -f "$SESSION_DIR/doc_chunks.json" ]; then
  log_ts "❌ doc_chunks.json not found in $SESSION_DIR"
  exit 1
fi
if [ ! -f "$SESSION_DIR/metadata.json" ]; then
  log_ts "❌ metadata.json not found in $SESSION_DIR"
  exit 1
fi
END_CHECK=$(date +%s)

# 5. Parse doc_chunks.json to verify embeddings
log_ts "Verifying embeddings in doc_chunks.json..."
START_EMBED=$(date +%s)
EMBEDDING_COUNT=$(jq '[.chunks[] | select(.embedding != null and (.embedding | length) > 0)] | length' "$SESSION_DIR/doc_chunks.json")
CHUNK_COUNT=$(jq '.chunks | length' "$SESSION_DIR/doc_chunks.json")
END_EMBED=$(date +%s)
if [ "$EMBEDDING_COUNT" -eq "$CHUNK_COUNT" ]; then
  log_ts "✅ All $CHUNK_COUNT chunks have non-empty embeddings."
else
  log_ts "❌ Only $EMBEDDING_COUNT of $CHUNK_COUNT chunks have embeddings!"
  exit 1
fi

# 6. Print summary and timing benchmarks
log_ts "[PASS] Embedding pipeline integration test succeeded."
echo "--- Benchmark Results ---"
echo "Total pipeline: $((END_TOTAL - START_TOTAL)) s"
echo "  Mimir CLI: $((END_CLI - START_CLI)) s"
echo "  Session search: $((END_FIND - START_FIND)) s"
echo "  Output file check: $((END_CHECK - START_CHECK)) s"
echo "  Embedding verification: $((END_EMBED - START_EMBED)) s"

# Cleanup test doc (optional)
# rm -f "$TEST_DOC" 