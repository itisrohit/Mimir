#include <iostream>
#include <string>
#include <vector>
#include <sstream>


using namespace std;

class MimirCLI {

public:
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
        cout << "  add-doc <file_path>     - Add document to current session\n";
        cout << "  query <question>        - Query documents in current session\n";
        cout << "  list                    - List all sessions\n";
        cout << "  help                    - Show this help message\n";
        cout << "  quit/exit               - Exit application\n";
        cout << "\n";
    }

    // Parses a command line input into tokens
    vector<string> parseCommand(const string& input) {
        vector<string> tokens;
        istringstream iss (input);
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
            cout << "Initializing session: " << tokens[1] << "\n";
            // TODO: Implement session initialization logic
        }
        else if (command == "add-doc") {
            if (tokens.size() < 2) {
                cout << "Usage: add-doc <file_path>\n";
                return;
            }
            cout << "Adding document: " << tokens[1] << "\n";
            // TODO: Implement document addition logic
        }
        else if (command == "query") {
            if (tokens.size() < 2) {
                cout << "Usage: query <question>\n";
                return;
            }
            string question;
            for (size_t i = 1; i< tokens.size(); ++i) {
                question += tokens[i] + " ";
            }
            cout << "Processing query: " << question << "\n";
            // TODO: Implement query processing logic
        }
        else if (command == "list") {
            cout << "Listing all sessions...\n";
            // TODO: Implement session listing logic
        }
        else {
            cout << "Unknown command: " << command << "\n";
            cout << "Type 'help' for a list of available commands.\n";
        }
    }

    void run() {
        printWelcome();
        printHelp();

        string input;
        while (true) {
            cout << "mimir> ";
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

