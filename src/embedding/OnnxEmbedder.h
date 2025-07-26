#ifndef ONNX_EMBEDDER_H
#define ONNX_EMBEDDER_H

#include <string>
#include <vector>
#include <memory>

// Forward declarations for ONNX Runtime and Tokenizers
namespace Ort { struct Env; struct Session; }
namespace tokenizers { class Tokenizer; }

class OnnxEmbedder {
public:
    // model_path: path to model.onnx
    // tokenizer_path: path to tokenizer.json
    // max_length: max sequence length
    // num_threads: ONNX inference threads
    OnnxEmbedder(const std::string& model_path, const std::string& tokenizer_path, int max_length, int num_threads);
    ~OnnxEmbedder();
    std::vector<std::vector<float>> get_embeddings(const std::vector<std::string>& texts);
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // ONNX_EMBEDDER_H 