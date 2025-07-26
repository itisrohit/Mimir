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
        else if (key == "description") app.description = value;
    }
    else if (section == "paths") {
        if (key == "sessions_dir") paths.sessions_dir = value;
        else if (key == "temp_dir") paths.temp_dir = value;
        else if (key == "logs_dir") paths.logs_dir = value;
        else if (key == "exports_dir") paths.exports_dir = value;
        else if (key == "models_dir") paths.models_dir = value;
    }
    else if (section == "document_processing") {
        if (key == "chunk_size") document_processing.chunk_size = stoul(value);
        else if (key == "chunk_overlap") document_processing.chunk_overlap = stoul(value);
        else if (key == "preserve_sentences") document_processing.preserve_sentences = (value == "true");
        else if (key == "preserve_paragraphs") document_processing.preserve_paragraphs = (value == "true");
        else if (key == "max_file_size_mb") document_processing.max_file_size_mb = stoul(value);
        else if (key == "remove_extra_whitespace") document_processing.remove_extra_whitespace = (value == "true");
        else if (key == "normalize_unicode") document_processing.normalize_unicode = (value == "true");
        else if (key == "clean_text") document_processing.clean_text = (value == "true");
        else if (key == "preserve_formatting") document_processing.preserve_formatting = (value == "true");
        else if (key == "separators") {
            // Simple separator parsing - split by comma
            document_processing.separators.clear();
            vector<string> seps = split(value, ',');
            for (const string& sep : seps) {
                string cleanSep = trim(sep);
                // Handle escaped characters
                if (cleanSep == "\\n") cleanSep = "\n";
                else if (cleanSep == "\\t") cleanSep = "\t";
                document_processing.separators.push_back(cleanSep);
            }
        }
        else if (key == "supported_types") {
            document_processing.supported_types.clear();
            vector<string> types = split(value, ',');
            for (const string& type : types) {
                document_processing.supported_types.push_back(trim(type));
            }
        }
    }
    else if (section == "embedding") {
        if (subsection.empty()) {
            if (key == "model") embedding.model = value;
            else if (key == "dim") embedding.dim = stoi(value);
            else if (key == "batch_size") embedding.batch_size = stoi(value);
            else if (key == "semantic_search_enabled") embedding.semantic_search_enabled = (value == "true");
            else if (key == "enable_caching") embedding.enable_caching = (value == "true");
            else if (key == "cache_size_mb") embedding.cache_size_mb = stoi(value);
            else if (key == "parallel_processing") embedding.parallel_processing = (value == "true");
            else if (key == "max_threads") embedding.max_threads = stoi(value);
        } else if (subsection == "tokenizer") {
            if (key == "type") embedding.tokenizer.type = value;
            else if (key == "model_path") embedding.tokenizer.model_path = value;
            else if (key == "max_length") embedding.tokenizer.max_length = stoi(value);
        } else if (subsection == "onnx") {
            if (key == "optimization_level") embedding.onnx.optimization_level = stoi(value);
            else if (key == "execution_mode") embedding.onnx.execution_mode = value;
            else if (key == "enable_mem_pattern") embedding.onnx.enable_mem_pattern = (value == "true");
            else if (key == "enable_cpu_mem_arena") embedding.onnx.enable_cpu_mem_arena = (value == "true");
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
        if (subsection.empty()) {
            if (key == "level") logging.level = value;
            else if (key == "file_logging") logging.file_logging = (value == "true");
            else if (key == "console_logging") logging.console_logging = (value == "true");
            else if (key == "max_log_size_mb") logging.max_log_size_mb = stoi(value);
            else if (key == "max_log_files") logging.max_log_files = stoi(value);
        } else if (subsection == "components") {
            logging.components[key] = value;
        }
    }
    else if (section == "performance") {
        if (key == "enable_caching") performance.enable_caching = (value == "true");
        else if (key == "cache_size_mb") performance.cache_size_mb = stoi(value);
        else if (key == "parallel_processing") performance.parallel_processing = (value == "true");
        else if (key == "max_threads") performance.max_threads = stoi(value);
        else if (key == "max_memory_usage_mb") performance.max_memory_usage_mb = stoi(value);
        else if (key == "enable_memory_monitoring") performance.enable_memory_monitoring = (value == "true");
        else if (key == "batch_processing") performance.batch_processing = (value == "true");
        else if (key == "max_batch_size") performance.max_batch_size = stoi(value);
        else if (key == "enable_profiling") performance.enable_profiling = (value == "true");
    }
    else if (section == "export") {
        if (subsection.empty()) {
            if (key == "default_format") export_config.default_format = value;
            else if (key == "include_metadata") export_config.include_metadata = (value == "true");
            else if (key == "include_timestamps") export_config.include_timestamps = (value == "true");
            else if (key == "include_sources") export_config.include_sources = (value == "true");
        } else if (subsection == "json") {
            if (key == "pretty_print") export_config.formats["json"].pretty_print = (value == "true");
            else if (key == "include_embeddings") export_config.formats["json"].include_embeddings = (value == "true");
        } else if (subsection == "markdown") {
            if (key == "include_headers") export_config.formats["markdown"].include_headers = (value == "true");
            else if (key == "include_links") export_config.formats["markdown"].include_links = (value == "true");
        }
    }
    else if (section == "session") {
        if (key == "auto_save") session.auto_save = (value == "true");
        else if (key == "save_interval_minutes") session.save_interval_minutes = stoi(value);
        else if (key == "max_sessions") session.max_sessions = stoi(value);
        else if (key == "cleanup_old_sessions") session.cleanup_old_sessions = (value == "true");
        else if (key == "max_session_age_days") session.max_session_age_days = stoi(value);
        else if (key == "include_embeddings") session.include_embeddings = (value == "true");
        else if (key == "include_chat_history") session.include_chat_history = (value == "true");
        else if (key == "compression_enabled") session.compression_enabled = (value == "true");
    }
    else if (section == "development") {
        if (key == "enable_debug_mode") development.enable_debug_mode = (value == "true");
        else if (key == "enable_profiling") development.enable_profiling = (value == "true");
        else if (key == "enable_memory_tracking") development.enable_memory_tracking = (value == "true");
        else if (key == "log_performance_metrics") development.log_performance_metrics = (value == "true");
    }
}

