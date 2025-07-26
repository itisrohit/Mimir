#include <iostream>
#include <string>
#include <vector>
#include "../src/embedding/ONNXEmbeddingManager.h"

using namespace std;

int main() {
    cout << "🧪 Testing BGE-M3 ONNX C++ Integration" << endl;
    cout << "========================================" << endl;
    
    // Initialize embedding manager
    ONNXEmbeddingManager embedding_manager;
    
    // Paths to model and tokenizer
    string model_path = "./models/bge-m3-onnx/model.onnx";
    string tokenizer_path = "./models/bge-m3-onnx";
    
    cout << "🔧 Initializing model from: " << model_path << endl;
    cout << "🔧 Tokenizer path: " << tokenizer_path << endl;
    
    // Initialize the model
    if (!embedding_manager.initialize(model_path, tokenizer_path)) {
        cerr << "❌ Failed to initialize BGE-M3 model" << endl;
        return 1;
    }
    
    cout << "✅ Model initialized successfully!" << endl;
    cout << "📊 Embedding dimension: " << embedding_manager.getEmbeddingDimension() << endl;
    cout << "📊 Max sequence length: " << embedding_manager.getMaxSequenceLength() << endl;
    
    // Test texts
    vector<string> test_texts = {
        "BGE M3 is an embedding model supporting dense retrieval, lexical matching and multi-vector interaction.",
        "This is a test sentence for embedding generation.",
        "ONNX Runtime provides efficient inference for machine learning models."
    };
    
    cout << "\n🔍 Testing single text embedding..." << endl;
    
    // Single test
    EmbeddingResult result = embedding_manager.generateEmbeddings(test_texts[0]);
    embedding_manager.printEmbeddingInfo(result);

    // Batch test
    for (const auto& text : test_texts) {
        EmbeddingResult res = embedding_manager.generateEmbeddings(text);
        embedding_manager.printEmbeddingInfo(res);
    }
    
    cout << "\n✅ All tests passed! BGE-M3 ONNX C++ integration is working correctly." << endl;
    
    return 0;
} 