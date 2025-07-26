#include "ONNXEmbeddingManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>
// REMOVED: #include "../../external/tokenizers-cpp/include/tokenizers_cpp.h"

ONNXEmbeddingManager::ONNXEmbeddingManager() 
    : embedding_dim(1024), max_sequence_length(8192), model_loaded(false) {
    env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "bge_m3_onnx");
}

ONNXEmbeddingManager::~ONNXEmbeddingManager() {
    session.reset();
}

bool ONNXEmbeddingManager::initialize(const string& model_path, const string& tokenizer_path) {
    try {
        cout << "🔧 Initializing BGE-M3 ONNX model and tokenizer..." << endl;
        if (!loadTokenizer(tokenizer_path)) {
            cerr << "❌ Failed to load tokenizer from: " << tokenizer_path << endl;
            return false;
        }
        if (!loadModel(model_path)) {
            cerr << "❌ Failed to load ONNX model from: " << model_path << endl;
            return false;
        }
        return true;
    } catch (const Ort::Exception& e) {
        cerr << "❌ Failed to initialize ONNX model or tokenizer: " << e.what() << endl;
        return false;
    }
}

bool ONNXEmbeddingManager::loadModel(const string& model_path) {
    try {
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        session = std::make_unique<Ort::Session>(env, model_path.c_str(), session_options);
        Ort::AllocatorWithDefaultOptions allocator;
        size_t num_inputs = session->GetInputCount();
        size_t num_outputs = session->GetOutputCount();
        cout << "📊 Model Info:" << endl;
        cout << "   Inputs: " << num_inputs << endl;
        cout << "   Outputs: " << num_outputs << endl;
        for (size_t i = 0; i < num_inputs; i++) {
            auto input_name = session->GetInputNameAllocated(i, allocator);
            input_names.push_back(string(input_name.get()));
            cout << "   Input " << i << ": " << input_names[i] << endl;
        }
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
        string tokenizer_onnx = tokenizer_path + "/tokenizer.onnx";
        tokenizer_session_options.SetIntraOpNumThreads(1);
        tokenizer_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        // Register ONNX Runtime Extensions if needed here
        tokenizer_session = std::make_unique<Ort::Session>(env, tokenizer_onnx.c_str(), tokenizer_session_options);
        Ort::AllocatorWithDefaultOptions allocator;
        size_t num_inputs = tokenizer_session->GetInputCount();
        size_t num_outputs = tokenizer_session->GetOutputCount();
        for (size_t i = 0; i < num_inputs; i++) {
            auto input_name = tokenizer_session->GetInputNameAllocated(i, allocator);
            tokenizer_input_names.push_back(string(input_name.get()));
        }
        for (size_t i = 0; i < num_outputs; i++) {
            auto output_name = tokenizer_session->GetOutputNameAllocated(i, allocator);
            tokenizer_output_names.push_back(string(output_name.get()));
        }
        tokenizer_loaded = true;
        cout << "✅ Tokenizer ONNX model loaded successfully!" << endl;
        return true;
    } catch (const Ort::Exception& e) {
        cerr << "❌ Failed to load tokenizer ONNX model: " << e.what() << endl;
        return false;
    }
}

void ONNXEmbeddingManager::runTokenizer(const std::string& text, std::vector<int64_t>& input_ids, std::vector<int64_t>& attention_mask) {
    if (!tokenizer_loaded) throw std::runtime_error("Tokenizer ONNX model not loaded");
    // Prepare input (batch of 1)
    std::vector<int64_t> input_shape = {1};
    std::vector<const char*> input_names;
    for (const auto& n : tokenizer_input_names) input_names.push_back(n.c_str());
    std::vector<const char*> output_names;
    for (const auto& n : tokenizer_output_names) output_names.push_back(n.c_str());
    std::vector<Ort::Value> input_tensors;
    Ort::AllocatorWithDefaultOptions allocator;
    // The tokenizer ONNX expects a string input
    std::vector<std::string> texts = {text};
    Ort::Value string_tensor = Ort::Value::CreateTensor(allocator, input_shape.data(), input_shape.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING);
    std::vector<const char*> cstrs;
    for (const auto& s : texts) cstrs.push_back(s.c_str());
    string_tensor.FillStringTensor(cstrs.data(), cstrs.size());
    input_tensors.push_back(std::move(string_tensor));
    // Run tokenizer session
    auto output_tensors = tokenizer_session->Run(Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(), input_tensors.size(), output_names.data(), output_names.size());
    // Extract input_ids and attention_mask from outputs
    // Assume output 0: input_ids, output 1: attention_mask (verify with your tokenizer ONNX outputs)
    int64_t* ids = output_tensors[0].GetTensorMutableData<int64_t>();
    int64_t* mask = output_tensors[1].GetTensorMutableData<int64_t>();
    auto ids_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    size_t seq_len = ids_shape[1];
    input_ids.assign(ids, ids + seq_len);
    attention_mask.assign(mask, mask + seq_len);
}

