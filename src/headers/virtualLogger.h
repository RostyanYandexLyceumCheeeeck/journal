#pragma once

#include <chrono>
#include <string>
#include <optional>
#include <algorithm>

// макрос экспорта для динамической библиотеки
#define LOG_API __attribute__((visibility("default")))


enum class LOG_API LevelImportance {
    Destroy = -2,
    Delete = -1,
    Low = 0,
    Medium,
    High
};


class LOG_API VirtualLogger {
public:
    virtual ~VirtualLogger() = default;

    virtual VirtualLogger& operator=(const VirtualLogger&) = delete;
    virtual VirtualLogger& operator=(VirtualLogger&&) noexcept = default;

    // setter/getter важности по умолчанию
    virtual void setMinLevel(LevelImportance level) noexcept = 0;
    virtual LevelImportance getMinLevel() const noexcept = 0;

    // операции чтения
    virtual std::string readOffset(std::streampos offset, uint64_t size) = 0;
    virtual std::streampos getEndFile() = 0;

    // операции записи
    virtual void writeToEnd(const std::string& data) = 0;
    virtual void writeBytesOffset(const char* data, std::size_t size, std::streampos offset) = 0;

    // convert [LevelImportance to std::string] and back
    static std::string lvl2str(LevelImportance level) noexcept;
    static std::optional<LevelImportance> str2lvl(std::string level) noexcept;
};

// типы указателей на функции фабрики
using CreateLogger_t = VirtualLogger* (*)(const std::filesystem::path& logPath, LevelImportance minLevel);
using DestroyLogger_t = void (*)(VirtualLogger*);


inline std::string VirtualLogger::lvl2str(LevelImportance level) noexcept {
    switch (level) {
        case LevelImportance::Destroy:  { return "DESTROY"; }
        case LevelImportance::Delete:   { return "DELETE";  }
        case LevelImportance::Low:      { return "LOW";     }
        case LevelImportance::Medium:   { return "MEDIUM";  }
        case LevelImportance::High:     { return "HIGH";    }
        default:                        { return "UNKNOWN"; }
    }
}

inline std::optional<LevelImportance> VirtualLogger::str2lvl(std::string level) noexcept {
    std::transform(level.begin(), level.end(), level.begin(), [](unsigned char c) {
        return std::toupper(c);
    });

    if (level == "LOW")         { return LevelImportance::Low;    }
    else if (level == "MEDIUM") { return LevelImportance::Medium; }
    else if (level == "HIGH")   { return LevelImportance::High;   }
    else                        { return std::nullopt;            }
}
