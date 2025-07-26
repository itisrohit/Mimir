#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include "session/SessionManager.h"
#include "config/ConfigManager.h"

using namespace std;

class MimirCLI {
private:
    SessionManager* sessionManager;

public:
    MimirCLI() {
        // Load configuration FIRST
        auto& config = ConfigManager::getInstance();
        if (!config.loadConfig("config.yaml")) {
            cout << "⚠️  Using default configuration\n";
        }
        // Now initialize session manager
        sessionManager = new SessionManager("");
    }
    ~MimirCLI() {
        delete sessionManager;
    }

    void printWelcome() {
        cout << "\n";
        cout << "    ███╗   ███╗██╗███╗   ███╗██╗██████╗ \n";
        cout << "    ████╗ ████║██║████╗ ████║██║██╔══██╗\n";
        cout << "    ██╔████╔██║██║██╔████╔██║██║██████╔╝\n";
        cout << "    ██║╚██╔╝██║██║██║╚██╔╝██║██║██╔══██╗\n";
        cout << "    ██║ ╚═╝ ██║██║██║ ╚═╝ ██║██║██║  ██║\n";
        cout << "    ╚═╝     ╚═╝╚═╝╚═╝     ╚═╝╚═╝╚═╝  ╚═╝\n";
        cout << "\n";
        cout << "    ────────────────────────────────────────\n";
        cout << "    ⚡ The smartest way to talk to your data ⚡\n";
        cout << "    ────────────────────────────────────────\n";
        cout << "\n";
        cout << "    Commands:\n";
        cout << "    • help  - Show available commands\n";
        cout << "    • quit  - Exit application\n";
        cout << "\n";
    }

    void printHelp() {
        cout << "Available Commands:\n";
        cout << "  init <session_name>     - Initialize a new session\n";
        cout << "  load <session_name>     - Load an existing session\n";
        cout << "  close                   - Close current session\n";
        cout << "  delete <session_name>   - Delete a session\n";
        cout << "  add-doc <file_path>     - Add document to current session\n";
        cout << "  query <question>        - Query documents in current session\n";
        cout << "  list                    - List all sessions\n";
        cout << "  info                    - Show current session info\n";
        cout << "  export <session_name>   - Export session data\n";
        cout << "  config [show|reload]    - Configuration management\n";
        cout << "  help                    - Show this help message\n";
        cout << "  quit/exit               - Exit application\n";
        cout << "\n";
    }

