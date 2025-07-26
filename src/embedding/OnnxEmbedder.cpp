#include "OnnxEmbedder.h"
#include "../config/ConfigManager.h"
#include <iostream>
#include <chrono>
#include <mutex>

using namespace std;
using namespace std::chrono;

// Global model cache to avoid reloading
static shared_ptr<Ort::Session> cachedModelSession = nullptr;
static shared_ptr<SentencePieceTokenizer> cachedTokenizer = nullptr;
static mutex modelCacheMutex;
static bool modelCacheInitialized = false;

OnnxEmbedder::OnnxEmbedder(const string& tokenizerPath, const string& modelPath)
    : env(ORT_LOGGING_LEVEL_ERROR, "OnnxEmbedder")  // Change from WARNING to ERROR
{
    loadSessions(tokenizerPath, modelPath);
}

void OnnxEmbedder::loadSessions(const string& tokenizerPath, const string& modelPath) {
    // Load configuration
    auto& config = ConfigManager::getInstance();
    auto embeddingConfig = config.getEmbeddingConfig();
    
    // Check if models are already cached
    {
        lock_guard<mutex> lock(modelCacheMutex);
        if (modelCacheInitialized && cachedModelSession && cachedTokenizer) {
            // Use cached models
            modelSession = cachedModelSession;
            tokenizer = cachedTokenizer;
            embeddingDim = embeddingConfig.dim;
            
            cout << "✅ Using cached models (fast initialization)" << endl;
            cout << "   Tokenizer: " << embeddingConfig.tokenizer.type << endl;
            cout << "   Model: " << modelPath << endl;
            cout << "   Dimension: " << embeddingDim << endl;
            return;
        }
    }
    
    // Load models for the first time
    cout << "🔄 Loading models (first time initialization)..." << endl;
    
    // Load SentencePiece tokenizer
    tokenizer = make_shared<SentencePieceTokenizer>(tokenizerPath);
    
    // Configure ONNX session options
    Ort::SessionOptions modelSessionOptions;
    
    // Apply ONNX configuration from config
    modelSessionOptions.SetIntraOpNumThreads(embeddingConfig.max_threads);
    modelSessionOptions.SetInterOpNumThreads(embeddingConfig.max_threads);
    modelSessionOptions.SetGraphOptimizationLevel(static_cast<GraphOptimizationLevel>(embeddingConfig.onnx.optimization_level));
    
    if (embeddingConfig.onnx.enable_mem_pattern) {
        modelSessionOptions.EnableMemPattern();
    }
    if (embeddingConfig.onnx.enable_cpu_mem_arena) {
        modelSessionOptions.EnableCpuMemArena();
    }
    
    // Load ONNX model
    modelSession = make_shared<Ort::Session>(env, modelPath.c_str(), modelSessionOptions);
    embeddingDim = embeddingConfig.dim;
    
    // Cache the models for future use
    {
        lock_guard<mutex> lock(modelCacheMutex);
        cachedModelSession = modelSession;
        cachedTokenizer = tokenizer;
        modelCacheInitialized = true;
    }
    
    cout << "✅ SentencePiece tokenizer and ONNX embedding model loaded successfully" << endl;
    cout << "   Tokenizer: " << embeddingConfig.tokenizer.type << endl;
    cout << "   Model: " << modelPath << endl;
    cout << "   Dimension: " << embeddingDim << endl;
    cout << "   ONNX Optimization: Level " << embeddingConfig.onnx.optimization_level << endl;
}

vector<vector<float>> OnnxEmbedder::embed(const vector<string>& texts) {
    if (!tokenizer || !modelSession) {
        cerr << "❌ Tokenizer or model not loaded" << endl;
        return {};
    }
    
    if (texts.empty()) return {};
    
    // Load configuration for batch processing
    auto& config = ConfigManager::getInstance();
    auto embeddingConfig = config.getEmbeddingConfig();
    auto performanceConfig = config.getPerformanceConfig();
    
    vector<vector<float>> embeddings;
    embeddings.reserve(texts.size());
    
    // Process in batches if configured
    size_t batchSize = embeddingConfig.batch_size;
    if (performanceConfig.batch_processing && texts.size() > batchSize) {
        cout << "🔄 Processing " << texts.size() << " texts in batches of " << batchSize << endl;
        
        for (size_t i = 0; i < texts.size(); i += batchSize) {
            size_t end = min(i + batchSize, texts.size());
            vector<string> batch(texts.begin() + i, texts.begin() + end);
            
            auto batchEmbeddings = processBatch(batch);
            embeddings.insert(embeddings.end(), 
                           std::make_move_iterator(batchEmbeddings.begin()),
                           std::make_move_iterator(batchEmbeddings.end()));
        }
    } else {
        embeddings = processBatch(texts);
    }
    
    return embeddings;
}

