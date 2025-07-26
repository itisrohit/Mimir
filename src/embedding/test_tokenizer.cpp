#include "SentencePieceTokenizer.h"
#include <iostream>

using namespace std;

int main() {
    try {
        // Initialize tokenizer
        SentencePieceTokenizer tokenizer("models/bge-m3-onnx/sentencepiece.bpe.model");
        
        if (!tokenizer.isLoaded()) {
            cerr << "❌ Tokenizer failed to load" << endl;
            return 1;
        }
        
        // Test single tokenization
        string test_text = "Hello world!";
        vector<int> tokens = tokenizer.tokenize(test_text);
        
        cout << "🔤 Test text: '" << test_text << "'" << endl;
        cout << "🎯 Token IDs: ";
        for (int token : tokens) {
            cout << token << " ";
        }
        cout << endl;
        
        // Test batch tokenization
        vector<string> texts = {"Hello world!", "How are you?", "This is a test."};
        vector<vector<int>> batch_tokens = tokenizer.tokenizeBatch(texts);
        
        cout << "\n📦 Batch tokenization:" << endl;
        for (size_t i = 0; i < texts.size(); ++i) {
            cout << "Text " << i + 1 << ": '" << texts[i] << "' -> ";
            for (int token : batch_tokens[i]) {
                cout << token << " ";
            }
            cout << endl;
        }
        
        cout << "\n✅ SentencePiece tokenizer wrapper test PASSED" << endl;
        return 0;
        
    } catch (const exception& e) {
        cerr << "❌ Exception: " << e.what() << endl;
        return 1;
    }
} 