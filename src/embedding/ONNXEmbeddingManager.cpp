#include "ONNXEmbeddingManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>

ONNXEmbeddingManager::ONNXEmbeddingManager() 
    : embedding_dim(1024), max_sequence_length(8192), model_loaded(false) {
    env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "bge_m3_onnx");
}

ONNXEmbeddingManager::~ONNXEmbeddingManager() {
    session.reset();
}

bool ONNXEmbeddingManager::initialize(const string& model_path, const string& tokenizer_path) {
    try {
        cout << "🔧 Initializing BGE-M3 ONNX model..." << endl;
        
        // Load tokenizer
        if (!loadTokenizer(tokenizer_path)) {
            cerr << "❌ Failed to load tokenizer from: " << tokenizer_path << endl;
            return false;
        }
        
        // Load ONNX model
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        
        session = make_unique<Ort::Session>(env, model_path.c_str(), session_options);
        
        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        
        size_t num_inputs = session->GetInputCount();
        size_t num_outputs = session->GetOutputCount();
        
        cout << "📊 Model Info:" << endl;
        cout << "   Inputs: " << num_inputs << endl;
        cout << "   Outputs: " << num_outputs << endl;
        
        // Get input names
        for (size_t i = 0; i < num_inputs; i++) {
            auto input_name = session->GetInputNameAllocated(i, allocator);
            input_names.push_back(string(input_name.get()));
            cout << "   Input " << i << ": " << input_names[i] << endl;
        }
        
        // Get output names
        for (size_t i = 0; i < num_outputs; i++) {
            auto output_name = session->GetOutputNameAllocated(i, allocator);
            output_names.push_back(string(output_name.get()));
            cout << "   Output " << i << ": " << output_names[i] << endl;
        }
        
        model_loaded = true;
        cout << "✅ BGE-M3 ONNX model initialized successfully!" << endl;
        return true;
        
    } catch (const Ort::Exception& e) {
        cerr << "❌ Failed to initialize ONNX model: " << e.what() << endl;
        return false;
    }
}

bool ONNXEmbeddingManager::loadTokenizer(const string& tokenizer_path) {
    try {
        // Load tokenizer.json
        string tokenizer_file = tokenizer_path + "/tokenizer.json";
        ifstream file(tokenizer_file);
        if (!file.is_open()) {
            cerr << "❌ Could not open tokenizer file: " << tokenizer_file << endl;
            return false;
        }
        
        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        tokenizer_config = json::parse(content);
        
        cout << "✅ Tokenizer loaded successfully" << endl;
        return true;
        
    } catch (const exception& e) {
        cerr << "❌ Failed to load tokenizer: " << e.what() << endl;
        return false;
    }
}

vector<int64_t> ONNXEmbeddingManager::tokenize(const string& text) {
    // Simple tokenization for BGE-M3
    // In a real implementation, you would use the actual tokenizer
    // For now, we'll use a simple approach with common tokens
    
    vector<int64_t> tokens;
    
    // Add [CLS] token
    tokens.push_back(2); // BGE-M3 uses 2 as CLS token
    
    // Simple word-based tokenization (for demonstration)
    istringstream iss(text);
    string word;
    while (iss >> word) {
        // Simple hash-based token ID (not accurate but functional for testing)
        int64_t token_id = hash<string>{}(word) % 1000 + 100; // Avoid special tokens
        tokens.push_back(token_id);
    }
    
    // Add [SEP] token
    tokens.push_back(1); // BGE-M3 uses 1 as SEP token
    
    // Truncate if too long
    if (tokens.size() > max_sequence_length) {
        tokens.resize(max_sequence_length);
        tokens.back() = 1; // Ensure SEP token at end
    }
    
    return tokens;
}

