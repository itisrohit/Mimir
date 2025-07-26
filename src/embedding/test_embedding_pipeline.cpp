#include "OnnxEmbedder.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    string modelDir = "models/bge-m3-onnx/";
    string tokenizerPath = modelDir + "tokenizer.onnx";
    string modelPath = modelDir + "model.onnx";
    string ortExtensionsPath = "/opt/homebrew/lib/libonnxruntime_extensions.dylib";

    vector<string> texts4 = {
        "The quick brown fox jumps over the lazy dog.",
        "ONNX Runtime C++ embedding pipeline test.",
        "Short.",
        "Another test sentence."
    };
    vector<string> texts1 = {"The quick brown fox jumps over the lazy dog."};

    try {
        OnnxEmbedder embedder(tokenizerPath, modelPath, ortExtensionsPath);
        cout << "\n==== Batch size 4 ====" << endl;
        vector<vector<float>> embeddings4 = embedder.embed(texts4);
        cout << "Embeddings generated: " << embeddings4.size() << endl;
        for (size_t i = 0; i < embeddings4.size(); ++i) {
            cout << "Embedding " << i << " (first 5 values): ";
            for (size_t j = 0; j < 5 && j < embeddings4[i].size(); ++j) {
                cout << embeddings4[i][j] << " ";
            }
            cout << endl;
        }
        cout << "\n==== Batch size 1 ====" << endl;
        vector<vector<float>> embeddings1 = embedder.embed(texts1);
        cout << "Embeddings generated: " << embeddings1.size() << endl;
        for (size_t i = 0; i < embeddings1.size(); ++i) {
            cout << "Embedding " << i << " (first 5 values): ";
            for (size_t j = 0; j < 5 && j < embeddings1[i].size(); ++j) {
                cout << embeddings1[i][j] << " ";
            }
            cout << endl;
        }
        cout << "[PASS] C++ ONNX embedding pipeline test succeeded." << endl;
        return 0;
    } catch (const std::exception& ex) {
        cerr << "[FAIL] Exception: " << ex.what() << endl;
        return 1;
    }
} 