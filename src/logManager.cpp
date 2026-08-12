#include "headers/logManager.h"


extern CreateLogger_t createAsyncLogger;
extern DestroyLogger_t destroyAsyncLogger;


LogManager::LogManager(const std::filesystem::path& logPath,
                       LevelImportance minLevel, 
                       size_t sizeWindow): sizeWindow_(sizeWindow), currLogEntry_(0) {
    
    logger_ = createAsyncLogger(logPath, minLevel);
    init();
}

LogManager::~LogManager() { 
    saveMap(); 
    if (logger_) { destroyAsyncLogger(logger_); }
}

void LogManager::pushLog(const std::string& message, LevelImportance level) {
    if (static_cast<int>(level) < static_cast<int>(getMinLevel())) { return; }
    
    auto now = std::chrono::system_clock::now();
    Message temp{.level = level, .timestamp = now, .message = message};
    
    logEntry entry = {
        .before = map_.last,
        .after = NPOS,
        .index = map_.numberElements,
        .size = message.size(),
        .message = temp,
        .self = NPOS
    };

    std::streampos newOffset = NPOS;

    // ищем первый подходящий по размеру свободный блок
    auto it = free_.lower_bound(entry.size);
    
    if (it != free_.end()) {
        newOffset = it->second;
        auto opt = parseEntry(newOffset);
        
        if (!opt) { newOffset = NPOS; }
        else {
            if (map_.free == newOffset) { map_.free = opt->before; } 
            else {
                for (const auto& [tmp, offset] : free_) {
                    auto parentOpt = parseEntry(offset);
                    if (parentOpt && parentOpt->before == newOffset) {
                        // перезаписываем before у родителя
                        logger_->writeBytesOffset(LogFormat::formatPos(opt->before).c_str(), 
                                                    LogFormat::NUM_LEN, 
                                                    offset + static_cast<std::streamoff>(LogFormat::BEFORE_OFFSET));
                        break;
                    }
                }
            }
            entry.index = opt->index;
            if (opt->index == map_.numberElements) { opt->index++; }
            free_.erase(it);

            // проверяем, мб хватает места на новый заголовок и свободного места
            if (opt->size - entry.size > LogFormat::HEADER_LEN) {
                std::size_t remainSize = opt->size - entry.size - LogFormat::HEADER_LEN;
                std::streampos remainOffset = newOffset + 
                                              static_cast<std::streamoff>(LogFormat::HEADER_LEN + entry.size + 1);

                logEntry remainEntry = {
                    .before = map_.free, // вставляем в голову списка свободных
                    .after = NPOS,
                    .index = opt->index,
                    .size = remainSize,
                    .message = {.level = LevelImportance::Destroy, .timestamp = now, .message = "."},
                    .self = remainOffset
                };

                std::string remainHeader = LogFormat::entryHead2str(remainEntry);
                logger_->writeBytesOffset(remainHeader.c_str(), remainHeader.size(), remainOffset);

                free_.insert({remainSize, remainOffset});
                map_.free = remainOffset;
            }

            std::string fullEntry = LogFormat::entryHead2str(entry) + message + '\n';
            logger_->writeBytesOffset(fullEntry.c_str(), fullEntry.size(), newOffset);
        }
    }

    // если подходящего свободного блока не нашлось, то дописываем в конец файла
    if (newOffset == NPOS) {
        std::string fullEntry = LogFormat::entryHead2str(entry) + message + '\n';
        logger_->writeToEnd(fullEntry);
        newOffset = endFile_;
        endFile_ += static_cast<std::streamoff>(fullEntry.size());
        entry.self = newOffset;
        map_.numberElements++;
    }

    if (map_.last == NPOS) { map_.first = newOffset; } 
    else { logger_->writeBytesOffset(LogFormat::formatPos(newOffset).c_str(), 
                                    LogFormat::NUM_LEN, 
                                    map_.last + static_cast<std::streamoff>(LogFormat::AFTER_OFFSET)); 
    }
    
    map_.last = newOffset;
    if (cache_.size() < sizeWindow_) { cache_.push_back(entry); }
}

void LogManager::pushLog(const std::string& message) {
    pushLog(message, logger_->getMinLevel());
}

void LogManager::setMinLevel(LevelImportance level) noexcept {
    return logger_->setMinLevel(level);
}

LevelImportance LogManager::getMinLevel() const noexcept {
    return logger_->getMinLevel();
}

void LogManager::stepWindow(int count) {
    if (count > 0) { forwardWindow(count); }
    else if (count < 0) { backwardWindow(std::abs(count)); }
}

