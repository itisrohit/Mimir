#!/bin/bash

# Mimir Embedding Pipeline Test
# Tests the complete pipeline: SentencePiece tokenizer + ONNX embedding model

set -e  # Exit on any error

echo "🧪 Testing Mimir Embedding Pipeline"
echo "=================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
MODEL_DIR="models/bge-m3-onnx/"
TOKENIZER_PATH="${MODEL_DIR}sentencepiece.bpe.model"
MODEL_PATH="${MODEL_DIR}model.onnx"
TEST_FILE="test_document.txt"
TEST_EXEC="test_embedding_pipeline"

echo -e "${BLUE}📁 Checking model files...${NC}"
if [ ! -f "$TOKENIZER_PATH" ]; then
    echo -e "${RED}❌ Tokenizer not found: $TOKENIZER_PATH${NC}"
    exit 1
fi

if [ ! -f "$MODEL_PATH" ]; then
    echo -e "${RED}❌ Model not found: $MODEL_PATH${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Model files found${NC}"

echo -e "${BLUE}🔨 Building test executable...${NC}"
make clean > /dev/null 2>&1

# Build test executable
g++ -std=c++17 -Wall -Wextra -O2 -I./include -I./src -I/opt/homebrew/include -I/opt/homebrew/Cellar/onnxruntime/1.22.1/include/onnxruntime -L/opt/homebrew/lib -Wl,-rpath,/opt/homebrew/lib -lonnxruntime -lsentencepiece -o "$TEST_EXEC" \
    scripts/test_embedding_pipeline.cpp src/embedding/OnnxEmbedder.cpp src/embedding/SentencePieceTokenizer.cpp

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Build successful${NC}"

echo -e "${BLUE}🧪 Running embedding pipeline test...${NC}"

# Run the test and capture output
OUTPUT=$(./"$TEST_EXEC" 2>&1)
EXIT_CODE=$?

# Clean up test executable
rm -f "$TEST_EXEC"

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✅ Test passed!${NC}"
    echo ""
    echo -e "${BLUE}📊 Test Results:${NC}"
    echo "$OUTPUT" | grep -E "(✅|❌|Testing|Generated|Performance)"
else
    echo -e "${RED}❌ Test failed with exit code $EXIT_CODE${NC}"
    echo ""
    echo -e "${YELLOW}📋 Full output:${NC}"
    echo "$OUTPUT"
    exit 1
fi

echo ""
echo -e "${GREEN}🎉 All tests completed successfully!${NC}"
echo -e "${BLUE}📈 Pipeline is ready for production use.${NC}" 