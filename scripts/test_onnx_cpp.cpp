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
    
    // Test single embedding
    try {
        EmbeddingResult result = embedding_manager.generateEmbedding(test_texts[0]);
        
        cout << "✅ Single embedding generated successfully!" << endl;
        cout << "   - Sequence length: " << result.sequence_length << endl;
        cout << "   - Dense embedding size: " << result.dense_embedding.size() << endl;
        cout << "   - Sparse weights size: " << result.sparse_weights.size() << endl;
        cout << "   - ColBERT embeddings: " << result.colbert_embeddings.size() << " x " 
             << (result.colbert_embeddings.empty() ? 0 : result.colbert_embeddings[0].size()) << endl;
        
        // Print first few values of dense embedding
        cout << "   - Dense embedding (first 5 values): ";
        for (int i = 0; i < min(5, (int)result.dense_embedding.size()); i++) {
            cout << result.dense_embedding[i];
            if (i < 4) cout << ", ";
        }
        cout << endl;
        
    } catch (const exception& e) {
        cerr << "❌ Error generating single embedding: " << e.what() << endl;
        return 1;
    }
    
    cout << "\n🔍 Testing batch embedding..." << endl;
    
    // Test batch embedding
    try {
        vector<EmbeddingResult> results = embedding_manager.generateEmbeddings(test_texts);
        
        cout << "✅ Batch embedding generated successfully!" << endl;
        cout << "   - Number of results: " << results.size() << endl;
        
        for (size_t i = 0; i < results.size(); i++) {
            cout << "   - Result " << i << ": " << results[i].dense_embedding.size() << " dimensions" << endl;
        }
        
    } catch (const exception& e) {
        cerr << "❌ Error generating batch embeddings: " << e.what() << endl;
        return 1;
    }
    
    cout << "\n✅ All tests passed! BGE-M3 ONNX C++ integration is working correctly." << endl;
    
    return 0;
} 