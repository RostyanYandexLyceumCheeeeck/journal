#include <iostream>

#include "headers/application.h"


extern CreateLogManager_t createLogManager;
extern DestroyLogManager_t destroyLogManager;


std::vector<std::string> parseCommand(const std::string& input) {
    std::string token;
    char c = 0;
    char quote = 0;
    bool in_quote = false;
    std::vector<std::string> result;

    for (size_t i = 0; i < input.length(); i++) {
        c = input[i];

        if (c == '"' || c == '\'') {
            if (in_quote && c == quote) {
                in_quote = false;
                quote = 0;
            } else if (!in_quote) {
                in_quote = true;
                quote = c;
            } else {
                token += c;
            }
            continue;
        }

        if (!in_quote && std::isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                result.push_back(token);
                token.clear();
            }
            continue;
        }

        token += c;
    }

    if (!token.empty()) {
        result.push_back(token);
    }

    return result;
}


Application::Application(std::filesystem::path path, LevelImportance minLevel): defaultLevel_(minLevel) {
    manager_ = createLogManager(path, minLevel, 5);
}

Application::~Application() {
    if (manager_) { destroyLogManager(manager_); }
}


void Application::printHelp() {
    std::cout << "Available commands:\n"
              << "  help                - Show this message.\n"
              << "  exit                - Exit the program.\n"
              << "  log                 - Show visible window of messages.\n"
              << "  stp <count>         - Move window forward (>0) or backward (<0).\n"
              << "  del <index>         - Mark message as deleted (by index in window 0..size-1).\n"
              << "  dst <index>         - Destroy message and free space (by index in window 0..size-1).\n"
              << "  jnl -gl             - Show the current default importance level.\n"
              << "  jnl -sl <level>     - Change the default importance level to <level>.\n"
              << "  msg [level] <text>  - Saves <text> to journal, specifying the importance level.\n"
              << "                           If the [level] is not specified, it is recorded by default\n"
              << "  <text>              - If the first word of the <text> is not a command(or only the word \"msg\" was entered),\n" 
              << "                           it will be treated as <text> with no importance level.\n"
              << "                           In other words, it is not possible to write an empty string to the journal.\n";
}

void Application::printLog() {
    auto logs = manager_->getWindowLogs(); 
    
    if (logs.empty()) {
        std::cout << "Log window is empty.\n";
        return;
    }

    for (size_t i = 0; i < logs.size(); ++i) {
        std::cout << "=== [ " << i << " ] ===\n";
        std::cout << manager_->showEntry(logs[i]);
    }
}

void Application::run() {
    std::vector<std::string> tokens;
    std::cout << "CLI Journal gateway. Enter 'help' for commands, 'exit' to quit.\n";
    
    while (true) {
        std::cout << "> ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\nExit from program!\n";
            break;
        }

        if (input.empty()) continue;

        tokens = parseCommand(input);

        if (tokens.empty()) continue;

        const std::string& command = tokens[0];
        
        if (command == "exit") {
            std::cout << "\nExit from journal!\n";
            break;
        }

        if (command == "help") {
            printHelp();
            continue;
        }
        
        if (command == "log") {
            printLog();
            continue;
        }

        if (command == "stp" && tokens.size() > 1) {
            int count = std::stoi(tokens[1]);
            manager_->stepWindow(count);
            printLog();
            continue;
        }

        if (command == "del" && tokens.size() > 1) {
            std::size_t idx = std::stoull(tokens[1]);
            manager_->deleteLog(idx);
            printLog();
            continue;
        }

        if (command == "dst" && tokens.size() > 1) {
            std::size_t idx = std::stoull(tokens[1]);
            manager_->destroyLog(idx);
            printLog();
            continue;
        }

        if (command == "jnl" && tokens.size() > 1) {
            if (tokens[1] == "-gl") { 
                std::cout << "level instance to default:  " 
                          << VirtualLogger::lvl2str(defaultLevel_) 
                          << std::endl;
            
            } else if (tokens[1] == "-sl" && tokens.size() > 2) {
                auto optLevel = VirtualLogger::str2lvl(tokens[2]);

                if (!optLevel) {
                    std::cout << "level " << tokens[2] << " not found!\n";
                    continue;
                }
                defaultLevel_ = optLevel.value();
            } 
            continue;
        }

        if (command == "msg" && tokens.size() > 1) {
            std::size_t offset = input.find(tokens[1]);

            if (tokens.size() > 2) {
                auto optLevel = VirtualLogger::str2lvl(tokens[1]);

                // если передали уровень важности
                if (optLevel) {
                    offset = input.find(tokens[2], offset + tokens[1].size());
                    manager_->pushLog(input.substr(offset), optLevel.value());
                    continue;
                }
            }

            manager_->pushLog(input.substr(offset), defaultLevel_);
            continue;
        }
        
        // если команда не опознана(или было введено только слово «msg»), 
        // то воспринимается как текст с важностью по умолчанию
        manager_->pushLog(input, defaultLevel_);
    }
}