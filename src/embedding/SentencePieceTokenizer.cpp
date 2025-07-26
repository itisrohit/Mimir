#include "SentencePieceTokenizer.h"
#include <iostream>

using namespace std;

SentencePieceTokenizer::SentencePieceTokenizer(const string& modelPath) : modelLoaded(false) {
    cout << "🔧 SentencePieceTokenizer: Starting to load model from " << modelPath << endl;
    const auto status = processor.Load(modelPath);
    cout << "🔧 SentencePieceTokenizer: Load() completed, checking status..." << endl;
    if (status.ok()) {
        modelLoaded = true;
        cout << "✅ SentencePiece tokenizer loaded successfully" << endl;
        cout << "📊 Vocabulary size: " << processor.GetPieceSize() << endl;
    } else {
        cerr << "❌ Failed to load SentencePiece model: " << status.ToString() << endl;
    }
    cout << "🔧 SentencePieceTokenizer: Constructor completed" << endl;
}

vector<int> SentencePieceTokenizer::tokenize(const string& text, bool addSpecialTokens) {
    if (!modelLoaded) {
        cerr << "❌ Tokenizer not loaded" << endl;
        return {};
    }
    
    vector<int> tokens;
    processor.Encode(text, &tokens);
    
    if (addSpecialTokens) {
        tokens = this->addSpecialTokens(tokens);
    }
    
    return tokens;
}

vector<vector<int>> SentencePieceTokenizer::tokenizeBatch(const vector<string>& texts, bool addSpecialTokens) {
    vector<vector<int>> batchTokens;
    batchTokens.reserve(texts.size());
    
    for (const string& text : texts) {
        batchTokens.push_back(tokenize(text, addSpecialTokens));
    }
    
    return batchTokens;
}

size_t SentencePieceTokenizer::getVocabSize() const {
    return modelLoaded ? processor.GetPieceSize() : 0;
}

bool SentencePieceTokenizer::isLoaded() const {
    return modelLoaded;
}

vector<int> SentencePieceTokenizer::addSpecialTokens(const vector<int>& tokens) {
    vector<int> result = tokens;
    
    // Add BOS token if not present
    if (result.empty() || result[0] != 0) {
        result.insert(result.begin(), 0);
    }
    
    // Add EOS token if not present
    if (result.empty() || result.back() != 2) {
        result.push_back(2);
    }
    
    return result;
} 