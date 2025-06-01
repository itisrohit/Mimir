#include "Chunker.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <iostream>
#include <sys/stat.h>

DocumentProcessor::DocumentProcessor() {
    // Loading configuration from existing ConfigManager
    updateConfig();
}

void DocumentProcessor::updateConfig() {
    auto& configManager = ConfigManager::getInstance();
    config = configManager.getDocumentProcessingConfig();
}

void DocumentProcessor::printConfig() const {
    cout << "\n📋 DOCUMENT PROCESSOR CONFIGURATION:\n";
    cout << "Chunk Size: " << config.chunk_size << " characters\n";
    cout << "Chunk Overlap: " << config.chunk_overlap << " characters\n";
    cout << "Preserve Sentences: " << (config.preserve_sentences ? "Yes" : "No") << "\n";
    cout << "Preserve Paragraphs: " << (config.preserve_paragraphs ? "Yes" : "No") << "\n";
    cout << "Max File Size: " << config.max_file_size_mb << " MB\n";
    cout << "\n";
}

vector<TextChunk> DocumentProcessor::processDocument(const string& filePath) {
    string fileType = detectFileType(filePath);
    
    cout << "📄 Processing " << fileType << " file: " << filePath << "\n";
    
    if (fileType == "txt" || fileType == "text") {
        return processTxtFile(filePath);
    } else if (fileType == "pdf") {
        return processPdfFile(filePath);
    } else if (fileType == "md" || fileType == "markdown") {
        return processMarkdownFile(filePath);
    } else {
        cout << "⚠️  Unsupported file type: " << fileType << ". Treating as text.\n";
        return processTxtFile(filePath);
    }
}

vector<TextChunk> DocumentProcessor::processTxtFile(const string& filePath) {
    string content = readTextFile(filePath);
    if (content.empty()) {
        cout << "❌ Failed to read file or file is empty.\n";
        return {};
    }
    
    content = cleanText(content);
    return chunkText(content, filePath);
}

vector<TextChunk> DocumentProcessor::processPdfFile(const string& filePath) {
    // For now, we'll implement a basic PDF text extraction
    // In a production system, you'd use libraries like poppler or pdfium
    cout << "⚠️  PDF processing not fully implemented. Using placeholder.\n";
    
    // Placeholder: Create a dummy chunk indicating PDF processing needed
    TextChunk chunk;
    chunk.id = generateChunkId(filePath, 0);
    chunk.content = "[PDF Document] " + filePath + " - Full PDF processing requires external library integration.";
    chunk.source_file = filePath;
    chunk.chunk_index = 0;
    chunk.start_position = 0;
    chunk.end_position = chunk.content.length();
    chunk.token_count = estimateTokenCount(chunk.content);
    chunk.metadata = "type:pdf,status:placeholder";
    
    return {chunk};
}

vector<TextChunk> DocumentProcessor::processMarkdownFile(const string& filePath) {
    string content = readTextFile(filePath);
    if (content.empty()) {
        return {};
    }
    
    // Clean markdown syntax for better chunking
    content = cleanText(content);
    
    // TODO: Implement markdown-specific processing (preserve headers, etc.)
    return chunkText(content, filePath);
}

vector<TextChunk> DocumentProcessor::chunkText(const string& text, const string& sourceFile) {
    vector<TextChunk> chunks;
    vector<string> textChunks = splitTextIntoChunks(text);
    
    cout << "📊 Created " << textChunks.size() << " chunks from " << sourceFile << "\n";
    
    for (size_t i = 0; i < textChunks.size(); ++i) {
        TextChunk chunk;
        chunk.id = generateChunkId(sourceFile, i);
        chunk.content = textChunks[i];
        chunk.source_file = sourceFile;
        chunk.chunk_index = i;
        chunk.start_position = 0; // TODO: Calculate actual positions
        chunk.end_position = chunk.content.length();
        chunk.token_count = estimateTokenCount(chunk.content);
        chunk.metadata = extractMetadata(sourceFile);
        
        chunks.push_back(chunk);
    }
    
    return chunks;
}

string DocumentProcessor::detectFileType(const string& filePath) {
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot == string::npos) {
        return "unknown";
    }
    
    string extension = filePath.substr(lastDot + 1);
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "txt" || extension == "text") return "txt";
    if (extension == "pdf") return "pdf";
    if (extension == "md" || extension == "markdown") return "md";
    if (extension == "csv") return "csv";
    if (extension == "json") return "json";
    if (extension == "xml") return "xml";
    if (extension == "html" || extension == "htm") return "html";
    
    return "unknown";
}

