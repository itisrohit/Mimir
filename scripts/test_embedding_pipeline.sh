#!/bin/bash
set -e

# Test file and session names
TEST_DOC="test_embed_doc.txt"
SESSION_NAME="embed_test_session"

# 1. Create a larger test document
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
echo -e "init $SESSION_NAME\nadd-doc $TEST_DOC\ninfo\nclose\nquit" | ./mimir
END_TOTAL=$(date +%s)

# 3. Find the latest session directory
SESSION_DIR=$(find .data/sessions -type d -name "${SESSION_NAME}_*" | sort | tail -1)
if [ -z "$SESSION_DIR" ]; then
  echo "❌ Session directory not found!"
  exit 1
fi

# 4. Check for doc_chunks.json and metadata.json
if [ ! -f "$SESSION_DIR/doc_chunks.json" ]; then
  echo "❌ doc_chunks.json not found in $SESSION_DIR"
  exit 1
fi
if [ ! -f "$SESSION_DIR/metadata.json" ]; then
  echo "❌ metadata.json not found in $SESSION_DIR"
  exit 1
fi

# 5. Parse doc_chunks.json to verify embeddings
EMBEDDING_COUNT=$(jq '[.chunks[] | select(.embedding != null and (.embedding | length) > 0)] | length' "$SESSION_DIR/doc_chunks.json")
CHUNK_COUNT=$(jq '.chunks | length' "$SESSION_DIR/doc_chunks.json")
if [ "$EMBEDDING_COUNT" -eq "$CHUNK_COUNT" ]; then
  echo "✅ All $CHUNK_COUNT chunks have non-empty embeddings."
else
  echo "❌ Only $EMBEDDING_COUNT of $CHUNK_COUNT chunks have embeddings!"
  exit 1
fi

# 6. Print summary and timing benchmarks
echo "[PASS] Embedding pipeline integration test succeeded."
echo "--- Benchmark Results ---"
echo "Total pipeline: $((END_TOTAL - START_TOTAL)) s"

# Cleanup test doc (optional)
# rm -f "$TEST_DOC" 