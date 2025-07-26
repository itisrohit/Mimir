#ifndef ONNX_EMBEDDER_H
#define ONNX_EMBEDDER_H

#include <string>
#include <vector>
#include <onnxruntime_cxx_api.h>
#include "SentencePieceTokenizer.h"

using namespace std;

class OnnxEmbedder {
public:
    OnnxEmbedder(const string& tokenizerPath, const string& modelPath);
    vector<vector<float>> embed(const vector<string>& texts);
    
private:
    Ort::Env env;
    unique_ptr<Ort::Session> modelSession;
    unique_ptr<SentencePieceTokenizer> tokenizer;
    size_t embeddingDim;
    void loadSessions(const string& tokenizerPath, const string& modelPath);
    vector<vector<float>> processBatch(const vector<string>& texts);
};

#endif // ONNX_EMBEDDER_H 