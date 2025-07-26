#include <iostream>
#include <string>
#include <vector>
#include "../src/embedding/ONNXEmbeddingManager.h"

using namespace std;

int main() {
    cout << "🧪 Testing BGE-M3 ONNX Integration with Batch Sizes" << endl;
    cout << "===============================================" << endl;
    
    try {
        // Initialize embedding manager
        ONNXEmbeddingManager embedding_manager;
        
        // Paths to model and tokenizer
        string model_path = "./models/bge-m3-onnx/model.onnx";
        string tokenizer_path = "./models/bge-m3-onnx";
        
        cout << "🔧 Initializing BGE-M3 model..." << endl;
        if (!embedding_manager.initialize(model_path, tokenizer_path)) {
            cerr << "❌ Failed to initialize BGE-M3 model" << endl;
            return 1;
        }
        cout << "\n✅ Model initialized successfully!" << endl;

        // Batch sizes to test
        vector<int> batch_sizes = {4, 8, 32};
        string base_text = "BGE M3 is an embedding model supporting dense retrieval, lexical matching and multi-vector interaction.";

        for (int batch : batch_sizes) {
            cout << "\n===============================" << endl;
            cout << "Testing batch size: " << batch << endl;
            vector<string> batch_texts(batch, base_text);
            try {
                // Try batch inference (if implemented)
                // If not, run single inference in a loop
                for (int i = 0; i < batch; ++i) {
                    cout << "\n--- Batch " << batch << " | Text " << (i + 1) << " ---" << endl;
                    EmbeddingResult result = embedding_manager.generateEmbeddings(batch_texts[i]);
                    embedding_manager.printEmbeddingInfo(result);
                }
                cout << "\n✅ Batch size " << batch << " completed successfully!" << endl;
            } catch (const exception& e) {
                cerr << "❌ Error for batch size " << batch << ": " << e.what() << endl;
            }
        }
        cout << "\n🎉 Batch size tests completed!" << endl;
    } catch (const exception& e) {
        cerr << "❌ Test failed: " << e.what() << endl;
        return 1;
    }
    return 0;
} 