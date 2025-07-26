#ifndef ONNX_EMBEDDER_H
#define ONNX_EMBEDDER_H

#include <string>
#include <vector>
#include <onnxruntime_cxx_api.h>

using namespace std;

class OnnxEmbedder {
public:
    OnnxEmbedder(const string& tokenizerPath, const string& modelPath, const string& ortExtensionsPath);
    vector<vector<float>> embed(const vector<string>& texts);

private:
    Ort::Env env;
    Ort::SessionOptions sessionOptions;
    unique_ptr<Ort::Session> modelSession;
    size_t embeddingDim;
    void loadSessions(const string& tokenizerPath, const string& modelPath, const string& ortExtensionsPath);
};

#endif // ONNX_EMBEDDER_H 