void ConfigManager::setDefaults() {
    // Defaults are already set in struct definitions
    // This method can be used for any additional default logic
}

void ConfigManager::printConfig() const {
    cout << "\n📋 CURRENT CONFIGURATION:\n";
    cout << "App: " << app.name << " v" << app.version << " - " << app.description << "\n";
    cout << "Debug Mode: " << (app.debug ? "enabled" : "disabled") << "\n";
    cout << "\n📁 Paths:\n";
    cout << "  Sessions: " << paths.sessions_dir << "\n";
    cout << "  Models: " << paths.models_dir << "\n";
    cout << "  Logs: " << paths.logs_dir << "\n";
    cout << "  Exports: " << paths.exports_dir << "\n";
    
    cout << "\n📄 Document Processing:\n";
    cout << "  Chunk Size: " << document_processing.chunk_size << " chars\n";
    cout << "  Chunk Overlap: " << document_processing.chunk_overlap << " chars\n";
    cout << "  Max File Size: " << document_processing.max_file_size_mb << " MB\n";
    cout << "  Supported Types: ";
    for (const auto& type : document_processing.supported_types) {
        cout << type << " ";
    }
    cout << "\n";
    
    cout << "\n🧠 Embedding:\n";
    cout << "  Model: " << embedding.model << "\n";
    cout << "  Dimension: " << embedding.dim << "\n";
    cout << "  Batch Size: " << embedding.batch_size << "\n";
    cout << "  Tokenizer: " << embedding.tokenizer.type << " (" << embedding.tokenizer.model_path << ")\n";
    cout << "  ONNX Optimization: Level " << embedding.onnx.optimization_level << "\n";
    cout << "  Semantic Search: " << (embedding.semantic_search_enabled ? "enabled" : "disabled") << "\n";
    
    cout << "\n🗄️ Vector Database:\n";
    cout << "  Type: " << vector_db.type << "\n";
    cout << "  Index: " << vector_db.index_type << "\n";
    cout << "  Metric: " << vector_db.metric << "\n";
    
    cout << "\n💬 Chat:\n";
    cout << "  Provider: " << chat.provider << "\n";
    cout << "  Model: " << chat.model << "\n";
    cout << "  Max Tokens: " << chat.max_tokens << "\n";
    cout << "  Temperature: " << chat.temperature << "\n";
    cout << "  Context Chunks: " << chat.max_context_chunks << "\n";
    
    cout << "\n⚡ Performance:\n";
    cout << "  Max Threads: " << performance.max_threads << "\n";
    cout << "  Cache Size: " << performance.cache_size_mb << " MB\n";
    cout << "  Max Memory: " << performance.max_memory_usage_mb << " MB\n";
    cout << "  Batch Processing: " << (performance.batch_processing ? "enabled" : "disabled") << "\n";
    cout << "  Profiling: " << (performance.enable_profiling ? "enabled" : "disabled") << "\n";
    
    cout << "\n📊 Logging:\n";
    cout << "  Level: " << logging.level << "\n";
    cout << "  File Logging: " << (logging.file_logging ? "enabled" : "disabled") << "\n";
    cout << "  Console Logging: " << (logging.console_logging ? "enabled" : "disabled") << "\n";
    
    cout << "\n💾 Session:\n";
    cout << "  Auto Save: " << (session.auto_save ? "enabled" : "disabled") << "\n";
    cout << "  Save Interval: " << session.save_interval_minutes << " minutes\n";
    cout << "  Max Sessions: " << session.max_sessions << "\n";
    cout << "  Include Embeddings: " << (session.include_embeddings ? "enabled" : "disabled") << "\n";
    
    cout << "\n🔧 Development:\n";
    cout << "  Debug Mode: " << (development.enable_debug_mode ? "enabled" : "disabled") << "\n";
    cout << "  Memory Tracking: " << (development.enable_memory_tracking ? "enabled" : "disabled") << "\n";
    cout << "  Performance Metrics: " << (development.log_performance_metrics ? "enabled" : "disabled") << "\n";
    
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

// Remove all legacy provider/vector_dimension/provider_settings logic
// Remove any use of getString, getInt, getBool
// Remove any print statements for Embedding Provider