void LogManager::deleteLog(std::size_t index) {
    if (index >= sizeWindow_ || currLogEntry_ + index >= cache_.size()) { return; }
    
    auto it = std::next(cache_.begin(), currLogEntry_ + index);
    std::streampos targetOffset = it->self;
    
    // изменяем уровень на delete 
    logger_->writeBytesOffset(LogFormat::formatLevel(LevelImportance::Delete).c_str(), 
                              LogFormat::LEVEL_LEN, 
                              targetOffset + std::streampos(LogFormat::LEVEL_OFFSET));
    
    // меняем before адресом корзины
    logger_->writeBytesOffset(LogFormat::formatPos(map_.basket).c_str(), 
                              LogFormat::NUM_LEN, 
                              targetOffset + std::streampos(LogFormat::BEFORE_OFFSET));
    map_.basket = targetOffset;

    relinkNeighbors(it->before, it->after);
    
    cache_.erase(it);
    loadMoreForward(1); 
}

void LogManager::destroyLog(std::size_t index) {
    if (index >= sizeWindow_ || currLogEntry_ + index >= cache_.size()) { return; }
    
    auto it = std::next(cache_.begin(), currLogEntry_ + index);
    std::streampos targetOffset = it->self;
    
    logger_->writeBytesOffset(LogFormat::formatLevel(LevelImportance::Destroy).c_str(), 
                              LogFormat::LEVEL_LEN, 
                              targetOffset + std::streampos(LogFormat::LEVEL_OFFSET));
    
    logger_->writeBytesOffset(LogFormat::formatPos(map_.free).c_str(), 
                              LogFormat::NUM_LEN, 
                              targetOffset + std::streampos(LogFormat::BEFORE_OFFSET));
    
    map_.free = targetOffset;
    relinkNeighbors(it->before, it->after);
    
    cache_.erase(it);
    loadMoreForward(1);
    saveMap();
}

std::vector<logEntry> LogManager::getWindowLogs() {
    std::vector<logEntry> entries;
    auto it = cache_.begin();
    std::advance(it, currLogEntry_);
    
    for (size_t i = 0; i < sizeWindow_ && it != cache_.end(); ++i, ++it) {
        entries.push_back(*it);
    }

    return entries;
}

std::string LogManager::showEntry(const logEntry& entry) {
    return "[" + LogFormat::formatTime(entry.message.timestamp) + "] " +
            "[" + VirtualLogger::lvl2str(entry.message.level) + "]\n" +
            entry.message.message + "\n";
}

void LogManager::init() {
    std::size_t size = LogFormat::MAP_LEN;
    std::string buffer = logger_->readOffset(0, size);

    if (buffer.size() < size) { 
        map_ = {0, NPOS, NPOS, NPOS, NPOS};
        saveMap();
        endFile_ = static_cast<std::streampos>(size); 
    } else {
        map_.numberElements = std::stoull(buffer.substr(1, LogFormat::NUM_LEN));
        map_.first  = static_cast<std::streampos>(std::stoull(buffer.substr(LogFormat::MAP_FIRST_OFFSET, 
                                                                            LogFormat::NUM_LEN)));
        map_.last   = static_cast<std::streampos>(std::stoull(buffer.substr(LogFormat::MAP_LAST_OFFSET, 
                                                                            LogFormat::NUM_LEN)));
        map_.basket = static_cast<std::streampos>(std::stoull(buffer.substr(LogFormat::MAP_BASKET_OFFSET, 
                                                                            LogFormat::NUM_LEN)));
        map_.free   = static_cast<std::streampos>(std::stoull(buffer.substr(LogFormat::MAP_FREE_OFFSET, 
                                                                            LogFormat::NUM_LEN)));
    
        endFile_ = logger_->getEndFile();
    }

    // первоначальное заполнение кэша с конца файла
    std::streampos curr = map_.last;
    size_t loaded = 0;
    
    while (curr != NPOS && loaded < sizeWindow_ * 3) {
        auto optEntry = parseEntry(curr);
        if (optEntry) {
            cache_.push_front(optEntry.value());
            loaded++;
        }
        curr = optEntry ? optEntry->before : NPOS; 
    }
    
    // видимое окно находится в самом конце кэша (т.е. самые новые логи)
    currLogEntry_ = (cache_.size() > sizeWindow_) ? (cache_.size() - sizeWindow_) : 0;
}

