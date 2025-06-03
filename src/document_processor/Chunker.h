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
    string readTextFile(const string& filePath);
    string cleanText(const string& text);
    vector<string> splitTextIntoChunks(const string& text);

private:
    DocumentProcessingConfig config;
    
    // Helper methods
    string generateChunkId(const string& sourceFile, int chunkIndex);
    size_t estimateTokenCount(const string& text);
    string extractMetadata(const string& sourceFile);
    string cleanChunk(const string& chunk);
    vector<size_t> findSentenceBoundaries(const string& text);
    vector<size_t> findParagraphBoundaries(const string& text);
    
    // 🆕 PDF PROCESSING METHODS:
    string extractTextFromPdf(const string& filePath);
    string extractTextFromPdfWithOCR(const string& filePath);
    string getPdfInfo(const string& filePath);
    string cleanPdfText(const string& text);
    string tryAlternativeExtraction(const string& filePath);
    string extractWithRawMode(const string& filePath);
    string extractPageByPage(const string& filePath);
    int getPdfPageCount(const string& filePath);
    
    // PDF structure detection
    struct PdfSection {
        string title;
        size_t start_position;
        size_t end_position;
        int level;
    };
    
    vector<PdfSection> detectPdfSections(const string& text);
    vector<size_t> detectPageBreaks(const string& text);
    size_t findOptimalChunkEnd(const string& text, size_t start, const vector<size_t>& boundaries, size_t targetSize);
    string extractPdfChunkMetadata(const string& content, const vector<PdfSection>& sections, const vector<size_t>& pages, size_t position);
};

#endif // CHUNKER_H