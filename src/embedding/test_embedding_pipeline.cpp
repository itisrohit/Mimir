#include "OnnxEmbedder.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    string modelDir = "models/bge-m3-onnx/";
    string tokenizerPath = modelDir + "sentencepiece.bpe.model";
    string modelPath = modelDir + "model.onnx";
    
    try {
        OnnxEmbedder embedder(tokenizerPath, modelPath);
        
        // Test with batch size 4
        vector<string> texts = {
            "Hello world!",
            "How are you today?",
            "This is a test sentence.",
            "Another example text."
        };
        
        cout << "Testing batch size " << texts.size() << "..." << endl;
        auto embeddings = embedder.embed(texts);
        
        if (embeddings.empty()) {
            cerr << "❌ No embeddings generated" << endl;
            return 1;
        }
        
        cout << "✅ Generated " << embeddings.size() << " embeddings" << endl;
        cout << "📊 Embedding dimension: " << embeddings[0].size() << endl;
        
        // Test with batch size 1
        vector<string> singleText = {"Single text test."};
        cout << "\nTesting batch size 1..." << endl;
        auto singleEmbedding = embedder.embed(singleText);
        
        if (singleEmbedding.empty()) {
            cerr << "❌ No single embedding generated" << endl;
            return 1;
        }
        
        cout << "✅ Generated single embedding successfully" << endl;
        cout << "📊 Single embedding dimension: " << singleEmbedding[0].size() << endl;
        
        cout << "\n🎉 C++ embedding pipeline test PASSED!" << endl;
        return 0;
        
    } catch (const exception& e) {
        cerr << "❌ Exception: " << e.what() << endl;
        return 1;
    }
} 