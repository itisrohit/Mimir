#pragma once

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include "SentencePieceTokenizer.h"

using namespace std;

class OnnxEmbedder {
private:
    Ort::Env env;
    shared_ptr<Ort::Session> modelSession;
    shared_ptr<class SentencePieceTokenizer> tokenizer;
    size_t embeddingDim;

    void loadSessions(const string& tokenizerPath, const string& modelPath);
    vector<vector<float>> processBatch(const vector<string>& texts);

public:
    OnnxEmbedder(const string& tokenizerPath, const string& modelPath);
    vector<vector<float>> embed(const vector<string>& texts);
}; 