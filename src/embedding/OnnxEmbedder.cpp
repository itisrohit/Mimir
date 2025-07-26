// OnnxEmbedder.cpp
// Implementation: ONNX Runtime + HuggingFace Tokenizers C++
#include "OnnxEmbedder.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

// ONNX Runtime and Tokenizers C++ includes
#include <onnxruntime_cxx_api.h>
#include <tokenizers_cpp.h>
#include <fstream>

// Helper to load file as string
static std::string LoadStringFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open file: " + path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

struct OnnxEmbedder::Impl {
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    std::shared_ptr<tokenizers::Tokenizer> tokenizer;
    std::string model_path;
    std::string tokenizer_path;
    int max_length;
    int num_threads;
    Impl(const std::string& model, const std::string& tokenizer_dir, int max_len, int threads)
        : model_path(model), tokenizer_path(tokenizer_dir), max_length(max_len), num_threads(threads) {
        // Verify files exist
        std::ifstream model_file(model_path);
        if (!model_file.good()) {
            throw std::runtime_error("ONNX model file not found: " + model_path);
        }
        std::string tokenizer_json = tokenizer_path + "/tokenizer.json";
        std::ifstream tokenizer_file(tokenizer_json);
        if (!tokenizer_file.good()) {
            throw std::runtime_error("Tokenizer file not found: " + tokenizer_json);
        }
        std::cout << "[OnnxEmbedder] Files verified. Model: " << model_path << std::endl;
        std::cout << "[OnnxEmbedder] Tokenizer: " << tokenizer_json << std::endl;
        // Initialize ONNX Runtime environment and session
        env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "OnnxEmbedder");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(num_threads);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        session = std::make_unique<Ort::Session>(*env, model_path.c_str(), session_options);
        // Print all output names for debugging
        size_t num_outputs = session->GetOutputCount();
        Ort::AllocatorWithDefaultOptions allocator;
        std::cout << "[OnnxEmbedder] Model output names:" << std::endl;
        for (size_t i = 0; i < num_outputs; ++i) {
            Ort::AllocatedStringPtr output_name = session->GetOutputNameAllocated(i, allocator);
            std::cout << "  Output " << i << ": " << output_name.get() << std::endl;
        }
        // Load HuggingFace Tokenizer from tokenizer.json using mlc-ai/tokenizers-cpp
        std::string blob = LoadStringFromFile(tokenizer_json);
        tokenizer = tokenizers::Tokenizer::FromBlobJSON(blob);
        if (!tokenizer) throw std::runtime_error("Failed to load tokenizer from " + tokenizer_json);
        std::cout << "[OnnxEmbedder] ONNX session and tokenizer initialized successfully" << std::endl;
        std::cout << "[OnnxEmbedder] Ready for embedding generation" << std::endl;
    }
    ~Impl() {}
};

OnnxEmbedder::OnnxEmbedder(const std::string& model_path, const std::string& tokenizer_path, int max_length, int num_threads)
    : pImpl(std::make_unique<Impl>(model_path, tokenizer_path, max_length, num_threads)) {}

OnnxEmbedder::~OnnxEmbedder() = default;

