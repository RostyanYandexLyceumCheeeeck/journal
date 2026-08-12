#include "headers/asyncLogger.h"


AsyncLogger::AsyncLogger(const std::filesystem::path& logPath, 
                         LevelImportance minLevel): path_(logPath), minLevel_(minLevel) {

    std::ofstream file(path_, std::ios::app);
    if (!file.is_open()) { 
        throw std::runtime_error("Failed to open log file: " + logPath.string()); 
    }
    file.close();
    
    workerThread_ = std::thread(&AsyncLogger::workerMain, this);
}

AsyncLogger::~AsyncLogger() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stopFlag_ = true;
    }
    cv_.notify_one(); 
    if (workerThread_.joinable()) { workerThread_.join(); }
}

void AsyncLogger::setMinLevel(LevelImportance level) noexcept { 
    minLevel_ = level; 
}

LevelImportance AsyncLogger::getMinLevel() const noexcept { 
    return minLevel_; 
}

std::string AsyncLogger::readSector(std::streampos start, std::streampos end) {
    if (start >= end) { return ""; }
    std::shared_lock<std::shared_mutex> lock(rwm_);
    
    std::ifstream file(path_, std::ios::in | std::ios::binary);
    if (!file.is_open()) { return ""; }
    
    std::string buffer;
    auto length = end - start;
    buffer.resize(length);

    file.seekg(start);
    if (!file.read(&buffer[0], buffer.size())) { buffer.resize(file.gcount()); }
    
    return buffer;
}

std::string AsyncLogger::readOffset(std::streampos offset, uint64_t size) {
    return readSector(offset, offset + static_cast<std::streamoff>(size));
}

std::streampos AsyncLogger::getEndFile() {
    std::unique_lock<std::shared_mutex> lock(rwm_);
    std::ifstream file(path_, std::ios::binary);
    
    file.seekg(0, std::ios::end);
    return file.tellg();
}


void AsyncLogger::submitTask(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queueTasks_.push(std::move(task));
    cv_.notify_one();
}

void AsyncLogger::workerMain() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this]() { return !queueTasks_.empty() || stopFlag_; });
            
            if (queueTasks_.empty() && stopFlag_) { break; }

            task = std::move(queueTasks_.front());
            queueTasks_.pop();
        }

        if (task) {
            std::unique_lock<std::shared_mutex> lock(rwm_);
            task();
        }
    }
}


void AsyncLogger::writeBytesOffset(const char* data, std::size_t size, std::streampos offset) {
    std::string buffer(data, size);
    submitTask([this, offset, buffer]() {
        writeOffsetBlock(offset, buffer);
    });
}

void AsyncLogger::writeOffsetBlock(std::streampos offset, const std::string& data) {
    std::fstream file(path_, std::ios::in | std::ios::out | std::ios::binary);
    file.seekp(offset, std::ios::beg);
    file.write(data.c_str(), data.size());
}


void AsyncLogger::writeToEnd(const std::string& data) {
    submitTask([this, data]() {
        appendBlock(data);
    });
}

void AsyncLogger::appendBlock(const std::string& data) {
    std::fstream file(path_, std::ios::in | std::ios::out | std::ios::ate | 
                                                            std::ios::binary);

    file.write(data.c_str(), data.size());
    return;
}

LOG_API CreateLogger_t createAsyncLogger = [](const std::filesystem::path& logPath, LevelImportance minLevel) -> VirtualLogger* {
    return new AsyncLogger(logPath, minLevel);
};

LOG_API DestroyLogger_t destroyAsyncLogger = [](VirtualLogger* logger) {
    delete logger;
};