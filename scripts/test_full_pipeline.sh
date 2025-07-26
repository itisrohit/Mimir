#!/bin/bash
set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

TEST_DOC="test_long_document.txt"
SESSION_NAME="test_session_$(date +%s)"

# Try to find Mimir executable in order of preference
if [ -f "./dist/mimir" ]; then
    MIMIR_EXEC="./dist/mimir"
elif [ -f "./mimir" ]; then
    MIMIR_EXEC="./mimir"
else
    echo -e "${RED}❌ Mimir executable not found!${NC}"
    echo -e "${YELLOW}💡 Try running: make install${NC}"
    exit 1
fi

echo -e "${BLUE}🔧 Using Mimir executable: $MIMIR_EXEC${NC}"

# 1. Create a long .txt document
echo -e "${BLUE}📝 Creating a long test document...${NC}"
cat /dev/null > "$TEST_DOC"
for i in {1..1000}; do
  echo "This is line $i of the test document. The quick brown fox jumps over the lazy dog." >> "$TEST_DOC"
done

# 2. Initialize Mimir (if needed)
echo -e "${BLUE}🚀 Initializing Mimir...${NC}"
if [ ! -f "$MIMIR_EXEC" ]; then
  echo -e "${RED}❌ Mimir executable not found!${NC}"
  exit 1
fi

# 3. Add the document (automate CLI)
echo -e "${BLUE}📄 Adding document to Mimir via CLI...${NC}"
echo -e "${BLUE}   Using session: $SESSION_NAME${NC}"
$MIMIR_EXEC 2>/dev/null <<EOF
init $SESSION_NAME
add-doc $TEST_DOC
exit
EOF

# 4. Check that the document was processed (look for output, logs, or session data)
echo -e "${BLUE}🔍 Checking if document was processed...${NC}"
if [ -d "./.data/sessions/${SESSION_NAME}_"* ] && [ -f "./.data/sessions/${SESSION_NAME}_"*/doc_chunks.json ]; then
    echo -e "${GREEN}✅ Document successfully processed and added to session!${NC}"
    echo -e "${GREEN}   Session data found in: ./.data/sessions/${SESSION_NAME}_*${NC}"
else
    echo -e "${RED}❌ Document not found in session data!${NC}"
    exit 1
fi

# 5. Clean up
echo -e "${BLUE}🧹 Cleaning up test files...${NC}"
rm -f "$TEST_DOC"

# Optionally, remove session data (uncomment if desired)
# rm -rf ./.data/sessions/${SESSION_NAME}*

echo -e "${GREEN}🎉 Full pipeline test completed successfully!${NC}" 