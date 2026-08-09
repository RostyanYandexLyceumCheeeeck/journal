#include "headers/logger.h"


Logger::Logger(const std::filesystem::path& logPath, LevelImportance minLevel): path_(logPath), 
                                                                                minLevel_(minLevel), 
                                                                                file_(logPath, std::ios::app) {
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + logPath.string());
    }
}


Logger::~Logger() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}


void Logger::setMinLevel(LevelImportance level) noexcept {
    minLevel_ = level;
}


LevelImportance Logger::getMinLevel() const noexcept {
    return minLevel_;
}


bool Logger::log(const std::string& message, LevelImportance level) {
    if (static_cast<int>(level) < static_cast<int>(minLevel_)) {
        return false;
    }

    Message entry {
        level,
        std::chrono::system_clock::now(),
        std::string(message)
    };

    writeToFile(entry);
    return true;
}


bool Logger::log(const std::string& message) {
    return log(message, getMinLevel());
}


void Logger::writeToFile(const Message& entry) {
    file_ << "[" << formatTime(entry.timestamp) << "] "
          << "[" << lvl2str(entry.level) << "]" << std::endl
          << entry.message << std::endl;
    file_.flush();
}


std::string Logger::formatTime(const std::chrono::system_clock::time_point& tp) {
    std::tm tm_buf{};
    std::ostringstream ss;
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);    

    localtime_r(&time_t_val, &tm_buf);
    ss << std::put_time(&tm_buf, "%d-%m-%Y %H:%M:%S");
    return ss.str();
}


extern "C" {
    LOG_API VirtualLogger* createLogger(const std::filesystem::path& logPath, 
                                        LevelImportance minLevel = LevelImportance::Low) { 
        return new Logger(logPath, minLevel); 
    }

    LOG_API void destroyLogger(VirtualLogger* p) { delete p; }
}