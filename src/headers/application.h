#pragma once

#include "virtualLogManager.h" 


std::vector<std::string> parseCommand(const std::string& input);

class Application {
    VirtualLogManager* manager_;
    LevelImportance defaultLevel_; // Уровень важности, назначаемый по умолчанию

public:
    Application(std::filesystem::path path, LevelImportance minLevel);
    ~Application();

    void run();

private:
    void printHelp();
    void printLog();
};
