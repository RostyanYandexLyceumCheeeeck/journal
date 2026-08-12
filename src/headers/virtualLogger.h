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

namespace NameLevels {
    inline std::string DESTROY = "DESTROY";
    inline std::string DELETE  = "DELETE";
    inline std::string LOW     = "LOW";
    inline std::string MEDIUM  = "MEDIUM";
    inline std::string HIGH    = "HIGH";
    inline std::string UNKNOWN = "UNKNOWN";
}


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
        case LevelImportance::Destroy:  { return NameLevels::DESTROY; }
        case LevelImportance::Delete:   { return NameLevels::DELETE;  }
        case LevelImportance::Low:      { return NameLevels::LOW;     }
        case LevelImportance::Medium:   { return NameLevels::MEDIUM;  }
        case LevelImportance::High:     { return NameLevels::HIGH;    }
        default:                        { return NameLevels::UNKNOWN; }
    }
}

inline std::optional<LevelImportance> VirtualLogger::str2lvl(std::string level) noexcept {
    std::transform(level.begin(), level.end(), level.begin(), [](unsigned char c) {
        return std::toupper(c);
    });

    if (level == NameLevels::LOW)         { return LevelImportance::Low;    }
    else if (level == NameLevels::MEDIUM) { return LevelImportance::Medium; }
    else if (level == NameLevels::HIGH)   { return LevelImportance::High;   }
    else                                  { return std::nullopt;            }
}
