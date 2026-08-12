#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <queue>

#include <condition_variable>
#include <shared_mutex>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#include "virtualLogger.h"


class LOG_API AsyncLogger : public VirtualLogger {
    std::filesystem::path path_;
    LevelImportance minLevel_;

    std::queue<std::function<void()>> queueTasks_; // очередь задач для писателя
    std::mutex queueMutex_;               // мьютекс для защиты очереди задач
    std::thread workerThread_;            // фоновый поток записи

    mutable std::shared_mutex rwm_;       // read-write mutex
    std::condition_variable cv_;          // для пробуждения потока
    std::atomic<bool> stopFlag_{false};   // флаг остановки фонового потока


public:
    AsyncLogger(const std::filesystem::path& logPath, LevelImportance minLevel = LevelImportance::Low);
    ~AsyncLogger() override;
    
    using VirtualLogger::operator=;
    
    void setMinLevel(LevelImportance level) noexcept override;
    LevelImportance getMinLevel() const noexcept override;

    // операции чтения
    std::string readSector(std::streampos start, std::streampos end);
    std::string readOffset(std::streampos offset, uint64_t size) override;
    std::streampos getEndFile() override;

    // операции записи
    void writeToEnd(const std::string& data) override;
    void writeBytesOffset(const char* data, std::size_t size, std::streampos offset) override;

private:
    // передача задачи записи в очередь
    void submitTask(std::function<void()> task);

    // внутренние блоки кода для записи
    void writeOffsetBlock(std::streampos offset, const std::string& data);
    void appendBlock(const std::string& data);

    void workerMain();
};