#include <iostream>
#include <dlfcn.h>

#include "headers/application.h"

#ifndef LIBRARY_PATH
#define LIBRARY_PATH "./bin/libLogger.so"
#endif

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

Application::Application(std::filesystem::path path, LevelImportance minLevel) {
    path_ = std::move(path);

    libHandle_ = dlopen(LIBRARY_PATH, RTLD_LAZY);
    if (!libHandle_) {
        throw std::runtime_error(dlerror());
    }

    // ищем функции создания/удаления
    auto createFunc = reinterpret_cast<CreateLogger_t>(dlsym(libHandle_, "createLogger"));
    auto destroyFunc = reinterpret_cast<DestroyLogger_t>(dlsym(libHandle_, "destroyLogger"));

    if (!createFunc || !destroyFunc) {
        dlclose(libHandle_);
        throw std::runtime_error(dlerror());
    }

    threadLogger_ = std::thread(&Application::logWorker, this);
    instanceLogger_ = std::unique_ptr<VirtualLogger, DestroyLogger_t>(createFunc(path_, minLevel), destroyFunc);
}

Application::~Application() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stopWorker_ = true;
    }
    
    cv_.notify_one();
    if (threadLogger_.joinable()) { threadLogger_.join(); }

    instanceLogger_.reset();
    if (libHandle_) { dlclose(libHandle_); }
}

void Application::logWorker() {
    while (true) {
        std::pair<std::string, LevelImportance> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this]() { 
                return !queue_.empty() || stopWorker_; 
            });

            if (queue_.empty() && stopWorker_) {
                break;
            }

            task = std::move(queue_.front());
            queue_.pop();
        }

        instanceLogger_->log(task.first, task.second);
    }
}

void Application::pushLog(std::string message, LevelImportance level) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.emplace(std::move(message), level);
    }
    cv_.notify_one(); 
}

void Application::pushLog(std::string message) {
    pushLog(message, instanceLogger_->getMinLevel());
}


void Application::printHelp() {
    std::cout << "Available commands:\n"
                << "  help                - Show this message.\n"
                << "  exit                - Exit the program.\n"
                << "  log                 - Show all messages.\n"
                << "  jnl -gl             - Show the current default importance level.\n"
                << "  jnl -sl <level>     - Change the default importance level to <level>.\n"
                << "  msg [level] <text>  - Saves <text> to journal, specifying the importance level. If the [level] is not specified, it is recorded by default\n"
                << "  <text>              - If the first word of the <text> is not a command(or only the word \"msg\" was entered), " 
                <<                          "it will be treated as <text> with no importance level. "
                <<                          "In other words, it is not possible to write an empty string to the journal.\n";
}

void Application::printLog() {
    std::ifstream file(path_);
    
    if (!file.is_open()) {
        std::cout << "Не удалось открыть файл\n";
        return;
    }

    if (file.peek() == std::ifstream::traits_type::eof()) return;

    std::lock_guard<std::mutex> lock(queueMutex_);
    std::cout << file.rdbuf();
    file.close();
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
            std::cout << "Exit from journal!\n";
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

        if (command == "jnl" && tokens.size() > 1) {
            if (tokens[1] == "-gl") { 
                std::cout << " level instance to default:  " 
                          << instanceLogger_->lvl2str(instanceLogger_->getMinLevel()) 
                          << std::endl;
            
            } else if (tokens[1] == "-sl" && tokens.size() > 2) {
                    auto optLevel = instanceLogger_->str2lvl(tokens[2]);
    
                    if (!optLevel) {
                        std::cout << "level " << tokens[2] << " not found!\n";
                        continue;;
                    }
                    instanceLogger_->setMinLevel(optLevel.value());
            
            } 
            continue;
        }

        if (command == "msg" && tokens.size() > 1) {
            std::size_t offset = command.size() + 1;

            if (tokens.size() > 2) {
                auto optLevel = instanceLogger_->str2lvl(tokens[1]);
                
                // если передали уровень важности
                if (optLevel) {
                    offset += tokens[1].size() + 1;
                    pushLog(input.substr(offset), optLevel.value());
                    continue;
                }
            }

            pushLog(input.substr(offset));
            continue;
        }
        // если команда не опознана(или было введено только слово «msg»), 
        // то воспринимается как текст с важностью по умолчанию
        pushLog(input);
    }
}
