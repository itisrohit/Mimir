#ifndef CHUNKER_H
#define CHUNKER_H

#include <string>
#include <vector>
#include <memory>
#include "../config/ConfigManager.h"

using namespace std;

struct TextChunk {
    string id;
    string content;
    string source_file;
    int chunk_index;
    size_t start_position;
    size_t end_position;
    size_t token_count;
    string metadata;
};

class DocumentProcessor {
public:
    DocumentProcessor(); 
    // Main processing method
    vector<TextChunk> processDocument(const string& filePath);
    
    // Configuration methods
    void updateConfig();  // Reload config settings
    void printConfig() const;
    
    // File type specific processors
    vector<TextChunk> processTxtFile(const string& filePath);
    vector<TextChunk> processPdfFile(const string& filePath);
    vector<TextChunk> processMarkdownFile(const string& filePath);
    
    // Text chunking methods
    vector<TextChunk> chunkText(const string& text, const string& sourceFile);
    
    // Utility methods
    string detectFileType(const string& filePath);
    string extractTextFromPdf(const string& filePath);
    string readTextFile(const string& filePath);
    string cleanText(const string& text);
    vector<string> splitTextIntoChunks(const string& text);

private:
    DocumentProcessingConfig config;  // Use your existing config struct
    
    // Helper methods
    string generateChunkId(const string& sourceFile, int chunkIndex);
    size_t estimateTokenCount(const string& text);
    vector<size_t> findSentenceBoundaries(const string& text);
    vector<size_t> findParagraphBoundaries(const string& text);
    string extractMetadata(const string& filePath);
};

#endif // CHUNKER_H