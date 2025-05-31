#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace std;

struct DocumentChunk {
    string id;
    string content;
    string source_file;
    int chunk_index;
    size_t start_position;
    size_t end_position;
};

struct ChatMessage {
    string id;
    string question;
    string answer;
    string timestamp;
    vector<string> source_chunks; // References to relevant document chunks
};

struct SessionMetadata {
    string name;
    string created_at;
    string last_modified;
    vector<string> documents;
    int total_chunks;
    int total_messages;
    string description;
};

class SessionManager {
private:
    string currentSessionName;
    string baseSessionPath;
    map<string, SessionMetadata> sessionCache;
    
    // Current session data
    SessionMetadata currentMetadata;
    vector<DocumentChunk> currentDocChunks;
    vector<ChatMessage> currentChatHistory;
    
    // Helper methods
    string generateSessionId(const string& name);
    string getCurrentTimestamp();
    string generateUniqueId();
    
    // File operations
    bool createSessionDirectory(const string& sessionId);
    bool saveMetadata(const string& sessionId);
    bool saveChatHistory(const string& sessionId);
    bool saveDocumentChunks(const string& sessionId);
    
    bool loadMetadata(const string& sessionId);
    bool loadChatHistory(const string& sessionId);
    bool loadDocumentChunks(const string& sessionId);
    
    // JSON helpers
    string metadataToJson();
    string chatHistoryToJson();
    string documentChunksToJson();
    
    bool parseMetadataFromJson(const string& json);
    bool parseChatHistoryFromJson(const string& json);
    bool parseDocumentChunksFromJson(const string& json);

public:
    SessionManager(const string& basePath = "./sessions");
    ~SessionManager();
    
    // Session lifecycle
    bool createSession(const string& name, const string& description = "");
    bool loadSession(const string& name);
    bool deleteSession(const string& name);
    bool saveCurrentSession();
    void closeSession(); 
    
    // Session info
    vector<string> listSessions();
    bool hasActiveSession() const;
    string getCurrentSessionName() const;
    SessionMetadata getCurrentMetadata() const;
    
    // Document management
    bool addDocument(const string& filePath);
    vector<string> getDocuments() const;
    vector<DocumentChunk> getDocumentChunks() const;
    
    // Chat management
    bool addChatMessage(const string& question, const string& answer, 
                       const vector<string>& sourceChunks = {});
    vector<ChatMessage> getChatHistory() const;
    ChatMessage getLastMessage() const;
    
    // Utility
    bool exportSession(const string& sessionName, const string& format = "txt");
    void printSessionInfo() const;
};

#endif // SESSION_MANAGER_H