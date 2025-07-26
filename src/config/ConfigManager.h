#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>

using namespace std;

struct AppConfig {
    string name = "Mimir";
    string version = "1.0.0";
    bool debug = false;
    string description = "High-performance document processing and embedding pipeline";
};

struct PathsConfig {
    string sessions_dir = "./.data/sessions";
    string temp_dir = "./.data/temp";
    string logs_dir = "./.data/logs";
    string exports_dir = "./.data/exports";
    string models_dir = "./models";
};

struct DocumentProcessingConfig {
    size_t chunk_size = 1000;
    size_t chunk_overlap = 200;
    bool preserve_sentences = true;
    bool preserve_paragraphs = true;
    size_t max_file_size_mb = 100;
    vector<string> supported_types = {"txt", "md", "pdf", "csv", "json"};
    bool remove_extra_whitespace = true;
    bool normalize_unicode = true;
    vector<string> separators = {"\n\n", "\n", ". ", "! ", "? ", " "};
    bool clean_text = true;
    bool preserve_formatting = false;
};

struct TokenizerConfig {
    string type = "sentencepiece";
    string model_path = "sentencepiece.bpe.model";
    int max_length = 512;
};

struct OnnxConfig {
    int optimization_level = 1;
    string execution_mode = "sequential";
    bool enable_mem_pattern = true;
    bool enable_cpu_mem_arena = true;
};

struct EmbeddingConfig {
    string model = "models/bge-m3-onnx";
    int dim = 1024;
    int batch_size = 16;
    bool semantic_search_enabled = true;
    bool enable_caching = true;
    int cache_size_mb = 256;
    bool parallel_processing = true;
    int max_threads = 4;
    TokenizerConfig tokenizer;
    OnnxConfig onnx;
};

struct VectorDbConfig {
    string type = "faiss";
    string index_type = "IndexFlatIP";
    string metric = "inner_product";
    int nlist = 100;
    map<string, string> provider_settings;
};

struct ChatConfig {
    string provider = "local";
    string model = "llama2";
    int max_tokens = 2048;
    double temperature = 0.7;
    int max_context_chunks = 5;
    double similarity_threshold = 0.7;
    map<string, string> provider_settings;
};

struct LoggingConfig {
    string level = "INFO";
    bool file_logging = true;
    bool console_logging = true;
    int max_log_size_mb = 10;
    int max_log_files = 5;
    map<string, string> components;  // Component-specific logging levels
};

struct PerformanceConfig {
    bool enable_caching = true;
    int cache_size_mb = 256;
    bool parallel_processing = true;
    int max_threads = 4;
    int max_memory_usage_mb = 2048;
    bool enable_memory_monitoring = true;
    bool batch_processing = true;
    int max_batch_size = 32;
    bool enable_profiling = false;
};

struct ExportFormatConfig {
    bool pretty_print = true;
    bool include_embeddings = false;
    bool include_headers = true;
    bool include_links = true;
};

struct ExportConfig {
    string default_format = "txt";
    bool include_metadata = true;
    bool include_timestamps = true;
    bool include_sources = true;
    map<string, ExportFormatConfig> formats;
};

struct SessionConfig {
    bool auto_save = true;
    int save_interval_minutes = 5;
    int max_sessions = 100;
    bool cleanup_old_sessions = false;
    int max_session_age_days = 30;
    bool include_embeddings = true;
    bool include_chat_history = true;
    bool compression_enabled = false;
};

struct DevelopmentConfig {
    bool enable_debug_mode = false;
    bool enable_profiling = false;
    bool enable_memory_tracking = false;
    bool log_performance_metrics = false;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    // Load configuration from file
    bool loadConfig(const string& configPath = "config.yaml");
    
    // Save current configuration
    bool saveConfig(const string& configPath = "config.yaml");
    
    // Getters for configuration sections
    const AppConfig& getAppConfig() const { return app; }
    const PathsConfig& getPathsConfig() const { return paths; }
    const DocumentProcessingConfig& getDocumentProcessingConfig() const { return document_processing; }
    const EmbeddingConfig& getEmbeddingConfig() const { return embedding; }
    const VectorDbConfig& getVectorDbConfig() const { return vector_db; }
    const ChatConfig& getChatConfig() const { return chat; }
    const LoggingConfig& getLoggingConfig() const { return logging; }
    const PerformanceConfig& getPerformanceConfig() const { return performance; }
    const ExportConfig& getExportConfig() const { return export_config; }
    const SessionConfig& getSessionConfig() const { return session; }
    const DevelopmentConfig& getDevelopmentConfig() const { return development; }
    
    // Setters for runtime configuration changes
    void setDocumentChunkSize(size_t size) { document_processing.chunk_size = size; }
    void setDocumentChunkOverlap(size_t overlap) { document_processing.chunk_overlap = overlap; }
    void setChatProvider(const string& provider) { chat.provider = provider; }
    void setVectorDbType(const string& type) { vector_db.type = type; }
    void setEmbeddingBatchSize(int batch_size) { embedding.batch_size = batch_size; }
    void setPerformanceMaxThreads(int max_threads) { performance.max_threads = max_threads; }
    void setLoggingLevel(const string& level) { logging.level = level; }
    
    // Utility methods
    string getConfigValue(const string& section, const string& key) const;
    bool setConfigValue(const string& section, const string& key, const string& value);
    void printConfig() const;
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Configuration sections - THESE WERE MISSING!
    AppConfig app;
    PathsConfig paths;
    DocumentProcessingConfig document_processing;
    EmbeddingConfig embedding;
    VectorDbConfig vector_db;
    ChatConfig chat;
    LoggingConfig logging;
    PerformanceConfig performance;
    ExportConfig export_config;
    SessionConfig session;
    DevelopmentConfig development;
    
    // Helper methods
    bool parseYamlFile(const string& filepath);
    void setDefaults();
    string trim(const string& str);
    vector<string> split(const string& str, char delimiter);
    void applyConfig(const string& section, const string& subsection, 
                    const string& key, const string& value);
};

#endif // CONFIG_MANAGER_H