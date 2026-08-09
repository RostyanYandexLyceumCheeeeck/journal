#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <queue>

#include <condition_variable>
#include <atomic>
#include <thread>
#include <mutex>

#include "virtualLogger.h"


std::vector<std::string> parseCommand(const std::string& input);


class Application {
    std::filesystem::path path_;

    void* libHandle_ = nullptr;                                                        // xэндл загруженной библиотеки
    std::unique_ptr<VirtualLogger, DestroyLogger_t> instanceLogger_{nullptr, nullptr}; // указатель на реализацию логгера 

    std::queue<std::pair<std::string, LevelImportance>> queue_; // очередь сообщений
    std::mutex queueMutex_;                                     // защита очереди
    std::thread threadLogger_;                                  // отдельный поток для логгера
    std::condition_variable cv_;                                // для пробуждения фонового потока
    std::atomic<bool> stopWorker_{false};                       // флаг завершения работы

public:
    Application(std::filesystem::path path, LevelImportance minLevel);
    ~Application();

    void run();

private:
    void logWorker();
    void pushLog(std::string message, LevelImportance level);
    void pushLog(std::string message);

    void printHelp();
    void printLog();
};
