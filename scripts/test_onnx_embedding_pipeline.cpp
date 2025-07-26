// test_onnx_embedding_pipeline.cpp
// Usage:
//   g++ -std=c++17 -I./src -I<onnxruntime_include> -I<tokenizers_include> \
//       -L<onnxruntime_lib> -L<tokenizers_lib> \
//       -lonnxruntime -ltokenizers -o test_onnx_embedding_pipeline \
//       scripts/test_onnx_embedding_pipeline.cpp src/embedding/OnnxEmbedder.cpp
//   ./test_onnx_embedding_pipeline
//
// Make sure to set the correct include/library paths for ONNX Runtime and Tokenizers C++
//
// This script tests the ONNX embedding pipeline end-to-end.

#include "../src/embedding/OnnxEmbedder.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    // Paths to your model and tokenizer
    std::string model_path = "models/onnx/model.onnx";
    std::string tokenizer_path = "models";
    int max_length = 2048;
    int num_threads = 4;

    try {
        OnnxEmbedder embedder(model_path, tokenizer_path, max_length, num_threads);
        std::vector<std::string> texts = {"Hello, world! This is a test."};
        auto embeddings = embedder.get_embeddings(texts);
        if (embeddings.empty()) {
            std::cerr << "No embeddings returned!" << std::endl;
            return 1;
        }
        std::cout << "Test embedding shape: (" << embeddings.size() << ", " << embeddings[0].size() << ")\n";
        std::cout << "Test embedding (first 8 values): ";
        for (size_t i = 0; i < std::min<size_t>(8, embeddings[0].size()); ++i) {
            std::cout << embeddings[0][i] << " ";
        }
        std::cout << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
} 