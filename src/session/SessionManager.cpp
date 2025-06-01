#include "SessionManager.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cerrno> 
#include "../document_processor/Chunker.h"
#include "../config/ConfigManager.h"

using namespace std;

// Helper functions to replace filesystem operations
bool path_exists(const string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool create_directories(const string& path) {
    // Check if path already exists
    if (path_exists(path)) {
        return true;
    }
    
    // Find parent directory
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != string::npos && lastSlash > 0) {
        string parent = path.substr(0, lastSlash);
        // Recursively create parent directories
        if (!create_directories(parent)) {
            return false;
        }
    }
    
    // Create this directory
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool is_directory(const string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) return false;
    return S_ISDIR(buffer.st_mode);
}

vector<string> list_directory(const string& path) {
    vector<string> entries;
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) return entries;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name != "." && name != "..") {
            entries.push_back(name);
        }
    }
    closedir(dir);
    return entries;
}

bool remove_directory_recursive(const string& path) {
    vector<string> entries = list_directory(path);
    
    // Remove all files and subdirectories first
    for (const string& entry : entries) {
        string fullPath = path + "/" + entry;
        if (is_directory(fullPath)) {
            // Recursively remove subdirectory
            if (!remove_directory_recursive(fullPath)) {
                return false;
            }
        } else {
            // Remove file
            if (unlink(fullPath.c_str()) != 0) {
                return false;
            }
        }
    }
    
    // Finally remove the directory itself
    return rmdir(path.c_str()) == 0;
}

SessionManager::SessionManager(const string &basePath)
{
    // Use config if basePath is empty, otherwise use provided path
    if (basePath.empty()) {
        auto& config = ConfigManager::getInstance();
        baseSessionPath = config.getPathsConfig().sessions_dir;
    } else {
        baseSessionPath = basePath;
    }
    
    // Create base session directory if it doesn't exist
    if (!path_exists(baseSessionPath))
    {
        create_directories(baseSessionPath);
    }

    // Scan for existing sessions
    vector<string> entries = list_directory(baseSessionPath);
    for (const string& sessionId : entries)
    {
        string fullPath = baseSessionPath + "/" + sessionId;
        if (is_directory(fullPath))
        {
            //  Try to load metadata for this session
            SessionMetadata metadata;
            string metadataPath = fullPath + "/metadata.json";

            if (path_exists(metadataPath))
            {
                // For now, extracting session name from the directory name
                // Format: name_YYYY-MM-DD-HHMMSS
                size_t lastUnderscore = sessionId.find_last_of('_');
                if (lastUnderscore != string::npos)
                {
                    metadata.name = sessionId.substr(0, lastUnderscore);
                }
                else
                {
                    metadata.name = sessionId; // Fallback to full session ID
                }
                sessionCache[metadata.name] = metadata;
            }
        }
    }
}

SessionManager::~SessionManager()
{
    // Save current session if it exists
    if (hasActiveSession())
    {
        saveCurrentSession();
    }
}

bool SessionManager::createSession(const string &name, const string &description)
{
    if (sessionCache.find(name) != sessionCache.end()) {
        cout << "❌ Session '" << name << "' already exists.\n";
        return false;
    }

    string sessionId = generateSessionId(name);

    // Initialize metadata
    currentMetadata.name = name;
    currentMetadata.description = description;
    currentMetadata.created_at = getCurrentTimestamp();
    currentMetadata.last_modified = currentMetadata.created_at;
    currentMetadata.total_chunks = 0;
    currentMetadata.total_messages = 0;

    // Create session directory
    string sessionPath = baseSessionPath + "/" + sessionId;
    if (!create_directories(sessionPath)) {
        cout << "❌ Failed to create session directory.\n";
        return false;
    }

    // Save metadata
    if (!saveMetadata(sessionId)) {
        cout << "❌ Failed to save session metadata.\n";
        return false;
    }

    sessionCache[name] = currentMetadata; 
    currentSessionName = name;
    cout << "✅ Session '" << name << "' created successfully.\n";
    return true;
}

