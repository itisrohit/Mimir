#ifndef ONNX_EMBEDDING_MANAGER_H
#define ONNX_EMBEDDING_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <nlohmann/json.hpp>
// REMOVED: #include "../../external/tokenizers-cpp/include/tokenizers_cpp.h"

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
    // ONNX Runtime sessions
    std::unique_ptr<Ort::Session> session; // Embedding model
    std::unique_ptr<Ort::Session> tokenizer_session; // Tokenizer model
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::SessionOptions tokenizer_session_options;
    
    // Model configuration
    size_t embedding_dim;
    size_t max_sequence_length;
    bool model_loaded;
    bool tokenizer_loaded;
    
    // Input/output names
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<std::string> tokenizer_input_names;
    std::vector<std::string> tokenizer_output_names;
    
    // Tokenization methods
    bool loadTokenizer(const std::string& tokenizer_path);
    bool loadModel(const std::string& model_path);
    void runTokenizer(const std::string& text, std::vector<int64_t>& input_ids, std::vector<int64_t>& attention_mask);
    
    // Helper methods
    // REMOVED: std::unique_ptr<tokenizers::Tokenizer> tokenizer;
    // REMOVED: std::vector<int64_t> tokenize(const std::string& text);
};

#endif // ONNX_EMBEDDING_MANAGER_H 