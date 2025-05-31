#!/bin/bash
set -e

echo "⚡ Running performance tests..."

# Create multiple test files
for i in {1..10}; do
  echo "Test document $i content - Lorem ipsum dolor sit amet" > test_doc_$i.txt
done

# Time the session operations
time cat << 'EOF' | timeout 30s ./mimir || true
init performance_test
add-doc test_doc_1.txt
add-doc test_doc_2.txt
add-doc test_doc_3.txt
add-doc test_doc_4.txt
add-doc test_doc_5.txt
info
export performance_test
close
quit
EOF

# Cleanup
rm -f test_doc_*.txt

echo "✅ Performance test completed!"