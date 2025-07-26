#include "OnnxEmbedder.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;
using namespace std::chrono;

// Read a text file and return its content
string readTextFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "❌ Could not open file: " << filename << endl;
        return "";
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Split text into chunks (simulate document processing)
vector<string> splitIntoChunks(const string& text, size_t chunkSize = 1000) {
    vector<string> chunks;
    size_t start = 0;
    
    while (start < text.length()) {
        size_t end = min(start + chunkSize, text.length());
        chunks.push_back(text.substr(start, end - start));
        start = end;
    }
    
    return chunks;
}

int main() {
    cout << "🚀 Performance Test: C++ Embedding Pipeline" << endl;
    cout << "=============================================" << endl;
    
    // Test document path
    string testDocPath = "test_document.txt";
    
    // Create a test document if it doesn't exist
    ifstream testFile(testDocPath);
    if (!testFile.good()) {
        cout << "📝 Creating test document..." << endl;
        ofstream docFile(testDocPath);
        docFile << "This is a comprehensive test document for measuring embedding performance. ";
        docFile << "It contains multiple sentences with various topics including machine learning, ";
        docFile << "artificial intelligence, natural language processing, and computational linguistics. ";
        docFile << "The document is designed to test the efficiency of the SentencePiece tokenizer ";
        docFile << "combined with the ONNX Runtime embedding model. We will measure the time taken ";
        docFile << "for tokenization, model inference, and the complete pipeline. ";
        docFile << "This test will help us understand the performance characteristics of our ";
        docFile << "pure C++ implementation compared to Python-based solutions. ";
        docFile << "The goal is to achieve high throughput while maintaining accuracy. ";
        docFile << "Performance metrics will include tokenization speed, embedding generation time, ";
        docFile << "and overall pipeline efficiency. This document contains approximately ";
        docFile << "500 words to provide a realistic test scenario for the embedding pipeline. ";
        docFile << "We will measure both single document processing and batch processing capabilities. ";
        docFile << "The test will also evaluate memory usage and computational efficiency. ";
        docFile << "Results will be compared against baseline implementations to ensure optimal performance. ";
        docFile << "This comprehensive evaluation will help identify any bottlenecks in the pipeline. ";
        docFile << "The document structure includes various sentence lengths and complexity levels. ";
        docFile << "This diversity helps ensure robust testing of the embedding system. ";
        docFile << "Performance optimization is crucial for real-world applications. ";
        docFile << "The test will validate both speed and accuracy of the embedding process. ";
        docFile << "End of test document for performance evaluation.";
        docFile.close();
        cout << "✅ Test document created: " << testDocPath << endl;
    }
    
    // Read the test document
    cout << "\n📖 Reading test document..." << endl;
    auto startRead = high_resolution_clock::now();
    string document = readTextFile(testDocPath);
    auto endRead = high_resolution_clock::now();
    auto readTime = duration_cast<microseconds>(endRead - startRead);
    
    if (document.empty()) {
        cerr << "❌ Failed to read test document" << endl;
        return 1;
    }
    
    cout << "📊 Document size: " << document.length() << " characters" << endl;
    cout << "⏱️  Read time: " << readTime.count() << " microseconds" << endl;
    
    // Split into chunks
    cout << "\n✂️  Splitting document into chunks..." << endl;
    auto startSplit = high_resolution_clock::now();
    vector<string> chunks = splitIntoChunks(document, 200); // 200 char chunks
    auto endSplit = high_resolution_clock::now();
    auto splitTime = duration_cast<microseconds>(endSplit - startSplit);
    
    cout << "📊 Number of chunks: " << chunks.size() << endl;
    cout << "⏱️  Split time: " << splitTime.count() << " microseconds" << endl;
    
    // Initialize embedder
    cout << "\n🔧 Initializing embedding pipeline..." << endl;
    auto startInit = high_resolution_clock::now();
    
    string modelDir = "models/bge-m3-onnx/";
    string tokenizerPath = modelDir + "sentencepiece.bpe.model";
    string modelPath = modelDir + "model.onnx";
    
    OnnxEmbedder embedder(tokenizerPath, modelPath);
    
    auto endInit = high_resolution_clock::now();
    auto initTime = duration_cast<milliseconds>(endInit - startInit);
    cout << "⏱️  Initialization time: " << initTime.count() << " milliseconds" << endl;
    
    // Test single chunk embedding
    cout << "\n🎯 Testing single chunk embedding..." << endl;
    auto startSingle = high_resolution_clock::now();
    vector<vector<float>> singleEmbedding = embedder.embed({chunks[0]});
    auto endSingle = high_resolution_clock::now();
    auto singleTime = duration_cast<microseconds>(endSingle - startSingle);
    
    cout << "📊 Single chunk embedding dimension: " << singleEmbedding[0].size() << endl;
    cout << "⏱️  Single chunk time: " << singleTime.count() << " microseconds" << endl;
    
    // Test batch embedding
    cout << "\n📦 Testing batch embedding..." << endl;
    auto startBatch = high_resolution_clock::now();
    vector<vector<float>> batchEmbeddings = embedder.embed(chunks);
    auto endBatch = high_resolution_clock::now();
    auto batchTime = duration_cast<milliseconds>(endBatch - startBatch);
    
    cout << "📊 Batch embeddings generated: " << batchEmbeddings.size() << endl;
    cout << "📊 Batch embedding dimension: " << batchEmbeddings[0].size() << endl;
    cout << "⏱️  Batch time: " << batchTime.count() << " milliseconds" << endl;
    
    // Calculate performance metrics
    cout << "\n📈 Performance Summary" << endl;
    cout << "=====================" << endl;
    
    double avgTimePerChunk = (double)batchTime.count() / chunks.size();
    double throughput = (double)chunks.size() / (batchTime.count() / 1000.0); // chunks per second
    
    cout << "📊 Total chunks processed: " << chunks.size() << endl;
    cout << "📊 Average time per chunk: " << avgTimePerChunk << " milliseconds" << endl;
    cout << "📊 Throughput: " << throughput << " chunks/second" << endl;
    cout << "📊 Total document processing time: " << batchTime.count() << " milliseconds" << endl;
    
    // Memory usage estimation (rough)
    size_t totalEmbeddings = batchEmbeddings.size() * batchEmbeddings[0].size();
    size_t memoryUsage = totalEmbeddings * sizeof(float);
    cout << "📊 Estimated memory usage: " << memoryUsage / 1024 << " KB" << endl;
    
    cout << "\n✅ Performance test completed successfully!" << endl;
    
    return 0;
} 