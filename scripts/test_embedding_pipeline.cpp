#include "../src/embedding/OnnxEmbedder.h"
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

int main() {
    string modelDir = "models/bge-m3-onnx/";
    string tokenizerPath = modelDir + "sentencepiece.bpe.model";
    string modelPath = modelDir + "model.onnx";
    
    try {
        cout << "🧪 Testing Mimir Embedding Pipeline" << endl;
        cout << "==================================" << endl;
        
        // Measure initialization time
        auto start = high_resolution_clock::now();
        OnnxEmbedder embedder(tokenizerPath, modelPath);
        auto end = high_resolution_clock::now();
        auto initTime = duration_cast<milliseconds>(end - start);
        
        cout << "✅ Initialization time: " << initTime.count() << "ms" << endl;
        
        // Test with batch size 4
        vector<string> texts = {
            "Hello world!",
            "How are you today?",
            "This is a test sentence.",
            "Another example text."
        };
        
        cout << "Testing batch size " << texts.size() << "..." << endl;
        
        // Measure embedding time
        start = high_resolution_clock::now();
        auto embeddings = embedder.embed(texts);
        end = high_resolution_clock::now();
        auto embedTime = duration_cast<milliseconds>(end - start);
        
        if (embeddings.empty()) {
            cerr << "❌ No embeddings generated" << endl;
            return 1;
        }
        
        cout << "✅ Generated " << embeddings.size() << " embeddings" << endl;
        cout << "✅ Embedding time: " << embedTime.count() << "ms" << endl;
        cout << "✅ Average time per text: " << embedTime.count() / texts.size() << "ms" << endl;
        
        // Performance metrics
        double throughput = (double)texts.size() / (embedTime.count() / 1000.0);
        cout << "✅ Throughput: " << throughput << " texts/second" << endl;
        
        return 0;
        
    } catch (const exception& e) {
        cerr << "❌ Error: " << e.what() << endl;
        return 1;
    }
} 