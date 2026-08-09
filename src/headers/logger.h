#pragma once

#include <filesystem>
#include <fstream>

#include "virtualLogger.h"


class LOG_API Logger: public VirtualLogger {
    std::filesystem::path path_;
    LevelImportance minLevel_;
    std::ofstream file_;

public:
    Logger(const std::filesystem::path& logPath, LevelImportance minLevel = LevelImportance::Low);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    Logger(Logger&&) noexcept = default;
    Logger& operator=(Logger&&) noexcept = default;

    void setMinLevel(LevelImportance level) noexcept;
    LevelImportance getMinLevel() const noexcept;

    bool log(const std::string& message, LevelImportance level);
    bool log(const std::string& message);

private:
    void writeToFile(const Message& entry);
    std::string formatTime(const std::chrono::system_clock::time_point& tp);
};
