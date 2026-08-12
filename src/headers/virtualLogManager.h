#pragma once

#include <filesystem>
#include <optional>
#include <vector>
#include <string>
#include <chrono>
#include <functional>

#include "virtualLogger.h"


inline std::string smp2str(const std::streampos& pos) {
    return std::to_string(static_cast<uint64_t>(pos));
} 

struct LOG_API Message {
    LevelImportance level;
    std::chrono::system_clock::time_point timestamp;
    std::string message;
};

struct LOG_API HeaderMap {
    uint64_t numberElements;  
    std::streampos first;     
    std::streampos last;      
    std::streampos basket;    
    std::streampos free;      
};

struct LOG_API logEntry {
    std::streampos before;  // адрес в файле предыдущей записи
    std::streampos after;   // адрес в файле следующей записи
    std::size_t index;      // индекс(номер) сообщения
    std::size_t size;       // размер сообщения
    Message message;
    std::streampos self;    // адрес в файле этой записи
};


class LOG_API VirtualLogManager {
public:
    virtual ~VirtualLogManager() = default;

    virtual void pushLog(const std::string& message, LevelImportance level) = 0;
    virtual void pushLog(const std::string& message) = 0;

    // setter/getter важности по умолчанию
    virtual void setMinLevel(LevelImportance level) noexcept = 0;
    virtual LevelImportance getMinLevel() const noexcept = 0;

    virtual void stepWindow(int count = 1) = 0;

    // index -- это индекс относительно "видимого" окна в консоли (0 .. sizeWindow_-1)
    virtual void deleteLog(std::size_t index) = 0;
    virtual void destroyLog(std::size_t index) = 0;

    // для отрисовки 
    virtual std::vector<logEntry> getWindowLogs() = 0;
    virtual std::string showEntry(const logEntry& entry) = 0;
};

using CreateLogManager_t = std::function<VirtualLogManager*(const std::filesystem::path& logPath, LevelImportance minLevel, size_t sizeWindow)>;
using DestroyLogManager_t = std::function<void(VirtualLogManager*)>;