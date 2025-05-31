#!/bin/bash
set -e

echo "🧪 Testing comprehensive workflow..."

# Create test files
echo "This is a test document for Mimir" > test_doc.txt
echo "# Test Markdown" > test.md

# Run workflow test
cat << 'EOF' | timeout 20s ./mimir || true
init workflow_test
add-doc test_doc.txt
add-doc test.md
query What is this document about?
info
list
export workflow_test
close
load workflow_test
info
close
delete workflow_test
y
list
quit
EOF

# Cleanup
rm -f test_doc.txt test.md

echo "✅ Workflow test completed!"