vector<vector<float>> OnnxEmbedder::processBatch(const vector<string>& texts) {
    if (texts.empty()) return {};
    
    auto start = high_resolution_clock::now();
    
    // Pre-allocate embeddings vector
    vector<vector<float>> embeddings;
    embeddings.reserve(texts.size());
    
    // Tokenize all texts using C++ SentencePiece
    vector<vector<int>> tokenizedTexts;
    tokenizedTexts.reserve(texts.size());
    
    auto tokenizeStart = high_resolution_clock::now();
    for (const auto& text : texts) {
        vector<int> tokens = tokenizer->tokenize(text, true);  // Add special tokens
        tokenizedTexts.push_back(std::move(tokens));
    }
    auto tokenizeEnd = high_resolution_clock::now();
    
    // Find max sequence length for padding
    size_t maxLen = 0;
    for (const auto& tokens : tokenizedTexts) {
        maxLen = max(maxLen, tokens.size());
    }
    
    // Pre-allocate vectors to avoid reallocations
    size_t totalTokens = texts.size() * maxLen;
    vector<int64_t> inputIds;
    vector<int64_t> attentionMask;
    inputIds.reserve(totalTokens);
    attentionMask.reserve(totalTokens);
    
    // Pad sequences and create input tensors more efficiently
    for (const auto& tokens : tokenizedTexts) {
        // Add input_ids
        for (int token : tokens) {
            inputIds.push_back(static_cast<int64_t>(token));
        }
        // Pad with 0s
        for (size_t i = tokens.size(); i < maxLen; ++i) {
            inputIds.push_back(0);
        }
        
        // Add attention_mask (1 for real tokens, 0 for padding)
        for (size_t i = 0; i < tokens.size(); ++i) {
            attentionMask.push_back(1);
        }
        for (size_t i = tokens.size(); i < maxLen; ++i) {
            attentionMask.push_back(0);
        }
    }
    
    auto tensorStart = high_resolution_clock::now();
    
    // Create input tensors with optimized memory layout
    vector<int64_t> inputShape = {static_cast<int64_t>(texts.size()), static_cast<int64_t>(maxLen)};
    
    // Use move semantics to avoid copying
    auto inputIdsTensor = Ort::Value::CreateTensor<int64_t>(
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
        inputIds.data(), inputIds.size(), inputShape.data(), inputShape.size()
    );
    
    auto attentionMaskTensor = Ort::Value::CreateTensor<int64_t>(
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
        attentionMask.data(), attentionMask.size(), inputShape.data(), inputShape.size()
    );
    
    auto inferenceStart = high_resolution_clock::now();
    
    // Run embedding model with optimized execution
    vector<const char*> modelInputNames = {"input_ids", "attention_mask"};
    vector<const char*> modelOutputNames = {"dense_vecs"};
    vector<Ort::Value> modelInputs;
    modelInputs.emplace_back(std::move(inputIdsTensor));
    modelInputs.emplace_back(std::move(attentionMaskTensor));
    
    auto modelOutputs = modelSession->Run(
        Ort::RunOptions{nullptr}, 
        modelInputNames.data(), 
        modelInputs.data(), 
        2, 
        modelOutputNames.data(), 
        1
    );
    
    auto inferenceEnd = high_resolution_clock::now();
    
    auto& denseVal = modelOutputs[0];
    const float* denseData = denseVal.GetTensorData<float>();
    
    // Extract embeddings more efficiently
    embeddings.reserve(texts.size());
    for (size_t i = 0; i < texts.size(); ++i) {
        vector<float> emb(denseData + i * embeddingDim, denseData + (i + 1) * embeddingDim);
        embeddings.emplace_back(std::move(emb));
    }
    
    auto end = high_resolution_clock::now();
    
    // Print timing information for debugging
    if (texts.size() > 4) {  // Only show timing for larger batches
        auto tokenizeTime = duration_cast<microseconds>(tokenizeEnd - tokenizeStart).count();
        auto tensorTime = duration_cast<microseconds>(inferenceStart - tensorStart).count();
        auto inferenceTime = duration_cast<microseconds>(inferenceEnd - inferenceStart).count();
        auto totalTime = duration_cast<microseconds>(end - start).count();
        
        cout << "⚡ Performance: " << texts.size() << " texts in " 
             << totalTime << "μs (tokenize: " << tokenizeTime 
             << "μs, tensor: " << tensorTime << "μs, inference: " 
             << inferenceTime << "μs)" << endl;
    }
    
    return embeddings;
} 