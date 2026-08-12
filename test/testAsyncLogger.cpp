#include <iostream>
#include <functional>

#include "testSources.h"
#include "asyncLogger.h"


extern CreateLogger_t createAsyncLogger;
extern DestroyLogger_t destroyAsyncLogger;


#define ASSERT_TEST(cond) { if (!(cond)) { throw std::runtime_error("Runtime Error in tests!"); } }

// фабрика тестов. при ошибке выводит в fileErrorPath.
void FabricTestsAsyncLogger(std::function<void(std::filesystem::path)> func, std::filesystem::path path, std::string nameFunc) {
    try {
        std::ofstream emptyFile(path, std::ios::trunc);
        if (emptyFile.is_open()) { emptyFile.close(); }

        func(path);
        printStatus(nameFunc, true);
    
    } catch (const std::exception& e) {
        printStatus(nameFunc, false);
        
        std::ofstream fileError(fileErrorPath, std::ios::app);
        if (fileError.is_open()) { fileError << "[ " << nameFunc << " ]: " << e.what() << "\n\n"; }
    }
}


// тесты конвертации уровней
void levelConversionsTests() {
    // lvl2str
    ASSERT_TEST(VirtualLogger::lvl2str(LevelImportance::Destroy) == NameLevels::DESTROY);
    ASSERT_TEST(VirtualLogger::lvl2str(LevelImportance::Delete) == NameLevels::DELETE);
    ASSERT_TEST(VirtualLogger::lvl2str(LevelImportance::Low) == NameLevels::LOW);
    ASSERT_TEST(VirtualLogger::lvl2str(LevelImportance::Medium) == NameLevels::MEDIUM);
    ASSERT_TEST(VirtualLogger::lvl2str(LevelImportance::High) == NameLevels::HIGH);
    ASSERT_TEST(VirtualLogger::lvl2str(static_cast<LevelImportance>(999)) == NameLevels::UNKNOWN);

    // str2lvl (регистр не важен)
    ASSERT_TEST(VirtualLogger::str2lvl("low") == LevelImportance::Low);
    ASSERT_TEST(VirtualLogger::str2lvl("MeDiUm") == LevelImportance::Medium);
    ASSERT_TEST(VirtualLogger::str2lvl("HIGH") == LevelImportance::High);
    ASSERT_TEST(!VirtualLogger::str2lvl("invalid_str").has_value());
    ASSERT_TEST(!VirtualLogger::str2lvl(NameLevels::DESTROY).has_value());
}

// тестирование уровней логирования и жизненного цикла файла
void getterSetterMinLevelTest(std::filesystem::path path) {
    {
        AsyncLogger logger(path, LevelImportance::High);
        ASSERT_TEST(logger.getMinLevel() == LevelImportance::High);
        
        logger.setMinLevel(LevelImportance::Low);
        ASSERT_TEST(logger.getMinLevel() == LevelImportance::Low);

        logger.setMinLevel(LevelImportance::Low);
        ASSERT_TEST(logger.getMinLevel() == LevelImportance::Low);

        logger.setMinLevel(LevelImportance::Medium);
        ASSERT_TEST(logger.getMinLevel() == LevelImportance::Medium);
}
}

// тестирование записи по смещению (writeBytesOffset)
void writeBytesOffsetTest(std::filesystem::path path) {

    {
        AsyncLogger logger(path);
        logger.writeToEnd("qqqqqwwwww");
        
        // перезаписываем "wwwww" на "eeeee" с офсета 5
        const std::string patch = "eeeee";
        logger.writeBytesOffset(patch.data(), patch.size(), 5);

    }

    {
        AsyncLogger logger(path);
        std::string result = logger.readOffset(0, 10);
        ASSERT_TEST(result == "qqqqqeeeee");
    }
}

// тестирование фабричных функций
void factoryFunctionsTest(std::filesystem::path path) {

    VirtualLogger* logger = createAsyncLogger(path, LevelImportance::Medium);
    ASSERT_TEST(logger != nullptr);
    ASSERT_TEST(logger->getMinLevel() == LevelImportance::Medium);

    logger->writeToEnd("qeewqeqeqeqeq");
    destroyAsyncLogger(logger);

    ASSERT_TEST(std::filesystem::file_size(path) > 0);
}


void runAsyncLoggerTests() {
    std::cout << YELLOW << "--- Running AsyncLogger Tests ---" << RESET << "\n";
    std::filesystem::path path = fileTemp;

    FabricTestsAsyncLogger([](std::filesystem::path){ levelConversionsTests(); }, path, "levelConversionsTests");
    FabricTestsAsyncLogger(getterSetterMinLevelTest, path, "getterSetterMinLevelTest");
    FabricTestsAsyncLogger(writeBytesOffsetTest, path, "writeBytesOffsetTest");
    FabricTestsAsyncLogger(factoryFunctionsTest, path, "factoryFunctionsTest");

    std::cout << YELLOW << "--- AsyncLogger tests passed successfully! ---" << RESET << "\n";
}