EmbeddingResult ONNXEmbeddingManager::generateEmbeddings(const string& text) {
    if (!model_loaded) {
        throw runtime_error("Model not initialized. Call initialize() first.");
    }
    
    try {
        // Tokenize input
        vector<int64_t> tokens = tokenize(text);
        size_t seq_len = tokens.size();
        
        // Create attention mask
        vector<int64_t> attention_mask(seq_len, 1);
        
        // Prepare input tensors
        Ort::AllocatorWithDefaultOptions allocator;
        vector<Ort::Value> input_tensors;
        
        // Input shape: [batch_size, sequence_length]
        vector<int64_t> input_shape = {1, static_cast<int64_t>(seq_len)};
        
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            allocator.GetInfo(), tokens.data(), tokens.size(), 
            input_shape.data(), input_shape.size()));
        
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            allocator.GetInfo(), attention_mask.data(), attention_mask.size(),
            input_shape.data(), input_shape.size()));
        
        // Prepare output tensors
        vector<Ort::Value> output_tensors;
        
        // Dense embeddings: [1, 1024]
        vector<int64_t> dense_shape = {1, 1024};
        vector<float> dense_output(1024);
        output_tensors.push_back(Ort::Value::CreateTensor<float>(
            allocator.GetInfo(), dense_output.data(), dense_output.size(),
            dense_shape.data(), dense_shape.size()));
        
        // Sparse weights: [1, seq_len, 1]
        vector<int64_t> sparse_shape = {1, static_cast<int64_t>(seq_len), 1};
        vector<float> sparse_output(seq_len);
        output_tensors.push_back(Ort::Value::CreateTensor<float>(
            allocator.GetInfo(), sparse_output.data(), sparse_output.size(),
            sparse_shape.data(), sparse_shape.size()));
        
        // ColBERT embeddings: [1, seq_len, 1024]
        vector<int64_t> colbert_shape = {1, static_cast<int64_t>(seq_len), 1024};
        vector<float> colbert_output(seq_len * 1024);
        output_tensors.push_back(Ort::Value::CreateTensor<float>(
            allocator.GetInfo(), colbert_output.data(), colbert_output.size(),
            colbert_shape.data(), colbert_shape.size()));
        
        // Convert names to const char*
        vector<const char*> input_name_ptrs;
        vector<const char*> output_name_ptrs;
        for (const auto& name : input_names) {
            input_name_ptrs.push_back(name.c_str());
        }
        for (const auto& name : output_names) {
            output_name_ptrs.push_back(name.c_str());
        }
        
        // Run inference
        session->Run(Ort::RunOptions{nullptr}, 
                     input_name_ptrs.data(), input_tensors.data(), input_tensors.size(),
                     output_name_ptrs.data(), output_tensors.data(), output_tensors.size());
        
        // Extract results
        EmbeddingResult result;
        result.sequence_length = seq_len;
        
        // Dense embeddings
        float* dense_data = output_tensors[0].GetTensorMutableData<float>();
        result.dense_embedding.assign(dense_data, dense_data + 1024);
        
        // Sparse weights
        float* sparse_data = output_tensors[1].GetTensorMutableData<float>();
        result.sparse_weights.assign(sparse_data, sparse_data + seq_len);
        
        // ColBERT embeddings
        float* colbert_data = output_tensors[2].GetTensorMutableData<float>();
        for (size_t i = 0; i < seq_len; i++) {
            vector<float> token_embedding(colbert_data + i * 1024, colbert_data + (i + 1) * 1024);
            result.colbert_embeddings.push_back(token_embedding);
        }
        
        return result;
        
    } catch (const Ort::Exception& e) {
        throw runtime_error("Inference failed: " + string(e.what()));
    }
}

void ONNXEmbeddingManager::printEmbeddingInfo(const EmbeddingResult& result) {
    cout << "\n📊 Embedding Results:" << endl;
    cout << "   Sequence length: " << result.sequence_length << endl;
    cout << "   Dense embedding size: " << result.dense_embedding.size() << endl;
    cout << "   Sparse weights size: " << result.sparse_weights.size() << endl;
    cout << "   ColBERT embeddings: " << result.colbert_embeddings.size() << " tokens" << endl;
    
    // Print sample values
    cout << "   Dense sample values: [";
    for (size_t i = 0; i < min(static_cast<size_t>(5), result.dense_embedding.size()); i++) {
        cout << result.dense_embedding[i];
        if (i < 4) cout << ", ";
    }
    cout << "]" << endl;
} 