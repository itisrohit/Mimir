#include "OnnxEmbedder.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

OnnxEmbedder::OnnxEmbedder(const string& tokenizerPath, const string& modelPath, const string& ortExtensionsPath)
    : env(ORT_LOGGING_LEVEL_WARNING, "OnnxEmbedder")
{
    loadSessions(tokenizerPath, modelPath, ortExtensionsPath);
}

void OnnxEmbedder::loadSessions(const string& tokenizerPath, const string& modelPath, const string& ortExtensionsPath) {
    // Note: We'll use Python for tokenizer, so we don't load the ONNX tokenizer here
    // The embedding model doesn't need custom ops, so we use a separate session options
    Ort::SessionOptions modelSessionOptions;
    modelSession = make_unique<Ort::Session>(env, modelPath.c_str(), modelSessionOptions);
    embeddingDim = 1024;
}

vector<vector<float>> OnnxEmbedder::embed(const vector<string>& texts) {
    vector<vector<float>> embeddings;
    if (texts.empty()) return embeddings;
    
    // Use Python tokenizer wrapper
    vector<vector<int64_t>> inputIdsBatch, attentionMaskBatch;
    
    for (const auto& text : texts) {
        // Call Python tokenizer
        string command = "source venv/bin/activate && python3 scripts/tokenizer_wrapper.py '" + text + "'";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            cerr << "Failed to call Python tokenizer" << endl;
            return embeddings;
        }
        
        char buffer[4096];
        string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // Parse JSON result
        try {
            json tokenizer_outputs = json::parse(result);
            
            // Extract tokens and token_indices (outputs 0 and 2)
            vector<int64_t> tokens = tokenizer_outputs[0];
            vector<int64_t> token_indices = tokenizer_outputs[2];
            
            // Sort tokens by token_indices
            vector<pair<int64_t, int64_t>> pairs;
            for (size_t i = 0; i < tokens.size() && i < token_indices.size(); ++i) {
                pairs.emplace_back(token_indices[i], tokens[i]);
            }
            sort(pairs.begin(), pairs.end());
            
            vector<int64_t> orderedTokens, attentionMask;
            for (auto& p : pairs) {
                orderedTokens.push_back(p.second);
                attentionMask.push_back(p.second == 1 ? 0 : 1); // pad token id = 1
            }
            
            inputIdsBatch.push_back(orderedTokens);
            attentionMaskBatch.push_back(attentionMask);
            
        } catch (const json::exception& e) {
            cerr << "Failed to parse tokenizer output: " << e.what() << endl;
            return embeddings;
        }
    }
    
    // Pad to max length
    size_t maxLenInBatch = 0;
    for (const auto& ids : inputIdsBatch) {
        maxLenInBatch = max(maxLenInBatch, ids.size());
    }
    for (auto& ids : inputIdsBatch) ids.resize(maxLenInBatch, 1);
    for (auto& mask : attentionMaskBatch) mask.resize(maxLenInBatch, 0);
    
    // Create input tensors for model
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    vector<int64_t> inputShape = {static_cast<int64_t>(texts.size()), static_cast<int64_t>(maxLenInBatch)};
    vector<int64_t> flatInputIds, flatAttentionMask;
    for (size_t i = 0; i < texts.size(); ++i) {
        flatInputIds.insert(flatInputIds.end(), inputIdsBatch[i].begin(), inputIdsBatch[i].end());
        flatAttentionMask.insert(flatAttentionMask.end(), attentionMaskBatch[i].begin(), attentionMaskBatch[i].end());
    }
    auto inputIdsTensor = Ort::Value::CreateTensor<int64_t>(memoryInfo, flatInputIds.data(), flatInputIds.size(), inputShape.data(), inputShape.size());
    auto attentionMaskTensor = Ort::Value::CreateTensor<int64_t>(memoryInfo, flatAttentionMask.data(), flatAttentionMask.size(), inputShape.data(), inputShape.size());
    
    // Run embedding model
    vector<const char*> modelInputNames = {"input_ids", "attention_mask"};
    vector<const char*> modelOutputNames = {"dense_vecs"};
    vector<Ort::Value> modelInputs;
    modelInputs.emplace_back(std::move(inputIdsTensor));
    modelInputs.emplace_back(std::move(attentionMaskTensor));
    auto modelOutputs = modelSession->Run(Ort::RunOptions{nullptr}, modelInputNames.data(), modelInputs.data(), 2, modelOutputNames.data(), 1);
    auto& denseVal = modelOutputs[0];
    const float* denseData = denseVal.GetTensorData<float>();
    for (size_t i = 0; i < texts.size(); ++i) {
        vector<float> emb(denseData + i * embeddingDim, denseData + (i + 1) * embeddingDim);
        embeddings.push_back(emb);
    }
    return embeddings;
} 