    // Parses a command line input into tokens
    vector<string> parseCommand(const string& input) {
        vector<string> tokens;
        istringstream iss(input);
        string token;

        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void handleCommand(const vector<string>& tokens) {
        if (tokens.empty()) {
            return;
        }

        const string& command = tokens[0];
        
        if (command == "help") {
            printHelp();
        }
        else if (command == "quit" || command == "exit") {
            cout << "Thanks for using Mimir! 👋\n";
            exit(0);
        }
        else if (command == "init") {
            if (tokens.size() < 2) {
                cout << "Usage: init <session_name>\n";
                return;
            }
            string description = "";
            if (tokens.size() > 2) {
                for (size_t i = 2; i < tokens.size(); ++i) {
                    description += tokens[i] + " ";
                }
            }
            sessionManager->createSession(tokens[1], description);
        }
        else if (command == "load") {
            if (tokens.size() < 2) {
                cout << "Usage: load <session_name>\n";
                return;
            }
            sessionManager->loadSession(tokens[1]);
        }
        else if (command == "close") {
            if (!sessionManager->hasActiveSession()) {
                cout << "❌ No active session to close.\n";
            } else {
                string sessionName = sessionManager->getCurrentSessionName();
                sessionManager->saveCurrentSession();  // Save before closing
                sessionManager->closeSession();        // New method we need to add
                cout << "✅ Session '" << sessionName << "' closed and saved.\n";
            }
        }
        else if (command == "delete" || command == "del") {
            if (tokens.size() < 2) {
                cout << "Usage: delete <session_name>\n";
                return;
            }
            
            // Add confirmation for safety
            string sessionName = tokens[1];
            cout << "⚠️  Are you sure you want to delete session '" << sessionName << "'? (y/N): ";
            string confirmation;
            getline(cin, confirmation);
            
            if (confirmation == "y" || confirmation == "Y" || confirmation == "yes") {
                sessionManager->deleteSession(sessionName);
            } else {
                cout << "❌ Delete cancelled.\n";
            }
        }
        else if (command == "add-doc") {
            if (tokens.size() < 2) {
                cout << "Usage: add-doc <file_path>\n";
                cout << "For files with spaces:\n";
                cout << "  • Use quotes: add-doc \"file name.pdf\"\n";
                cout << "  • Or escape spaces: add-doc file\\ name.pdf\n";
                return;
            }
            
            // Reconstruct the full file path from remaining tokens
            string filePath;
            for (size_t i = 1; i < tokens.size(); ++i) {
                if (i > 1) filePath += " ";
                filePath += tokens[i];
            }
            
            // Clean up escaped characters and quotes
            if (!filePath.empty()) {
                // Remove surrounding quotes
                if ((filePath.front() == '"' && filePath.back() == '"') ||
                    (filePath.front() == '\'' && filePath.back() == '\'')) {
                    filePath = filePath.substr(1, filePath.length() - 2);
                }
                
                // Handle escaped spaces
                regex escapedSpace("\\\\ ");
                filePath = regex_replace(filePath, escapedSpace, " ");
            }
            
            cout << "📁 Processing file: " << filePath << "\n";
            sessionManager->addDocument(filePath);
        }
        else if (command == "query") {
            if (tokens.size() < 2) {
                cout << "Usage: query <question>\n";
                return;
            }
            string question;
            for (size_t i = 1; i < tokens.size(); ++i) {
                question += tokens[i] + " ";
            }
            
            // TODO: Implement actual query processing
            string answer = "This is a placeholder response for: " + question;
            sessionManager->addChatMessage(question, answer);
            cout << "💡 " << answer << "\n";
        }
        else if (command == "list") {
            vector<string> sessions = sessionManager->listSessions();
            if (sessions.empty()) {
                cout << "No sessions found.\n";
            } else {
                cout << "📋 Available sessions:\n";
                for (const auto& session : sessions) {
                    string marker = (session == sessionManager->getCurrentSessionName()) ? " (active)" : "";
                    cout << "  • " << session << marker << "\n";
                }
            }
        }
        else if (command == "info") {
            sessionManager->printSessionInfo();
            // Show auto-save status
            if (sessionManager->isAutoSaveEnabled()) {
                cout << "💾 Auto-save: Enabled (documents saved immediately)\n";
            } else {
                cout << "💾 Auto-save: Disabled (manual save required)\n";
            }
        }
        else if (command == "export") {
            if (tokens.size() < 2) {
                cout << "Usage: export <session_name>\n";
                return;
            }
            sessionManager->exportSession(tokens[1]);
        }
        else if (command == "config") {
            if (tokens.size() > 1) {
                if (tokens[1] == "show") {
                    ConfigManager::getInstance().printConfig();
                } else if (tokens[1] == "reload") {
                    auto& config = ConfigManager::getInstance();
                    if (config.loadConfig("config.yaml")) {
                        cout << "✅ Configuration reloaded\n";
                    } else {
                        cout << "❌ Failed to reload configuration\n";
                    }
                } else if (tokens[1] == "set" && tokens.size() >= 4) {
                    // config set section key value
                    // TODO: Implement runtime config changes
                    cout << "⚠️  Runtime config changes not yet implemented\n";
                }
            } else {
                cout << "Usage: config [show|reload|set]\n";
            }
        }
        else {
            cout << "Unknown command: " << command << "\n";
            cout << "Type 'help' for a list of available commands.\n";
        }
    }

    void run() {
        printWelcome();

        string input;
        while (true) {
            string prompt = "mimir";
            if (sessionManager->hasActiveSession()) {
                prompt += " [" + sessionManager->getCurrentSessionName() + "]";
            }
            prompt += "> ";
            
            cout << prompt;
            getline(cin, input);

            if (cin.eof()) {
                cout << "\nThanks for using Mimir! 👋\n";
                break;
            }

            auto tokens = parseCommand(input);
            handleCommand(tokens);
        }
    }
};

int main() {
    MimirCLI cli;
    cli.run();
    return 0;
}