string DocumentProcessor::readTextFile(const string& filePath) {
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        cout << "❌ Cannot open file: " << filePath << "\n";
        return "";
    }
    
    // Get file size
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);
    
    // Use config max file size
    size_t maxSize = config.max_file_size_mb * 1024 * 1024;
    if (fileSize > maxSize) {
        cout << "⚠️  File too large (>" << config.max_file_size_mb << "MB): " << filePath << "\n";
        return "";
    }
    
    string content;
    content.resize(fileSize);
    file.read(&content[0], fileSize);
    file.close();
    
    return content;
}

string DocumentProcessor::cleanText(const string& text) {
    string cleaned = text;
    
    if (config.remove_extra_whitespace) {
        // Remove excessive whitespace
        regex multipleSpaces("  +");
        cleaned = regex_replace(cleaned, multipleSpaces, " ");
        
        // Remove excessive newlines
        regex multipleNewlines("\n\n\n+");
        cleaned = regex_replace(cleaned, multipleNewlines, "\n\n");
        
        // Remove carriage returns
        regex carriageReturns("\r");
        cleaned = regex_replace(cleaned, carriageReturns, "");
    }
    
    // Trim leading/trailing whitespace
    size_t start = cleaned.find_first_not_of(" \t\n");
    if (start == string::npos) return "";
    
    size_t end = cleaned.find_last_not_of(" \t\n");
    cleaned = cleaned.substr(start, end - start + 1);
    
    return cleaned;
}

vector<string> DocumentProcessor::splitTextIntoChunks(const string& text) {
    vector<string> chunks;
    
    if (text.length() <= config.chunk_size) {
        chunks.push_back(text);
        return chunks;
    }
    
    size_t start = 0;
    
    while (start < text.length()) {
        size_t end = start + config.chunk_size;
        
        // If we're not at the end, try to find a good break point
        if (end < text.length()) {
            // Try to break at separators in order of preference
            size_t bestBreak = end;
            
            for (const string& separator : config.separators) {
                size_t breakPoint = text.rfind(separator, end);
                if (breakPoint != string::npos && breakPoint > start) {
                    bestBreak = breakPoint + separator.length();
                    break;
                }
            }
            
            end = bestBreak;
        } else {
            end = text.length();
        }
        
        string chunk = text.substr(start, end - start);
        chunks.push_back(chunk);
        
        // Calculate next start position with overlap
        if (end < text.length()) {
            start = (end > config.chunk_overlap) ? end - config.chunk_overlap : end;
        } else {
            break;
        }
    }
    
    return chunks;
}

string DocumentProcessor::generateChunkId(const string& sourceFile, int chunkIndex) {
    // Simple ID generation - in production, use UUID or hash
    size_t fileHash = hash<string>{}(sourceFile);
    return "chunk_" + to_string(fileHash) + "_" + to_string(chunkIndex);
}

size_t DocumentProcessor::estimateTokenCount(const string& text) {
    // Rough token estimation: average 4 characters per token
    return text.length() / 4;
}

string DocumentProcessor::extractMetadata(const string& filePath) {
    stringstream metadata;
    
    // Get file stats
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == 0) {
        metadata << "size:" << fileStat.st_size;
        metadata << ",modified:" << fileStat.st_mtime;
    }
    
    metadata << ",type:" << detectFileType(filePath);
    
    return metadata.str();
}

vector<size_t> DocumentProcessor::findSentenceBoundaries(const string& text) {
    vector<size_t> boundaries;
    regex sentenceEnd("[.!?]\\s+");
    
    sregex_iterator iter(text.begin(), text.end(), sentenceEnd);
    sregex_iterator end;
    
    for (; iter != end; ++iter) {
        boundaries.push_back(iter->position() + iter->length());
    }
    
    return boundaries;
}

vector<size_t> DocumentProcessor::findParagraphBoundaries(const string& text) {
    vector<size_t> boundaries;
    regex paragraphEnd("\n\n+");
    
    sregex_iterator iter(text.begin(), text.end(), paragraphEnd);
    sregex_iterator end;
    
    for (; iter != end; ++iter) {
        boundaries.push_back(iter->position() + iter->length());
    }
    
    return boundaries;
}