std::vector<std::vector<float>> OnnxEmbedder::get_embeddings(const std::vector<std::string>& texts) {
    if (texts.empty()) return {};
    std::cout << "[OnnxEmbedder] Processing " << texts.size() << " texts" << std::endl;
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(texts.size()); // Pre-allocate for better performance
    
    try {
        // Process texts in batches for better performance
        const size_t batch_size = 2; // Try smaller batch size first
        const size_t total_texts = texts.size();
        
        for (size_t batch_start = 0; batch_start < total_texts; batch_start += batch_size) {
            size_t batch_end = std::min(batch_start + batch_size, total_texts);
            size_t current_batch_size = batch_end - batch_start;
            
            std::cout << "[OnnxEmbedder] Processing batch " << (batch_start / batch_size + 1) 
                      << " (" << current_batch_size << " texts)" << std::endl;
            
            // 1. Tokenize all texts in this batch
            std::vector<std::vector<int64_t>> batch_input_ids;
            std::vector<std::vector<int64_t>> batch_attention_masks;
            size_t max_seq_len = 0;
            
            for (size_t i = batch_start; i < batch_end; ++i) {
                auto ids = pImpl->tokenizer->Encode(texts[i]);
                std::vector<int64_t> input_ids(ids.begin(), ids.end());
                std::vector<int64_t> attention_mask(ids.size(), 1);
                
                batch_input_ids.push_back(std::move(input_ids));
                batch_attention_masks.push_back(std::move(attention_mask));
                max_seq_len = std::max(max_seq_len, ids.size());
            }
            
            // 2. Pad sequences to the same length
            for (auto& input_ids : batch_input_ids) {
                input_ids.resize(max_seq_len, 0); // Pad with 0
            }
            for (auto& attention_mask : batch_attention_masks) {
                attention_mask.resize(max_seq_len, 0); // Pad with 0
            }
            
            // 3. Prepare ONNX input tensorsc
            Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
            
            // Create input_ids tensor - flatten the batch
            std::vector<int64_t> input_ids_flat;
            for (const auto& ids : batch_input_ids) {
                input_ids_flat.insert(input_ids_flat.end(), ids.begin(), ids.end());
            }
            std::vector<int64_t> input_shape = {static_cast<int64_t>(current_batch_size), static_cast<int64_t>(max_seq_len)};
            
            // Create attention_mask tensor - flatten the batch
            std::vector<int64_t> attention_mask_flat;
            for (const auto& mask : batch_attention_masks) {
                attention_mask_flat.insert(attention_mask_flat.end(), mask.begin(), mask.end());
            }
            
            // Create task_id tensor (all ones for passage embedding)
            std::vector<int64_t> task_ids(current_batch_size, 1); // passage task_id = 1
            
            std::vector<Ort::Value> input_tensors;
            
            // Create input_ids tensor
            input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
                memory_info, input_ids_flat.data(), input_ids_flat.size(),
                input_shape.data(), input_shape.size()));
            
            // Create attention_mask tensor
            input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
                memory_info, attention_mask_flat.data(), attention_mask_flat.size(),
                input_shape.data(), input_shape.size()));
            
            // Create task_id tensor
            input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
                memory_info, task_ids.data(), task_ids.size(),
                std::vector<int64_t>{static_cast<int64_t>(current_batch_size)}.data(), 1));
            
            // 4. Run inference
            std::vector<const char*> input_names = {"input_ids", "attention_mask", "task_id"};
            std::vector<const char*> output_names = {"text_embeds"};
            
            auto output_tensors = pImpl->session->Run(Ort::RunOptions{nullptr}, 
                                                    input_names.data(), 
                                                    input_tensors.data(), 
                                                    input_tensors.size(),
                                                    output_names.data(), 
                                                    output_names.size());
            
            // 5. Extract embedding vectors from output_tensors
            if (!output_tensors.empty()) {
                Ort::Value& output_tensor = output_tensors[0];
                float* output_data = output_tensor.GetTensorMutableData<float>();
                auto shape = output_tensor.GetTensorTypeAndShapeInfo().GetShape();
                size_t batch_size_out = shape[0];
                size_t hidden_size = shape.back();
                
                std::cout << "[OnnxEmbedder] Output tensor shape: [";
                for (size_t i = 0; i < shape.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << shape[i];
                }
                std::cout << "]" << std::endl;
                
                // Handle different output shapes
                if (shape.size() == 2) {
                    // Shape: [batch, hidden] - direct embeddings
                    for (size_t i = 0; i < batch_size_out; ++i) {
                        std::vector<float> embedding(output_data + i * hidden_size, 
                                                   output_data + (i + 1) * hidden_size);
                        embeddings.push_back(std::move(embedding));
                    }
                } else if (shape.size() == 3) {
                    // Shape: [batch, seq, hidden] - need mean pooling
                    size_t seq_len = shape[1];
                    for (size_t i = 0; i < batch_size_out; ++i) {
                        std::vector<float> embedding(hidden_size, 0.0f);
                        
                        // Mean pooling over sequence dimension
                        for (size_t j = 0; j < seq_len; ++j) {
                            for (size_t k = 0; k < hidden_size; ++k) {
                                embedding[k] += output_data[i * seq_len * hidden_size + j * hidden_size + k];
                            }
                        }
                        
                        // Normalize by sequence length
                        for (size_t k = 0; k < hidden_size; ++k) {
                            embedding[k] /= seq_len;
                        }
                        
                        embeddings.push_back(std::move(embedding));
                    }
                } else {
                    throw std::runtime_error("Unexpected output tensor shape: " + std::to_string(shape.size()) + " dimensions");
                }
            }
        }
        
        std::cout << "[OnnxEmbedder] Successfully generated " << embeddings.size() << " embeddings" << std::endl;
        return embeddings;
        
    } catch (const Ort::Exception& e) {
        std::cerr << "[OnnxEmbedder] ONNX Runtime error: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "[OnnxEmbedder] Error: " << e.what() << std::endl;
        throw;
    }
} 


