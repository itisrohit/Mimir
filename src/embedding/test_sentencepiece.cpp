#include <sentencepiece_processor.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    try {
        // Load the SentencePiece model
        sentencepiece::SentencePieceProcessor processor;
        const auto status = processor.Load("models/bge-m3-onnx/sentencepiece.bpe.model");
        
        if (!status.ok()) {
            cerr << "❌ Failed to load SentencePiece model: " << status.ToString() << endl;
            return 1;
        }
        
        cout << "✅ SentencePiece model loaded successfully" << endl;
        cout << "📊 Vocabulary size: " << processor.GetPieceSize() << endl;
        
        // Test tokenization
        string test_text = "Hello world!";
        vector<int> ids;
        processor.Encode(test_text, &ids);
        
        cout << "🔤 Test text: '" << test_text << "'" << endl;
        cout << "🎯 Token IDs (raw): ";
        for (int id : ids) {
            cout << id << " ";
        }
        cout << endl;
        
        // Test with special tokens (like Python tokenizer)
        vector<int> ids_with_special;
        processor.Encode(test_text, &ids_with_special);
        
        // Add BOS and EOS tokens if needed
        if (ids_with_special.empty() || ids_with_special[0] != 0) {
            ids_with_special.insert(ids_with_special.begin(), 0);  // BOS token
        }
        if (ids_with_special.empty() || ids_with_special.back() != 2) {
            ids_with_special.push_back(2);  // EOS token
        }
        
        cout << "🎯 Token IDs (with special): ";
        for (int id : ids_with_special) {
            cout << id << " ";
        }
        cout << endl;
        
        // Test with special tokens
        vector<string> pieces = processor.EncodeAsPieces(test_text);
        
        cout << "🔤 Token pieces: ";
        for (const string& piece : pieces) {
            cout << "'" << piece << "' ";
        }
        cout << endl;
        
        cout << "✅ SentencePiece C++ test PASSED" << endl;
        return 0;
        
    } catch (const exception& e) {
        cerr << "❌ Exception: " << e.what() << endl;
        return 1;
    }
} 