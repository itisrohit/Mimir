#!/bin/bash
set -e

echo "🧪 Running CI tests locally..."

# Build
./scripts/setup_env.sh
./scripts/build.sh

# Create test document
echo "Test document content" > test_doc.txt

# Run comprehensive test
echo "Testing session workflow..."
cat << 'EOF' | timeout 15s ./mimir
init ci_test
add-doc test_doc.txt
info
close
load ci_test
info
delete ci_test
y
quit
EOF

# Cleanup
rm -f test_doc.txt

echo "✅ All CI tests passed locally!"