bool SessionManager::deleteSession(const string &name)
{
    if (sessionCache.find(name) == sessionCache.end())
    {
        cout << "❌ Session '" << name << "' not found.\n";
        return false;
    }

    string sessionId = generateSessionId(name);
    string sessionPath = baseSessionPath + "/" + sessionId;

    // Replace fs::remove_all with custom function
    if (!remove_directory_recursive(sessionPath))
    {
        cout << "❌ Failed to delete session directory.\n";
        return false;
    }

    sessionCache.erase(name);

    if (currentSessionName == name)
    {
        currentSessionName.clear();
        currentDocChunks.clear();
        currentChatHistory.clear();
    }

    cout << "✅ Session '" << name << "' deleted successfully.\n";
    return true;
}

bool SessionManager::saveCurrentSession() {
    if (currentSessionName.empty()) {
        cout << "❌ No active session to save.\n";
        return false;
    }

    string sessionId = generateSessionId(currentSessionName);
    
    // Update metadata timestamp
    currentMetadata.last_modified = getCurrentTimestamp();
    
    // Save all components
    bool success = true;
    success &= saveMetadata(sessionId);
    success &= saveChatHistory(sessionId);
    success &= saveDocumentChunks(sessionId);
    success &= saveFaissIndex(sessionId);  // Updated method name
    
    if (success) {
        cout << "✅ Session saved successfully.\n";
    } else {
        cout << "❌ Failed to save some session components.\n";
    }
    
    return success;
}

vector<string> SessionManager::listSessions()
{
    vector<string> sessions;
    for (const auto &pair : sessionCache)
    {
        sessions.push_back(pair.first);
    }
    return sessions;
}

bool SessionManager::hasActiveSession() const
{
    return !currentSessionName.empty();
}

string SessionManager::getCurrentSessionName() const
{
    return currentSessionName;
}

SessionMetadata SessionManager::getCurrentMetadata() const
{
    return currentMetadata;
}

bool SessionManager::addDocument(const string &filePath)
{
    if (!hasActiveSession())
    {
        cout << "❌ No active session. Create or load a session first.\n";
        return false;
    }

    if (!path_exists(filePath))
    {
        cout << "❌ File '" << filePath << "' does not exist.\n";
        return false;
    }

    // Check if document is already added
    auto it = find(currentMetadata.documents.begin(), currentMetadata.documents.end(), filePath);
    if (it != currentMetadata.documents.end())
    {
        cout << "⚠️  Document '" << filePath << "' already added to session.\n";
        return false;
    }

    // Process document using the new document processor
    DocumentProcessor processor;
    vector<TextChunk> textChunks = processor.processDocument(filePath);
    
    if (textChunks.empty()) {
        cout << "❌ Failed to process document or document is empty.\n";
        return false;
    }

    // Convert TextChunk to DocumentChunk and add to session
    for (const auto& textChunk : textChunks) {
        DocumentChunk chunk;
        chunk.id = textChunk.id;
        chunk.content = textChunk.content;
        chunk.source_file = textChunk.source_file;
        chunk.chunk_index = textChunk.chunk_index;
        chunk.start_position = textChunk.start_position;
        chunk.end_position = textChunk.end_position;
        
        currentDocChunks.push_back(chunk);
    }

    // Add to metadata
    currentMetadata.documents.push_back(filePath);
    currentMetadata.total_chunks = currentDocChunks.size();

    cout << "✅ Document '" << filePath << "' processed into " << textChunks.size() << " chunks.\n";
    return true;
}


vector<string> SessionManager::getDocuments() const {
    return currentMetadata.documents;
}


vector<DocumentChunk> SessionManager::getDocumentChunks() const {
    return currentDocChunks;
}

