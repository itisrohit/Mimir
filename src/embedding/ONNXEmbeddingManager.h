#ifndef ONNX_EMBEDDING_MANAGER_H
#define ONNX_EMBEDDING_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

struct EmbeddingResult {
    vector<float> dense_embedding;      // Shape: (1024,)
    vector<float> sparse_weights;       // Shape: (seq_len, 1)
    vector<vector<float>> colbert_embeddings; // Shape: (seq_len, 1024)
    size_t sequence_length;
};

class ONNXEmbeddingManager {
public:
    ONNXEmbeddingManager();
    ~ONNXEmbeddingManager();
    
    // Initialize the model and tokenizer
    bool initialize(const string& model_path, const string& tokenizer_path);
    
    // Generate embeddings for a single text
    EmbeddingResult generateEmbeddings(const string& text);
    
    // Get model info
    size_t getEmbeddingDimension() const { return embedding_dim; }
    size_t getMaxSequenceLength() const { return max_sequence_length; }
    
    // Check if model is loaded
    bool isInitialized() const { return model_loaded; }
    
    // Print embedding information
    void printEmbeddingInfo(const EmbeddingResult& result);

private:
    // ONNX Runtime session
    unique_ptr<Ort::Session> session;
    Ort::Env env;
    
    // Model configuration
    size_t embedding_dim;
    size_t max_sequence_length;
    bool model_loaded;
    
    // Input/output names
    vector<string> input_names;
    vector<string> output_names;
    
    // Tokenizer data
    json tokenizer_config;
    json vocab;
    vector<string> merges;
    
    // Tokenization methods
    vector<int64_t> tokenize(const string& text);
    
    // Helper methods
    bool loadTokenizer(const string& tokenizer_path);
};

#endif // ONNX_EMBEDDING_MANAGER_H 