void LogManager::saveMap() {
    std::ostringstream oss;
    
    oss << "[" << LogFormat::formatNumber(map_.numberElements) << "] "
        << "[" << LogFormat::formatPos(map_.first) << "] "
        << "[" << LogFormat::formatPos(map_.last) << "] "
        << "[" << LogFormat::formatPos(map_.basket) << "] "
        << "[" << LogFormat::formatPos(map_.free) << "]\n";

    std::string header = oss.str();
    logger_->writeBytesOffset(header.c_str(), header.size(), 0);
}

void LogManager::forwardWindow(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (currLogEntry_ + sizeWindow_ < cache_.size()) {
            currLogEntry_++;
        } else {
            if (loadMoreForward(1)) { currLogEntry_++; } 
            else { break; }
        }
    }
}

void LogManager::backwardWindow(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (currLogEntry_ > 0) {
            currLogEntry_--;
        } else {
            if (loadMoreBackward(1)) { currLogEntry_--; } 
            else { break; }
        }
    }
}

bool LogManager::loadMoreForward(std::size_t amount) {
    if (cache_.empty()) { return false; }

    std::streampos curr = cache_.back().after;
    size_t loaded = 0;

    while (curr != NPOS && loaded < amount) {
        auto optEntry = parseEntry(curr);
        if (optEntry) {
            cache_.push_back(optEntry.value());
            loaded++;
        }
        curr = optEntry ? optEntry->after : NPOS;

        if (cache_.size() > sizeWindow_ * 3) { 
            cache_.pop_front();
            if (currLogEntry_ > 0) { currLogEntry_--; }
        }
    }

    return loaded > 0;
}

bool LogManager::loadMoreBackward(std::size_t amount) {
    if (cache_.empty()) { return false; }
    
    std::streampos curr = cache_.front().before;
    size_t loaded = 0;

    while (curr != NPOS && loaded < amount) {
        auto optEntry = parseEntry(curr);
        if (optEntry) {
            cache_.push_front(optEntry.value());
            loaded++;
            currLogEntry_++; 
        }
        curr = optEntry ? optEntry->before : NPOS;

        if (cache_.size() > sizeWindow_ * 3) { cache_.pop_back(); }
    }
    
    return loaded > 0;
}

std::optional<logEntry> LogManager::parseEntry(std::streampos offset) {
    std::string header = logger_->readOffset(offset, LogFormat::HEADER_LEN);
    if (header.size() < LogFormat::HEADER_LEN) { return std::nullopt; }

    logEntry entry;
    entry.self = offset;
    
    std::string lvlStr = header.substr(LogFormat::LEVEL_OFFSET, LogFormat::LEVEL_LEN);
    lvlStr.erase(std::remove(lvlStr.begin(), lvlStr.end(), ' '), lvlStr.end());    

    auto lvl = VirtualLogger::str2lvl(lvlStr);
    if (!lvl) { return std::nullopt; }

    entry.message.level = lvl.value();
    entry.index  = std::stoull(header.substr(LogFormat::INDEX_OFFSET, LogFormat::NUM_LEN));
    entry.size   = std::stoull(header.substr(LogFormat::SIZE_OFFSET, LogFormat::NUM_LEN));
    entry.message.message = logger_->readOffset(offset + 
                                               static_cast<std::streamoff>(LogFormat::HEADER_LEN), entry.size);

    entry.before = static_cast<std::streampos>(std::stoull(header.substr(LogFormat::BEFORE_OFFSET, 
                                                                         LogFormat::NUM_LEN)));
    entry.after  = static_cast<std::streampos>(std::stoull(header.substr(LogFormat::AFTER_OFFSET, 
                                                                         LogFormat::NUM_LEN)));
    
    return entry;
} 

void LogManager::relinkNeighbors(std::streampos before, std::streampos after) {
    if (before != NPOS) {
        logger_->writeBytesOffset(LogFormat::formatPos(after).c_str(), 
                                  LogFormat::NUM_LEN, 
                                  before + std::streampos(LogFormat::AFTER_OFFSET));
    } else {
        map_.first = after;
    }

    if (after != NPOS) {
        logger_->writeBytesOffset(LogFormat::formatPos(before).c_str(), 
                                  LogFormat::NUM_LEN, 
                                  after + std::streampos(LogFormat::BEFORE_OFFSET));
    } else {
        map_.last = before;
    }
}

// для динамической библиотеки
LOG_API CreateLogManager_t createLogManager = [](const std::filesystem::path& logPath, 
                                                 LevelImportance minLevel, 
                                                 size_t sizeWindow) -> VirtualLogManager* {
    
    return new LogManager(logPath, minLevel, sizeWindow);
};

LOG_API DestroyLogManager_t destroyLogManager = [](VirtualLogManager* manager) {
    delete manager;
};