bool SessionManager::addChatMessage(const string& question, const string& answer,
                                   const vector<string>& sourceChunks) {
    if (!hasActiveSession()) {
        cout << "❌ No active session.\n";
        return false;
    }
    
    ChatMessage message;
    message.id = generateUniqueId();
    message.question = question;
    message.answer = answer;
    message.timestamp = getCurrentTimestamp();
    message.source_chunks = sourceChunks;
    
    currentChatHistory.push_back(message);
    currentMetadata.total_messages = currentChatHistory.size();
    
    return true;
}

vector<ChatMessage> SessionManager::getChatHistory() const {
    return currentChatHistory;
}

ChatMessage SessionManager::getLastMessage() const {
    if (currentChatHistory.empty()) {
        return ChatMessage();
    }
    return currentChatHistory.back();
}


void SessionManager::printSessionInfo() const {
    if (!hasActiveSession()) {
        cout << "❌ No active session.\n";
        return;
    }
    
    cout << "\n📊 SESSION INFO:\n";
    cout << "Name: " << currentMetadata.name << "\n";
    cout << "Created: " << currentMetadata.created_at << "\n";
    cout << "Last Modified: " << currentMetadata.last_modified << "\n";
    cout << "Documents: " << currentMetadata.documents.size() << "\n";
    cout << "Chunks: " << currentMetadata.total_chunks << "\n";
    cout << "Messages: " << currentMetadata.total_messages << "\n";
    if (!currentMetadata.description.empty()) {
        cout << "Description: " << currentMetadata.description << "\n";
    }
    cout << "\n";
}

// Private helper methods

string SessionManager::generateSessionId(const string& name) {
    // Find existing session with this name to reuse same timestamp
    vector<string> entries = list_directory(baseSessionPath);
    for (const string& sessionId : entries) {
        size_t lastUnderscore = sessionId.find_last_of('_');
        if (lastUnderscore != string::npos) {
            string existingName = sessionId.substr(0, lastUnderscore);
            if (existingName == name) {
                return sessionId; // Reuse existing session ID
            }
        }
    }

    // Create new session ID with current timestamp
    auto now = chrono::system_clock::now();
    auto time_t = chrono::system_clock::to_time_t(now);

    stringstream ss;
    ss << name << "_" << put_time(std::localtime(&time_t), "%Y-%m-%d-%H%M%S");
    return ss.str();
}

