#include "Chunker.h"
#include "../config/ConfigManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <sys/stat.h>
#include <cstdlib>
#include <ctime>

using namespace std;

DocumentProcessor::DocumentProcessor() {
    // Loading configuration from existing ConfigManager
    auto& configManager = ConfigManager::getInstance();
    config = configManager.getDocumentProcessingConfig();
    
    cout << "✅ DocumentProcessor initialized with configuration:" << endl;
    cout << "   Chunk Size: " << config.chunk_size << " chars" << endl;
    cout << "   Chunk Overlap: " << config.chunk_overlap << " chars" << endl;
    cout << "   Preserve Sentences: " << (config.preserve_sentences ? "enabled" : "disabled") << endl;
    cout << "   Preserve Paragraphs: " << (config.preserve_paragraphs ? "enabled" : "disabled") << endl;
    cout << "   Separators: " << config.separators.size() << " configured" << endl;
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

// 🆕 PRODUCTION PDF PROCESSING
vector<TextChunk> DocumentProcessor::processPdfFile(const string& filePath) {
    cout << "📄 Processing PDF file with production-level extraction: " << filePath << "\n";
    
    // Extract text from PDF using Poppler/pdftotext
    string extractedText = extractTextFromPdf(filePath);
    
    if (extractedText.empty()) {
        cout << "❌ Failed to extract text from PDF or PDF is empty/scanned.\n";
        
        // Try OCR for scanned PDFs
        cout << "🔍 Attempting OCR extraction...\n";
        extractedText = extractTextFromPdfWithOCR(filePath);
        
        if (extractedText.empty()) {
            cout << "❌ OCR extraction also failed.\n";
            
            // Final fallback - create placeholder chunk
            TextChunk chunk;
            chunk.id = generateChunkId(filePath, 0);
            chunk.content = "[PDF Document] " + filePath + " - Could not extract text. May be encrypted or image-only PDF.";
            chunk.source_file = filePath;
            chunk.chunk_index = 0;
            chunk.start_position = 0;
            chunk.end_position = chunk.content.length();
            chunk.token_count = estimateTokenCount(chunk.content);
            chunk.metadata = "type:pdf,status:extraction_failed";
            
            return {chunk};
        } else {
            cout << "✅ OCR extraction successful!\n";
        }
    }
    
    // Clean and normalize extracted text
    extractedText = cleanPdfText(extractedText);
    
    if (extractedText.length() < 50) {
        cout << "⚠️  Extracted text too short, might be a scanned/image PDF.\n";
        return {};
    }
    
    cout << "📊 Extracted " << extractedText.length() << " characters from PDF\n";
    
    // Use your existing excellent chunking system
    return chunkText(extractedText, filePath);
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
    
    size_t currentPosition = 0;
    
    for (size_t i = 0; i < textChunks.size(); ++i) {
        TextChunk chunk;
        chunk.id = generateChunkId(sourceFile, i);
        chunk.content = textChunks[i];
        chunk.source_file = sourceFile;
        chunk.chunk_index = i;
        
        // Calculate actual positions in original text
        size_t chunkStart = text.find(chunk.content, currentPosition);
        if (chunkStart != string::npos) {
            chunk.start_position = chunkStart;
            chunk.end_position = chunkStart + chunk.content.length();
            currentPosition = chunkStart + chunk.content.length();
        } else {
            // Fallback if exact match not found
            chunk.start_position = currentPosition;
            chunk.end_position = currentPosition + chunk.content.length();
            currentPosition = chunk.end_position;
        }
        
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
        size_t end = min(start + config.chunk_size, text.length());
        
        // If we're not at the end of text, find a good break point
        if (end < text.length()) {
            size_t bestBreak = end;
            
            // Try to break at separators in order of preference
            for (const string& separator : config.separators) {
                size_t breakPoint = text.rfind(separator, end);
                if (breakPoint != string::npos && breakPoint > start) {
                    bestBreak = breakPoint + separator.length();
                    break;
                }
            }
            
            // If no separator found, try to break at word boundary
            if (bestBreak == end) {
                size_t spacePos = text.rfind(' ', end);
                if (spacePos != string::npos && spacePos > start) {
                    bestBreak = spacePos + 1;
                }
            }
            
            end = bestBreak;
        }
        
        string chunk = text.substr(start, end - start);
        
        // Clean up the chunk
        chunk = cleanChunk(chunk);
        
        if (!chunk.empty()) {
            chunks.push_back(chunk);
        }
        
        // Calculate next start position with proper overlap
        if (end >= text.length()) {
            break;
        }
        
        // Apply overlap: move back by overlap amount, but ensure we make progress
        size_t overlap = min(config.chunk_overlap, end - start - 1);
        size_t tentativeStart = end - overlap;

        // Adjust overlap start to word boundary to avoid cut-off words
        if (tentativeStart > 0 && tentativeStart < text.length()) {
            // Find the start of the word at tentativeStart
            while (tentativeStart > start && 
                   text[tentativeStart] != ' ' && 
                   text[tentativeStart] != '\n' && 
                   text[tentativeStart] != '.' &&
                   text[tentativeStart] != '!' &&
                   text[tentativeStart] != '?' &&
                   text[tentativeStart] != '\t') {
                tentativeStart--;
            }
            
            // Skip the space/punctuation to start at beginning of word
            if (tentativeStart < text.length() && 
                (text[tentativeStart] == ' ' || 
                 text[tentativeStart] == '\n' || 
                 text[tentativeStart] == '\t')) {
                tentativeStart++;
            }
            
            // Additional check: if we're in the middle of a sentence, 
            // try to find a sentence boundary for cleaner overlap
            if (tentativeStart > start + 50) { // Only if we have room to look back
                size_t sentenceStart = tentativeStart;
                
                // Look for sentence boundaries within reasonable distance
                for (size_t lookBack = tentativeStart; 
                     lookBack > max(start, tentativeStart - 100) && lookBack > 0; 
                     lookBack--) {
                    if ((text[lookBack] == '.' || text[lookBack] == '!' || text[lookBack] == '?') &&
                        lookBack + 1 < text.length() && 
                        (text[lookBack + 1] == ' ' || text[lookBack + 1] == '\n')) {
                        sentenceStart = lookBack + 2; // Start after ". "
                        break;
                    }
                }
                
                // Use sentence boundary if it's reasonable
                if (sentenceStart > start && sentenceStart < tentativeStart + 50) {
                    tentativeStart = sentenceStart;
                }
            }
        }

        start = tentativeStart;

        // Ensure we don't get stuck in infinite loop
        if (start >= end) {
            start = end;
        }
    }
    
    return chunks;
}

string DocumentProcessor::cleanChunk(const string& chunk) {
    if (chunk.empty()) return "";
    
    string cleaned = chunk;
    
    // Remove leading/trailing whitespace
    size_t start = cleaned.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    
    size_t end = cleaned.find_last_not_of(" \t\n\r");
    cleaned = cleaned.substr(start, end - start + 1);
    
    // Fix common chunking artifacts
    
    // Remove orphaned punctuation at the start
    if (!cleaned.empty() && 
        (cleaned[0] == '.' || cleaned[0] == ',' || cleaned[0] == ';' || 
         cleaned[0] == ':' || cleaned[0] == ')' || cleaned[0] == ']')) {
        cleaned = cleaned.substr(1);
        // Remove any whitespace after removing punctuation
        size_t newStart = cleaned.find_first_not_of(" \t");
        if (newStart != string::npos) {
            cleaned = cleaned.substr(newStart);
        }
    }
    
    // Remove orphaned opening brackets/parentheses at the end
    if (!cleaned.empty() && 
        (cleaned.back() == '(' || cleaned.back() == '[' || cleaned.back() == '{')) {
        cleaned.pop_back();
        // Remove trailing whitespace
        while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t')) {
            cleaned.pop_back();
        }
    }
    
    // Fix incomplete words at the beginning (like "ary tools" -> find full word)
    if (!cleaned.empty() && cleaned.length() > 3) {
        // Check if first word is incomplete (no capital letter, very short)
        size_t firstSpace = cleaned.find(' ');
        if (firstSpace != string::npos && firstSpace < 5) {
            string firstWord = cleaned.substr(0, firstSpace);
            // If first word is very short and lowercase, it might be incomplete
            if (firstWord.length() <= 3 && islower(firstWord[0])) {
                // Skip the incomplete word
                cleaned = cleaned.substr(firstSpace + 1);
            }
        }
    }
    
    return cleaned;
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
    
    for (size_t i = 0; i < text.length() - 1; ++i) {
        char current = text[i];
        char next = text[i + 1];
        
        // Standard sentence endings
        if ((current == '.' || current == '!' || current == '?') && 
            (next == ' ' || next == '\n' || next == '\t')) {
            
            // Avoid common abbreviations
            bool isAbbreviation = false;
            
            // Check for common abbreviations
            if (current == '.' && i >= 2) {
                string prev3 = text.substr(i - 2, 3);
                string prev4 = (i >= 3) ? text.substr(i - 3, 4) : "";
                
                // Common abbreviations
                if (prev3 == "Dr." || prev3 == "Mr." || prev3 == "Ms." || 
                    prev4 == "Prof." || prev3 == "etc." || prev3 == "vs." ||
                    prev3 == "e.g." || prev3 == "i.e.") {
                    isAbbreviation = true;
                }
                
                // Check for single letter abbreviations (A. B. C.)
                if (i >= 1 && text[i-1] != ' ' && isupper(text[i-1])) {
                    isAbbreviation = true;
                }
            }
            
            if (!isAbbreviation) {
                boundaries.push_back(i + 1); // Position after the punctuation
            }
        }
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

// 🆕 PRODUCTION PDF METHODS

string DocumentProcessor::extractTextFromPdf(const string& filePath) {
    // First, check PDF info to understand the document
    string pdfInfo = getPdfInfo(filePath);
    cout << "📋 PDF Info: " << pdfInfo << "\n";
    
    // Create temporary file for extracted text
    string tempFile = "/tmp/mimir_pdf_" + to_string(time(nullptr)) + ".txt";
    
    // Build pdftotext command with optimized settings
    stringstream command;
    command << "pdftotext ";
    command << "-layout ";           // Preserve layout
    command << "-nopgbrk ";         // No page breaks
    command << "-enc UTF-8 ";       // UTF-8 encoding
    command << "-eol unix ";        // Unix line endings
    command << "-q ";               // Quiet mode
    command << "\"" << filePath << "\" ";
    command << "\"" << tempFile << "\" ";
    command << "2>/dev/null";       // Suppress errors
    
    cout << "🔧 Running Poppler pdftotext: " << command.str() << "\n";
    
    int result = system(command.str().c_str());
    
    if (result != 0) {
        cout << "❌ pdftotext failed with code: " << result << "\n";
        remove(tempFile.c_str());
        return tryAlternativeExtraction(filePath);
    }
    
    // Read extracted text
    string content = readTextFile(tempFile);
    
    // Clean up temporary file
    remove(tempFile.c_str());
    
    return content;
}

string DocumentProcessor::extractTextFromPdfWithOCR(const string& filePath) {
    cout << "🔍 Starting OCR extraction (this may take a while)...\n";
    
    // Create temporary directory for OCR processing
    string tempDir = "/tmp/mimir_ocr_" + to_string(time(nullptr));
    string mkdirCmd = "mkdir -p \"" + tempDir + "\"";
    system(mkdirCmd.c_str());
    
    // Convert PDF pages to images
    stringstream convertCmd;
    convertCmd << "pdftoppm ";
    convertCmd << "-png ";                    // PNG format
    convertCmd << "-r 300 ";                  // 300 DPI for good OCR
    convertCmd << "-gray ";                   // Grayscale for faster processing
    convertCmd << "\"" << filePath << "\" ";
    convertCmd << "\"" << tempDir << "/page\" ";
    convertCmd << "2>/dev/null";
    
    cout << "🔧 Converting PDF to images...\n";
    int convertResult = system(convertCmd.str().c_str());
    
    if (convertResult != 0) {
        cout << "❌ PDF to image conversion failed\n";
        string cleanupCmd = "rm -rf \"" + tempDir + "\"";
        system(cleanupCmd.c_str());
        return "";
    }
    
    // Run OCR on each image
    string combinedText;
    int pageCount = 0;
    
    // Find all generated image files
    string listCmd = "ls \"" + tempDir + "\"/*.png 2>/dev/null | sort";
    FILE* pipe = popen(listCmd.c_str(), "r");
    
    if (pipe) {
        char buffer[512];
        vector<string> imageFiles;
        
        while (fgets(buffer, sizeof(buffer), pipe)) {
            string filename = buffer;
            filename.erase(filename.find_last_not_of(" \n\r\t") + 1);
            imageFiles.push_back(filename);
        }
        pclose(pipe);
        
        for (const string& imageFile : imageFiles) {
            pageCount++;
            cout << "🔍 OCR processing page " << pageCount << "...\n";
            
            string ocrTempFile = tempDir + "/ocr_output_" + to_string(pageCount) + ".txt";
            
            stringstream ocrCmd;
            ocrCmd << "tesseract ";
            ocrCmd << "\"" << imageFile << "\" ";
            ocrCmd << "\"" << ocrTempFile.substr(0, ocrTempFile.find_last_of('.')) << "\" ";
            ocrCmd << "-l eng ";                    // English language
            ocrCmd << "--psm 1 ";                   // Automatic page segmentation with OSD
            ocrCmd << "--oem 3 ";                   // Default OCR Engine Mode
            ocrCmd << "2>/dev/null";                // Suppress tesseract warnings
            
            int ocrResult = system(ocrCmd.str().c_str());
            
            if (ocrResult == 0) {
                string pageText = readTextFile(ocrTempFile);
                if (!pageText.empty()) {
                    combinedText += pageText + "\n\n--- Page " + to_string(pageCount) + " ---\n\n";
                }
            }
        }
    }
    
    // Clean up temporary directory
    string cleanupCmd = "rm -rf \"" + tempDir + "\"";
    system(cleanupCmd.c_str());
    
    if (combinedText.empty()) {
        cout << "❌ OCR extraction produced no text\n";
    } else {
        cout << "✅ OCR extracted " << combinedText.length() << " characters from " << pageCount << " pages\n";
    }
    
    return combinedText;
}

string DocumentProcessor::getPdfInfo(const string& filePath) {
    string tempFile = "/tmp/mimir_pdfinfo_" + to_string(time(nullptr)) + ".txt";
    
    stringstream command;
    command << "pdfinfo \"" << filePath << "\" > \"" << tempFile << "\" 2>/dev/null";
    
    int result = system(command.str().c_str());
    
    if (result != 0) {
        return "PDF info extraction failed";
    }
    
    string info = readTextFile(tempFile);
    remove(tempFile.c_str());
    
    // Extract key information
    stringstream summary;
    istringstream infoStream(info);
    string line;
    
    while (getline(infoStream, line)) {
        if (line.find("Pages:") == 0 || 
            line.find("Title:") == 0 || 
            line.find("Author:") == 0 ||
            line.find("Creator:") == 0 ||
            line.find("Encrypted:") == 0) {
            summary << line << " ";
        }
    }
    
    return summary.str();
}

string DocumentProcessor::cleanPdfText(const string& text) {
    string cleaned = text;
    
    // Remove PDF-specific artifacts
    
    // 1. Fix hyphenated words across lines
    regex hyphenNewline("-\\s*\\n\\s*");
    cleaned = regex_replace(cleaned, hyphenNewline, "");
    
    // 2. Fix excessive spacing from layout preservation
    regex excessiveSpaces("  +");
    cleaned = regex_replace(cleaned, excessiveSpaces, " ");
    
    // 3. Remove page numbers and headers/footers (common patterns)
    regex pageNumbers("\\n\\s*\\d+\\s*\\n");
    cleaned = regex_replace(cleaned, pageNumbers, "\n\n");
    
    // 4. Fix broken sentences from column layouts
    regex brokenSentences("([.!?])\\s*\\n\\s*([a-z])");
    cleaned = regex_replace(cleaned, brokenSentences, "$1 $2");
    
    // 5. Remove OCR artifacts
    regex ocrArtifacts("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F\\x7F]");
    cleaned = regex_replace(cleaned, ocrArtifacts, "");
    
    // 6. Clean up multiple newlines
    regex multipleNewlines("\\n\\n\\n+");
    cleaned = regex_replace(cleaned, multipleNewlines, "\n\n");
    
    // 7. Trim whitespace
    cleaned = regex_replace(cleaned, regex("^\\s+|\\s+$"), "");
    
    return cleaned;
}

string DocumentProcessor::tryAlternativeExtraction(const string& filePath) {
    cout << "🔄 Trying alternative Poppler extraction methods...\n";
    
    // Method 1: Raw text extraction (faster, less formatting)
    string rawText = extractWithRawMode(filePath);
    if (!rawText.empty()) {
        cout << "✅ Raw text extraction successful\n";
        return rawText;
    }
    
    // Method 2: Page-by-page extraction
    string pageByPageText = extractPageByPage(filePath);
    if (!pageByPageText.empty()) {
        cout << "✅ Page-by-page extraction successful\n";
        return pageByPageText;
    }
    
    // Method 3: OCR fallback
    cout << "🔍 All Poppler methods failed, falling back to OCR...\n";
    return extractTextFromPdfWithOCR(filePath);
}

string DocumentProcessor::extractWithRawMode(const string& filePath) {
    string tempFile = "/tmp/mimir_pdf_raw_" + to_string(time(nullptr)) + ".txt";
    
    stringstream command;
    command << "pdftotext -raw ";   // Raw text mode (no layout)
    command << "-enc UTF-8 ";
    command << "\"" << filePath << "\" ";
    command << "\"" << tempFile << "\" ";
    command << "2>/dev/null";
    
    int result = system(command.str().c_str());
    
    if (result == 0) {
        string content = readTextFile(tempFile);
        remove(tempFile.c_str());
        return content;
    }
    
    return "";
}

string DocumentProcessor::extractPageByPage(const string& filePath) {
    // Get page count first
    int pageCount = getPdfPageCount(filePath);
    if (pageCount <= 0) return "";
    
    cout << "📄 Extracting " << pageCount << " pages individually...\n";
    
    string combinedText;
    
    for (int page = 1; page <= pageCount; ++page) {
        string tempFile = "/tmp/mimir_page_" + to_string(page) + "_" + to_string(time(nullptr)) + ".txt";
        
        stringstream command;
        command << "pdftotext ";
        command << "-f " << page << " ";    // From page
        command << "-l " << page << " ";    // To page
        command << "-layout ";
        command << "\"" << filePath << "\" ";
        command << "\"" << tempFile << "\" ";
        command << "2>/dev/null";
        
        int result = system(command.str().c_str());
        
        if (result == 0) {
            string pageText = readTextFile(tempFile);
            if (!pageText.empty()) {
                combinedText += pageText + "\n\n--- Page " + to_string(page) + " ---\n\n";
            }
        }
        
        remove(tempFile.c_str());
        
        // Progress indicator for large documents
        if (page % 10 == 0) {
            cout << "📄 Processed " << page << "/" << pageCount << " pages\n";
        }
    }
    
    return combinedText;
}

int DocumentProcessor::getPdfPageCount(const string& filePath) {
    string tempFile = "/tmp/mimir_info_" + to_string(time(nullptr)) + ".txt";
    
    stringstream command;
    command << "pdfinfo \"" << filePath << "\" | grep 'Pages:' > \"" << tempFile << "\" 2>/dev/null";
    
    int result = system(command.str().c_str());
    
    if (result != 0) {
        return -1;
    }
    
    string info = readTextFile(tempFile);
    remove(tempFile.c_str());
    
    // Parse "Pages: 42"
    regex pagePattern("Pages:\\s*(\\d+)");
    smatch match;
    
    if (regex_search(info, match, pagePattern)) {
        return stoi(match[1]);
    }
    
    return -1;
}

// Helper methods for PDF structure detection (simplified versions)
vector<DocumentProcessor::PdfSection> DocumentProcessor::detectPdfSections(const string& text) {
    vector<PdfSection> sections;
    
    // Basic section detection - you can enhance this later
    regex sectionPattern("^\\s*\\d+\\.\\s+[A-Z][^\\n]{5,50}\\s*$", regex::multiline);
    sregex_iterator iter(text.begin(), text.end(), sectionPattern);
    sregex_iterator end;
    
    for (; iter != end; ++iter) {
        PdfSection section;
        section.title = iter->str();
        section.start_position = iter->position();
        section.level = 1;
        sections.push_back(section);
    }
    
    return sections;
}

vector<size_t> DocumentProcessor::detectPageBreaks(const string& text) {
    vector<size_t> pageBreaks;
    
    regex pageBreakPattern("\\n\\s*---\\s*Page\\s+\\d+\\s*---\\s*\\n");
    sregex_iterator iter(text.begin(), text.end(), pageBreakPattern);
    sregex_iterator end;
    
    for (; iter != end; ++iter) {
        pageBreaks.push_back(iter->position());
    }
    
    return pageBreaks;
}

size_t DocumentProcessor::findOptimalChunkEnd(const string& text, size_t start, const vector<size_t>& boundaries, size_t targetSize) {
    size_t idealEnd = start + targetSize;
    
    if (idealEnd >= text.length()) {
        return text.length();
    }
    
    // Find the best boundary within reasonable range
    size_t bestEnd = idealEnd;
    size_t minDistance = targetSize;
    
    for (size_t boundary : boundaries) {
        if (boundary <= start) continue;
        if (boundary > start + targetSize * 1.5) break;
        
        size_t distance = (boundary > idealEnd) ? boundary - idealEnd : idealEnd - boundary;
        if (distance < minDistance && boundary - start >= targetSize * 0.5) {
            bestEnd = boundary;
            minDistance = distance;
        }
    }
    
    return bestEnd;
}

string DocumentProcessor::extractPdfChunkMetadata(const string& content, const vector<PdfSection>& sections, const vector<size_t>& pages, size_t position) {
    (void)content;
    stringstream metadata;
    
    // Find which section this chunk belongs to
    string currentSection = "Unknown";
    for (const auto& section : sections) {
        if (section.start_position <= position) {
            currentSection = section.title;
        } else {
            break;
        }
    }
    
    // Find which page this chunk is on
    int currentPage = 1;
    for (size_t pageBreak : pages) {
        if (pageBreak <= position) {
            currentPage++;
        } else {
            break;
        }
    }
    
    metadata << "section:" << currentSection << ";page:" << currentPage;
    return metadata.str();
}