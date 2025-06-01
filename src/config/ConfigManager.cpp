#include "ConfigManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const string& configPath) {
    // Set defaults first
    setDefaults();
    
    // Try to load from file
    if (!parseYamlFile(configPath)) {
        cout << "⚠️  Could not load config from " << configPath << ", using defaults.\n";
        return false;
    }
    
    cout << "✅ Configuration loaded from " << configPath << "\n";
    return true;
}

bool ConfigManager::parseYamlFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    string line;
    string currentSection;
    string currentSubsection;
    
    while (getline(file, line)) {
        line = trim(line);
        
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check for section headers
        if (line.back() == ':' && line.find("  ") != 0) {
            currentSection = line.substr(0, line.length() - 1);
            currentSubsection.clear();
            continue;
        }
        
        // Check for subsection headers (indented)
        if (line.find("  ") == 0 && line.back() == ':') {
            currentSubsection = trim(line.substr(2, line.length() - 3));
            continue;
        }
        
        // Parse key-value pairs
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string key = trim(line.substr(0, colonPos));
            string value = trim(line.substr(colonPos + 1));
            
            // Remove quotes if present
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            // Remove leading spaces for indented keys
            if (key.find("  ") == 0) {
                key = key.substr(2);
            }
            
            // Apply configuration based on section
            applyConfig(currentSection, currentSubsection, key, value);
        }
    }
    
    file.close();
    return true;
}

void ConfigManager::applyConfig(const string& section, const string& subsection, 
                               const string& key, const string& value) {
    if (section == "app") {
        if (key == "name") app.name = value;
        else if (key == "version") app.version = value;
        else if (key == "debug") app.debug = (value == "true");
    }
    else if (section == "paths") {
        if (key == "sessions_dir") paths.sessions_dir = value;
        else if (key == "temp_dir") paths.temp_dir = value;
        else if (key == "logs_dir") paths.logs_dir = value;
        else if (key == "exports_dir") paths.exports_dir = value;
    }
    else if (section == "document_processing") {
        if (key == "chunk_size") document_processing.chunk_size = stoul(value);
        else if (key == "chunk_overlap") document_processing.chunk_overlap = stoul(value);
        else if (key == "preserve_sentences") document_processing.preserve_sentences = (value == "true");
        else if (key == "preserve_paragraphs") document_processing.preserve_paragraphs = (value == "true");
        else if (key == "max_file_size_mb") document_processing.max_file_size_mb = stoul(value);
        else if (key == "remove_extra_whitespace") document_processing.remove_extra_whitespace = (value == "true");
        else if (key == "normalize_unicode") document_processing.normalize_unicode = (value == "true");
    }
    else if (section == "embedding") {
        if (subsection.empty()) {
            if (key == "provider") embedding.provider = value;
            else if (key == "model") embedding.model = value;
            else if (key == "vector_dimension") embedding.vector_dimension = stoi(value);
            else if (key == "batch_size") embedding.batch_size = stoi(value);
        } else {
            embedding.provider_settings[subsection + "." + key] = value;
        }
    }
    else if (section == "vector_db") {
        if (subsection.empty()) {
            if (key == "type") vector_db.type = value;
        } else if (subsection == "faiss") {
            if (key == "index_type") vector_db.index_type = value;
            else if (key == "metric") vector_db.metric = value;
            else if (key == "nlist") vector_db.nlist = stoi(value);
        }
    }
    else if (section == "chat") {
        if (subsection.empty()) {
            if (key == "provider") chat.provider = value;
            else if (key == "model") chat.model = value;
            else if (key == "max_tokens") chat.max_tokens = stoi(value);
            else if (key == "temperature") chat.temperature = stod(value);
            else if (key == "max_context_chunks") chat.max_context_chunks = stoi(value);
            else if (key == "similarity_threshold") chat.similarity_threshold = stod(value);
        } else {
            chat.provider_settings[subsection + "." + key] = value;
        }
    }
    else if (section == "logging") {
        if (key == "level") logging.level = value;
        else if (key == "file_logging") logging.file_logging = (value == "true");
        else if (key == "console_logging") logging.console_logging = (value == "true");
        else if (key == "max_log_size_mb") logging.max_log_size_mb = stoi(value);
        else if (key == "max_log_files") logging.max_log_files = stoi(value);
    }
    else if (section == "performance") {
        if (key == "enable_caching") performance.enable_caching = (value == "true");
        else if (key == "cache_size_mb") performance.cache_size_mb = stoi(value);
        else if (key == "parallel_processing") performance.parallel_processing = (value == "true");
        else if (key == "max_threads") performance.max_threads = stoi(value);
    }
    else if (section == "export") {
        if (key == "default_format") export_config.default_format = value;
        else if (key == "include_metadata") export_config.include_metadata = (value == "true");
        else if (key == "include_timestamps") export_config.include_timestamps = (value == "true");
        else if (key == "include_sources") export_config.include_sources = (value == "true");
    }
    else if (section == "session") {
        if (key == "auto_save") session.auto_save = (value == "true");
        else if (key == "save_interval_minutes") session.save_interval_minutes = stoi(value);
        else if (key == "max_sessions") session.max_sessions = stoi(value);
        else if (key == "cleanup_old_sessions") session.cleanup_old_sessions = (value == "true");
        else if (key == "max_session_age_days") session.max_session_age_days = stoi(value);
    }
}

void ConfigManager::setDefaults() {
    // Defaults are already set in struct definitions
    // This method can be used for any additional default logic
}

void ConfigManager::printConfig() const {
    cout << "\n📋 CURRENT CONFIGURATION:\n";
    cout << "App: " << app.name << " v" << app.version << "\n";
    cout << "Sessions Dir: " << paths.sessions_dir << "\n";
    cout << "Chunk Size: " << document_processing.chunk_size << "\n";
    cout << "Chunk Overlap: " << document_processing.chunk_overlap << "\n";
    cout << "Embedding Provider: " << embedding.provider << "\n";
    cout << "Embedding Model: " << embedding.model << "\n";
    cout << "Vector DB: " << vector_db.type << "\n";
    cout << "Chat Provider: " << chat.provider << "\n";
    cout << "Chat Model: " << chat.model << "\n";
    cout << "\n";
}

string ConfigManager::trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

vector<string> ConfigManager::split(const string& str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}