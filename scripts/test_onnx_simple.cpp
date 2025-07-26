#include <iostream>
#include <string>
#include <vector>
#include <onnxruntime_cxx_api.h>

using namespace std;

int main() {
    cout << "🧪 Testing BGE-M3 ONNX with proper INT64 handling" << endl;
    cout << "==================================================" << endl;
    
    // Initialize ONNX Runtime
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "bge_m3_test");
    
    // Load model
    string model_path = "./models/bge-m3-onnx/model.onnx";
    cout << "🔧 Loading model from: " << model_path << endl;
    
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    
    unique_ptr<Ort::Session> session = make_unique<Ort::Session>(env, model_path.c_str(), session_options);
    
    // Get input/output info
    Ort::AllocatorWithDefaultOptions allocator;
    
    size_t num_inputs = session->GetInputCount();
    size_t num_outputs = session->GetOutputCount();
    
    cout << "📊 Model Info:" << endl;
    cout << "   Inputs: " << num_inputs << endl;
    cout << "   Outputs: " << num_outputs << endl;
    
    // Get input names and shapes
    for (size_t i = 0; i < num_inputs; i++) {
        auto input_name = session->GetInputNameAllocated(i, allocator);
        cout << "   Input " << i << ": " << input_name.get() << endl;
        
        // Get input type info
        auto type_info = session->GetInputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
        auto element_type = tensor_info.GetElementType();
        auto shape = tensor_info.GetShape();
        
        cout << "     Type: " << element_type << endl;
        cout << "     Shape: [";
        for (size_t j = 0; j < shape.size(); j++) {
            cout << shape[j];
            if (j < shape.size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
    
    // Get output names and shapes
    for (size_t i = 0; i < num_outputs; i++) {
        auto output_name = session->GetOutputNameAllocated(i, allocator);
        cout << "   Output " << i << ": " << output_name.get() << endl;
        
        // Get output type info
        auto type_info = session->GetOutputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
        auto element_type = tensor_info.GetElementType();
        auto shape = tensor_info.GetShape();
        
        cout << "     Type: " << element_type << endl;
        cout << "     Shape: [";
        for (size_t j = 0; j < shape.size(); j++) {
            cout << shape[j];
            if (j < shape.size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
    
    // Create test input (simple tokenization for BGE-M3)
    // BGE-M3 typically uses SentencePiece tokenizer
    // For testing, we'll use a simple sequence of token IDs
    vector<int64_t> input_ids = {2, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1}; // [CLS] + tokens + [SEP]
    vector<int64_t> attention_mask(input_ids.size(), 1);
    
    cout << "\n🔤 Test Input:" << endl;
    cout << "   Input IDs: [";
    for (size_t i = 0; i < input_ids.size(); i++) {
        cout << input_ids[i];
        if (i < input_ids.size() - 1) cout << ", ";
    }
    cout << "]" << endl;
    cout << "   Sequence length: " << input_ids.size() << endl;
    
    // Get input names and store them properly
    vector<string> input_name_strings;
    for (size_t i = 0; i < num_inputs; i++) {
        auto input_name = session->GetInputNameAllocated(i, allocator);
        input_name_strings.push_back(string(input_name.get()));
    }
    
    // Convert to const char* for ONNX Runtime
    vector<const char*> input_names;
    for (const auto& name : input_name_strings) {
        input_names.push_back(name.c_str());
    }
    
    // Create input tensors with proper shapes
    vector<int64_t> input_shape = {1, static_cast<int64_t>(input_ids.size())}; // batch_size=1, seq_len
    
    vector<Ort::Value> input_tensors;
    input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
        allocator.GetInfo(), input_ids.data(), input_ids.size(), 
        input_shape.data(), input_shape.size()));
    
    input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
        allocator.GetInfo(), attention_mask.data(), attention_mask.size(),
        input_shape.data(), input_shape.size()));
    
    // Prepare output tensors
    vector<string> output_name_strings;
    for (size_t i = 0; i < num_outputs; i++) {
        auto output_name = session->GetOutputNameAllocated(i, allocator);
        output_name_strings.push_back(string(output_name.get()));
    }
    
    // Convert to const char* for ONNX Runtime
    vector<const char*> output_names;
    for (const auto& name : output_name_strings) {
        output_names.push_back(name.c_str());
    }
    
    // Allocate output tensors based on expected shapes
    vector<Ort::Value> output_tensors;
    
    // Dense embeddings: [1, 1024]
    vector<int64_t> dense_shape = {1, 1024};
    vector<float> dense_output(1024);
    output_tensors.push_back(Ort::Value::CreateTensor<float>(
        allocator.GetInfo(), dense_output.data(), dense_output.size(),
        dense_shape.data(), dense_shape.size()));
    
    // Sparse weights: [1, seq_len, 1]
    vector<int64_t> sparse_shape = {1, static_cast<int64_t>(input_ids.size()), 1};
    vector<float> sparse_output(input_ids.size());
    output_tensors.push_back(Ort::Value::CreateTensor<float>(
        allocator.GetInfo(), sparse_output.data(), sparse_output.size(),
        sparse_shape.data(), sparse_shape.size()));
    
    // ColBERT embeddings: [1, seq_len, 1024]
    vector<int64_t> colbert_shape = {1, static_cast<int64_t>(input_ids.size()), 1024};
    vector<float> colbert_output(input_ids.size() * 1024);
    output_tensors.push_back(Ort::Value::CreateTensor<float>(
        allocator.GetInfo(), colbert_output.data(), colbert_output.size(),
        colbert_shape.data(), colbert_shape.size()));
    
    // Run inference
    cout << "\n🚀 Running inference..." << endl;
    try {
        session->Run(Ort::RunOptions{nullptr}, 
                     input_names.data(), input_tensors.data(), input_tensors.size(),
                     output_names.data(), output_tensors.data(), output_tensors.size());
        
        cout << "✅ Inference successful!" << endl;
        
        // Print output shapes and sample values
        for (size_t i = 0; i < output_tensors.size(); i++) {
            auto tensor_info = output_tensors[i].GetTensorTypeAndShapeInfo();
            auto shape = tensor_info.GetShape();
            auto element_type = tensor_info.GetElementType();
            
            cout << "\n📊 Output " << i << " (" << output_names[i] << "):" << endl;
            cout << "   Shape: [";
            for (size_t j = 0; j < shape.size(); j++) {
                cout << shape[j];
                if (j < shape.size() - 1) cout << ", ";
            }
            cout << "]" << endl;
            cout << "   Type: " << element_type << endl;
            
            // Print sample values for float outputs
            if (element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                float* data = output_tensors[i].GetTensorMutableData<float>();
                size_t num_elements = 1;
                for (auto dim : shape) {
                    if (dim > 0) num_elements *= dim;
                }
                
                cout << "   Sample values: [";
                size_t num_to_show = min(static_cast<size_t>(5), num_elements);
                for (size_t j = 0; j < num_to_show; j++) {
                    cout << data[j];
                    if (j < num_to_show - 1) cout << ", ";
                }
                if (num_elements > num_to_show) cout << ", ...";
                cout << "]" << endl;
            }
        }
        
        cout << "\n🎉 Test completed successfully!" << endl;
        cout << "   Model loaded and inference executed without errors." << endl;
        cout << "   Input shape: [" << input_shape[0] << ", " << input_shape[1] << "]" << endl;
        cout << "   Expected outputs: " << num_outputs << " tensors" << endl;
        
    } catch (const Ort::Exception& e) {
        cerr << "❌ Inference failed: " << e.what() << endl;
        return 1;
    }
    
    return 0;
} 