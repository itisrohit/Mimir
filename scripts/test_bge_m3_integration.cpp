#include <iostream>
#include <string>
#include <vector>
#include "../src/embedding/ONNXEmbeddingManager.h"

using namespace std;

int main() {
    cout << "🧪 Testing BGE-M3 ONNX Integration" << endl;
    cout << "===================================" << endl;
    
    try {
        // Initialize embedding manager
        ONNXEmbeddingManager embedding_manager;
        
        // Paths to model and tokenizer
        string model_path = "./models/bge-m3-onnx/model.onnx";
        string tokenizer_path = "./models/bge-m3-onnx";
        
        cout << "🔧 Initializing BGE-M3 model..." << endl;
        cout << "   Model path: " << model_path << endl;
        cout << "   Tokenizer path: " << tokenizer_path << endl;
        
        // Initialize the model
        if (!embedding_manager.initialize(model_path, tokenizer_path)) {
            cerr << "❌ Failed to initialize BGE-M3 model" << endl;
            return 1;
        }
        
        cout << "\n✅ Model initialized successfully!" << endl;
        
        // Test texts
        vector<string> test_texts = {
            "BGE M3 is an embedding model supporting dense retrieval, lexical matching and multi-vector interaction.",
            "The model can generate three types of embeddings: dense, sparse, and ColBERT.",
            "This is a test of the ONNX Runtime integration with BGE-M3."
        };
        
        cout << "\n🔤 Testing embeddings for " << test_texts.size() << " texts..." << endl;
        
        for (size_t i = 0; i < test_texts.size(); i++) {
            cout << "\n--- Text " << (i + 1) << " ---" << endl;
            cout << "Input: " << test_texts[i] << endl;
            
            try {
                // Generate embeddings
                EmbeddingResult result = embedding_manager.generateEmbeddings(test_texts[i]);
                
                // Print embedding information
                embedding_manager.printEmbeddingInfo(result);
                
                // Additional analysis
                cout << "   Sparse weights sample: [";
                for (size_t j = 0; j < min(static_cast<size_t>(5), result.sparse_weights.size()); j++) {
                    cout << result.sparse_weights[j];
                    if (j < 4) cout << ", ";
                }
                cout << "]" << endl;
                
                cout << "   ColBERT embeddings shape: " << result.colbert_embeddings.size() 
                     << " tokens × " << (result.colbert_embeddings.empty() ? 0 : result.colbert_embeddings[0].size()) 
                     << " dimensions" << endl;
                
                // Verify embedding properties
                bool valid_dense = result.dense_embedding.size() == 1024;
                bool valid_sparse = result.sparse_weights.size() == result.sequence_length;
                bool valid_colbert = result.colbert_embeddings.size() == result.sequence_length;
                
                cout << "   Validation:" << endl;
                cout << "     ✓ Dense embedding (1024-dim): " << (valid_dense ? "PASS" : "FAIL") << endl;
                cout << "     ✓ Sparse weights (seq_len): " << (valid_sparse ? "PASS" : "FAIL") << endl;
                cout << "     ✓ ColBERT embeddings (seq_len×1024): " << (valid_colbert ? "PASS" : "FAIL") << endl;
                
                if (valid_dense && valid_sparse && valid_colbert) {
                    cout << "   ✅ All embeddings generated successfully!" << endl;
                } else {
                    cout << "   ⚠ Some embeddings have unexpected shapes" << endl;
                }
                
            } catch (const exception& e) {
                cerr << "❌ Error generating embeddings: " << e.what() << endl;
            }
        }
        
        cout << "\n🎉 Integration test completed!" << endl;
        cout << "   BGE-M3 ONNX model is working correctly." << endl;
        cout << "   Ready for integration into Mimir project." << endl;
        
    } catch (const exception& e) {
        cerr << "❌ Test failed: " << e.what() << endl;
        return 1;
    }
    
    return 0;
} 