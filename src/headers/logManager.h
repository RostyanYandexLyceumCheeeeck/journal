#pragma once

#include <map>
#include <list>
#include <vector>
#include <cstring>
#include <iterator>
#include <sstream>
#include <iomanip>

#include "virtualLogManager.h"


// формат строки заголовка:
// [dd-mm-yyyy hh:mm:ss] [level] [index] [size] [before] [after]\n
// каждый из level/index/size/before/after занимает по 20 символов
namespace LogFormat {
    // внутренняя длина полей без учета скобок
    inline std::size_t LEVEL_LEN  = 7;
    inline std::size_t TIME_LEN   = 19;
    inline std::size_t NUM_LEN    = 20;

    // смещения от начала строки до начала данных
    inline std::size_t TIME_OFFSET   = 1;
    inline std::size_t LEVEL_OFFSET  = TIME_OFFSET + TIME_LEN + 3;     // 23 
    inline std::size_t INDEX_OFFSET  = LEVEL_OFFSET + LEVEL_LEN + 3;   // 33
    inline std::size_t SIZE_OFFSET   = INDEX_OFFSET + NUM_LEN + 3;     // 56
    inline std::size_t BEFORE_OFFSET = SIZE_OFFSET + NUM_LEN + 3;      // 79
    inline std::size_t AFTER_OFFSET  = BEFORE_OFFSET + NUM_LEN + 3;    // 102

    // общий размер вместе с '\n'
    inline std::size_t HEADER_LEN = AFTER_OFFSET + NUM_LEN + 2;        // 124
    
    // для HeaderMap
    inline std::size_t MAP_NUMBER_OFFSET = 1;
    inline std::size_t MAP_FIRST_OFFSET  = MAP_NUMBER_OFFSET + NUM_LEN + 3;  // 24 
    inline std::size_t MAP_LAST_OFFSET   = MAP_FIRST_OFFSET + NUM_LEN + 3;   // 47
    inline std::size_t MAP_BASKET_OFFSET = MAP_LAST_OFFSET + NUM_LEN + 3;    // 70
    inline std::size_t MAP_FREE_OFFSET   = MAP_BASKET_OFFSET + NUM_LEN + 3;  // 93
    
    // общий размер вместе с '\n'
    inline std::size_t MAP_LEN = MAP_FREE_OFFSET + NUM_LEN + 2;              // 115

    inline std::string appendSpace(std::string st, size_t len) { 
            return st + std::string(len - st.size(), ' '); 
        };

    // функция для обновления адресов [before]/[after] по адресу
    inline std::string formatPos(std::streampos pos) {
        std::ostringstream oss;
        oss << appendSpace(smp2str(pos), LogFormat::NUM_LEN);
        
        return oss.str();
    }

    inline std::string formatLevel(LevelImportance level) {
        std::ostringstream oss;
        oss << appendSpace(VirtualLogger::lvl2str(level), LogFormat::LEVEL_LEN);
        
        return oss.str();
    }
    
    inline std::string formatNumber(std::size_t number) {
        std::ostringstream oss;
        oss << appendSpace(std::to_string(number), LogFormat::NUM_LEN);
        
        return oss.str();
    }

    inline std::string formatTime(const std::chrono::system_clock::time_point& tp) {
        std::tm tm_buf{};
        std::ostringstream ss;
        auto time_t_val = std::chrono::system_clock::to_time_t(tp);    

        localtime_r(&time_t_val, &tm_buf);
        ss << std::put_time(&tm_buf, "%d-%m-%Y %H:%M:%S");
        return ss.str();
    }

    inline std::string entryHead2str(const logEntry& entry) {
        std::ostringstream oss;        
        oss << "[" << formatTime(entry.message.timestamp) << "] "
            << "[" << formatLevel(entry.message.level) << "] "
            << "[" << formatNumber(entry.index) << "] "
            << "[" << formatNumber(entry.size) << "] "
            << "[" << formatPos(entry.before) << "] "
            << "[" << formatPos(entry.after) << "]\n";
            
        return oss.str();
    }
}


class LOG_API LogManager: public VirtualLogManager {
    HeaderMap map_;
    std::streampos endFile_;
    std::size_t sizeWindow_;
    std::size_t currLogEntry_;   // индекс первого элемента "видимого" окна внутри cache_
    std::list<logEntry> cache_;  // размер до 3 * sizeWindow_
    std::list<logEntry> basket_;                  
    std::multimap<std::size_t, std::streampos> free_;  
    
    VirtualLogger* logger_;
    const std::streampos NPOS = static_cast<std::streampos>(-1);

public:
    LogManager(const std::filesystem::path& logPath,LevelImportance minLevel = LevelImportance::Low, size_t sizeWindow = 3);
    ~LogManager() override;

    void pushLog(const std::string& message, LevelImportance level) override;
    void pushLog(const std::string& message) override;
    
    void setMinLevel(LevelImportance level) noexcept override;
    LevelImportance getMinLevel() const noexcept override;

    void stepWindow(int count = 1) override;

    void deleteLog(std::size_t index) override;
    void destroyLog(std::size_t index) override;

    std::vector<logEntry> getWindowLogs() override;
    std::string showEntry(const logEntry& entry) override;
private:
    void init();
    void saveMap();

    void forwardWindow(std::size_t count = 1);
    void backwardWindow(std::size_t count = 1);

    // подгрузка элементов с диска вперёд/назад (к новым/старым логам)
    bool loadMoreForward(std::size_t amount);
    bool loadMoreBackward(std::size_t amount);

    // парсер записи
    std::optional<logEntry> parseEntry(std::streampos offset);

    // функция связки двух записей
    void relinkNeighbors(std::streampos before, std::streampos after);
};