string SessionManager::getCurrentTimestamp() {
    auto now = chrono::system_clock::now();
    auto time_t = chrono::system_clock::to_time_t(now);
    
    stringstream ss;
    ss << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

string SessionManager::generateUniqueId() {
    auto now = chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = chrono::duration_cast<chrono::milliseconds>(duration).count();
    
    return "id_" + to_string(millis);
}

bool SessionManager::saveMetadata(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/metadata.json";
    ofstream file(filePath);

     if (!file.is_open()) {
        return false;
    }

    file << metadataToJson();
    file.close();
    return true;
}


bool SessionManager::saveChatHistory(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/chat_history.json";
    ofstream file(filePath);
    
    if (!file.is_open()) {
        return false;
    }
    
    file << chatHistoryToJson();
    file.close();
    return true;
}

bool SessionManager::saveDocumentChunks(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/doc_chunks.json";
    ofstream file(filePath);
    
    if (!file.is_open()) {
        return false;
    }
    
    file << documentChunksToJson();
    file.close();
    return true;
}

bool SessionManager::saveFaissIndex(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/faiss_index.bin";
    
    // TODO: Implement FAISS index saving
    // For now, create an empty file as placeholder
    ofstream file(filePath, ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Placeholder: Write a simple header to indicate this is a FAISS index file
    string header = "FAISS_INDEX_V1";
    file.write(header.c_str(), header.length());
    file.close();
    return true;
}

bool SessionManager::loadMetadata(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/metadata.json";
    ifstream file(filePath);
    
    if (!file.is_open()) {
        return false;
    }
    
    string content((istreambuf_iterator<char>(file)),
                        istreambuf_iterator<char>());
    file.close();
    
    return parseMetadataFromJson(content);
}

bool SessionManager::loadChatHistory(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/chat_history.json";
    ifstream file(filePath);
    
    if (!file.is_open()) {
        // Chat history file might not exist for new sessions
        currentChatHistory.clear();
        return true;
    }
    
    string content((istreambuf_iterator<char>(file)),
                        istreambuf_iterator<char>());
    file.close();
    
    return parseChatHistoryFromJson(content);
}

bool SessionManager::loadDocumentChunks(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/doc_chunks.json";
    ifstream file(filePath);
    
    if (!file.is_open()) {
        // Document chunks file might not exist for new sessions
        currentDocChunks.clear();
        return true;
    }
    
    string content((istreambuf_iterator<char>(file)),
                        istreambuf_iterator<char>());
    file.close();
    
    return parseDocumentChunksFromJson(content);
}

bool SessionManager::loadFaissIndex(const string& sessionId) {
    string filePath = baseSessionPath + "/" + sessionId + "/faiss_index.bin";
    
    if (!path_exists(filePath)) {
        // Index file might not exist for sessions without documents
        return true;
    }
    
    // TODO: Implement FAISS index loading
    // For now, just verify the file exists and has the header
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    string header(14, '\0');  // "FAISS_INDEX_V1" length
    file.read(&header[0], 14);
    file.close();
    
    return header == "FAISS_INDEX_V1";
}

bool SessionManager::loadSession(const string& name) {
    if (sessionCache.find(name) == sessionCache.end()) {
        cout << "❌ Session '" << name << "' not found.\n";
        return false;
    }

    // Save current session if one is active
    if (hasActiveSession()) {
        saveCurrentSession();
    }

    string sessionId = generateSessionId(name);

    // Load all session components
    bool success = true;
    success &= loadMetadata(sessionId);
    success &= loadChatHistory(sessionId);
    success &= loadDocumentChunks(sessionId);
    success &= loadFaissIndex(sessionId);  // Updated method name

    if (success) {
        currentSessionName = name;
        cout << "✅ Session '" << name << "' loaded successfully.\n";
    } else {
        cout << "❌ Failed to load session '" << name << "'.\n";
    }

    return success;
}

// Simple JSON serialization (basic implementation, can be improved)
string SessionManager::metadataToJson() {
    stringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << currentMetadata.name << "\",\n";
    ss << "  \"created_at\": \"" << currentMetadata.created_at << "\",\n";
    ss << "  \"last_modified\": \"" << currentMetadata.last_modified << "\",\n";
    ss << "  \"description\": \"" << currentMetadata.description << "\",\n";
    ss << "  \"total_chunks\": " << currentMetadata.total_chunks << ",\n";
    ss << "  \"total_messages\": " << currentMetadata.total_messages << ",\n";
    ss << "  \"documents\": [\n";
    
    for (size_t i = 0; i < currentMetadata.documents.size(); ++i) {
        ss << "    \"" << currentMetadata.documents[i] << "\"";
        if (i < currentMetadata.documents.size() - 1) ss << ",";
        ss << "\n";
    }
    
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}


string SessionManager::chatHistoryToJson() {
    stringstream ss;
    ss << "{\n";
    ss << "  \"messages\": [\n";
    
    for (size_t i = 0; i < currentChatHistory.size(); ++i) {
        const auto& msg = currentChatHistory[i];
        ss << "    {\n";
        ss << "      \"id\": \"" << msg.id << "\",\n";
        ss << "      \"question\": \"" << msg.question << "\",\n";
        ss << "      \"answer\": \"" << msg.answer << "\",\n";
        ss << "      \"timestamp\": \"" << msg.timestamp << "\",\n";
        ss << "      \"source_chunks\": [";
        
        for (size_t j = 0; j < msg.source_chunks.size(); ++j) {
            ss << "\"" << msg.source_chunks[j] << "\"";
            if (j < msg.source_chunks.size() - 1) ss << ", ";
        }
        
        ss << "]\n";
        ss << "    }";
        if (i < currentChatHistory.size() - 1) ss << ",";
        ss << "\n";
    }
    
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

string SessionManager::documentChunksToJson() {
    stringstream ss;
    ss << "{\n";
    ss << "  \"chunks\": [\n";
    
    for (size_t i = 0; i < currentDocChunks.size(); ++i) {
        const auto& chunk = currentDocChunks[i];
        ss << "    {\n";
        ss << "      \"id\": \"" << chunk.id << "\",\n";
        ss << "      \"content\": \"" << chunk.content << "\",\n";
        ss << "      \"source_file\": \"" << chunk.source_file << "\",\n";
        ss << "      \"chunk_index\": " << chunk.chunk_index << ",\n";
        ss << "      \"start_position\": " << chunk.start_position << ",\n";
        ss << "      \"end_position\": " << chunk.end_position << "\n";
        ss << "    }";
        if (i < currentDocChunks.size() - 1) ss << ",";
        ss << "\n";
    }
    
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

// Basic JSON parsing (simplified implementation)
bool SessionManager::parseMetadataFromJson(const string& json) {
    // TODO: Implement proper JSON parsing
    // For now, just initialize with default values
    currentMetadata = SessionMetadata();
    return true;
}

bool SessionManager::parseChatHistoryFromJson(const string& json) {
    // TODO: Implement proper JSON parsing
    currentChatHistory.clear();
    return true;
}

bool SessionManager::parseDocumentChunksFromJson(const string& json) {
    // TODO: Implement proper JSON parsing
    currentDocChunks.clear();
    return true;
}

bool SessionManager::exportSession(const string& sessionName, const string& format) {
    auto it = sessionCache.find(sessionName);
    if (it == sessionCache.end()) {
        cout << "❌ Session '" << sessionName << "' not found.\n";
        return false;
    }
    
    string exportPath = sessionName + "_export." + format;
    ofstream exportFile(exportPath);
    
    if (!exportFile.is_open()) {
        cout << "❌ Failed to create export file: " << exportPath << "\n";
        return false;
    }
    
    const SessionMetadata& metadata = it->second;
    
    if (format == "txt") {
        exportFile << "=== MIMIR SESSION EXPORT ===\n\n";
        exportFile << "Session: " << metadata.name << "\n";
        exportFile << "Created: " << metadata.created_at << "\n";
        exportFile << "Last Modified: " << metadata.last_modified << "\n";
        if (!metadata.description.empty()) {
            exportFile << "Description: " << metadata.description << "\n";
        }
        exportFile << "\nDocuments (" << metadata.documents.size() << "):\n";
        
        for (const auto& doc : metadata.documents) {
            exportFile << "  - " << doc << "\n";
        }
        
        exportFile << "\nChat History (" << metadata.total_messages << " messages):\n\n";
        
        // Load current session temporarily to export chat history
        string currentSession = currentSessionName;
        if (sessionName != currentSessionName) {
            // Temporarily load this session to get chat history
            string sessionId = generateSessionId(sessionName);
            vector<ChatMessage> tempHistory;
            // This is simplified - you'd need to load the actual history
        }
        
        for (const auto& chat : currentChatHistory) {
            exportFile << "Q: " << chat.question << "\n";
            exportFile << "A: " << chat.answer << "\n";
            exportFile << "   [" << chat.timestamp << "]\n\n";
        }
    }
    
    exportFile.close();
    cout << "✅ Session '" << sessionName << "' exported to: " << exportPath << "\n";
    return true;
}

void SessionManager::closeSession() {
    if (hasActiveSession()) {
        // Save current session before closing
        saveCurrentSession();
        
        // Clear current session state
        currentSessionName.clear();
        currentDocChunks.clear();
        currentChatHistory.clear();
        currentMetadata = SessionMetadata();
    }
}