EmbeddingResult ONNXEmbeddingManager::generateEmbeddings(const string& text) {
    if (!model_loaded || !tokenizer_loaded) {
        throw runtime_error("Model or tokenizer not initialized. Call initialize() first.");
    }
    try {
        std::vector<int64_t> input_ids, attention_mask;
        runTokenizer(text, input_ids, attention_mask);
        // Truncate to model's expected max sequence length
        size_t model_max_seq_len = 30;
        // If length is 31, remove the last token (SEP)
        if (input_ids.size() == model_max_seq_len + 1) {
            input_ids.pop_back();
            attention_mask.pop_back();
        } else if (input_ids.size() > model_max_seq_len) {
            input_ids.resize(model_max_seq_len);
            attention_mask.resize(model_max_seq_len);
        }
        // Debug: print only after truncation/removal
        std::cout << "Final input IDs: [";
        for (size_t i = 0; i < input_ids.size(); ++i) {
            std::cout << input_ids[i];
            if (i + 1 < input_ids.size()) std::cout << ", ";
        }
        std::cout << "] (length: " << input_ids.size() << ")" << std::endl;
        size_t seq_len = input_ids.size();
        // Prepare input tensors
        Ort::AllocatorWithDefaultOptions allocator;
        std::vector<Ort::Value> input_tensors;
        std::vector<int64_t> input_shape = {1, static_cast<int64_t>(seq_len)};
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            allocator.GetInfo(), input_ids.data(), input_ids.size(),
            input_shape.data(), input_shape.size()));
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            allocator.GetInfo(), attention_mask.data(), attention_mask.size(),
            input_shape.data(), input_shape.size()));
        // Prepare output tensors
        std::vector<Ort::Value> output_tensors;
        std::vector<int64_t> dense_shape = {1, 1024};
        std::vector<float> dense_output(1024);
        output_tensors.push_back(Ort::Value::CreateTensor<float>(
            allocator.GetInfo(), dense_output.data(), dense_output.size(),
            dense_shape.data(), dense_shape.size()));
        std::vector<int64_t> sparse_shape = {1, static_cast<int64_t>(seq_len), 1};
        std::vector<float> sparse_output(seq_len);
        output_tensors.push_back(Ort::Value::CreateTensor<float>(
            allocator.GetInfo(), sparse_output.data(), sparse_output.size(),
            sparse_shape.data(), sparse_shape.size()));
        std::vector<int64_t> colbert_shape = {1, static_cast<int64_t>(seq_len), 1024};
        std::vector<float> colbert_output(seq_len * 1024);
        output_tensors.push_back(Ort::Value::CreateTensor<float>(
            allocator.GetInfo(), colbert_output.data(), colbert_output.size(),
            colbert_shape.data(), colbert_shape.size()));
        // Convert names to const char*
        std::vector<const char*> input_name_ptrs;
        std::vector<const char*> output_name_ptrs;
        for (const auto& name : input_names) input_name_ptrs.push_back(name.c_str());
        for (const auto& name : output_names) output_name_ptrs.push_back(name.c_str());
        // Run inference
        session->Run(Ort::RunOptions{nullptr},
                     input_name_ptrs.data(), input_tensors.data(), input_tensors.size(),
                     output_name_ptrs.data(), output_tensors.data(), output_tensors.size());
        // Extract results
        EmbeddingResult result;
        result.sequence_length = seq_len;
        float* dense_data = output_tensors[0].GetTensorMutableData<float>();
        result.dense_embedding.assign(dense_data, dense_data + 1024);
        float* sparse_data = output_tensors[1].GetTensorMutableData<float>();
        result.sparse_weights.assign(sparse_data, sparse_data + seq_len);
        float* colbert_data = output_tensors[2].GetTensorMutableData<float>();
        for (size_t i = 0; i < seq_len; i++) {
            std::vector<float> token_embedding(colbert_data + i * 1024, colbert_data + (i + 1) * 1024);
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