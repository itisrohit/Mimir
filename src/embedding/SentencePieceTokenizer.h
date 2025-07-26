#ifndef SENTENCEPIECE_TOKENIZER_H
#define SENTENCEPIECE_TOKENIZER_H

#include <string>
#include <vector>
#include <sentencepiece_processor.h>

using namespace std;

class SentencePieceTokenizer {
public:
    SentencePieceTokenizer(const string& modelPath);
    ~SentencePieceTokenizer() = default;
    
    // Tokenize a single text
    vector<int> tokenize(const string& text, bool addSpecialTokens = true);
    
    // Tokenize multiple texts (batch processing)
    vector<vector<int>> tokenizeBatch(const vector<string>& texts, bool addSpecialTokens = true);
    
    // Get vocabulary size
    size_t getVocabSize() const;
    
    // Check if model is loaded
    bool isLoaded() const;

private:
    sentencepiece::SentencePieceProcessor processor;
    bool modelLoaded;
    
    // Add special tokens (BOS, EOS) if needed
    vector<int> addSpecialTokens(const vector<int>& tokens);
};

#endif // SENTENCEPIECE_TOKENIZER_H 