# директории
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build
BIN_DIR = bin
FILES_TEST_DIR = $(BUILD_DIR)/test

# имена итоговых файлов
LOGGER_LIB = $(BIN_DIR)/libLogger.so
MANAGER_LIB = $(BIN_DIR)/libManager.so
APP_NAME = $(BIN_DIR)/journal_app
TEST_BIN = $(BIN_DIR)/test_app

# пути файлов для [данных/ошибок] тестов
TEMP_FILE_PATH = $(FILES_TEST_DIR)/temp_file_tests.txt
ERROR_FILE_PATH = $(FILES_TEST_DIR)/test_errors.txt

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I$(SRC_DIR)/headers -fPIC 	\
		   -DTEMP_FILE_PATH=\"$(TEMP_FILE_PATH)\" 				\
           -DERROR_FILE_PATH=\"$(ERROR_FILE_PATH)\"

# флаги линковки
LIB_LDFLAGS = -shared
RPATH_FLAGS = -Wl,-rpath=./$(BIN_DIR)

# файлы исходников
LOGGER_SRCS = $(SRC_DIR)/asyncLogger.cpp 
MANAGER_SRCS = $(SRC_DIR)/logManager.cpp
APP_SRCS = $(SRC_DIR)/application.cpp $(SRC_DIR)/main.cpp

# объектные файлы
LOGGER_OBJS = $(LOGGER_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
MANAGER_OBJS = $(MANAGER_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
APP_OBJS = $(APP_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.cpp, $(BUILD_DIR)/test_%.o, $(TEST_SRCS))

.PHONY: all logger_lib manager_lib app test clean

all: logger_lib manager_lib app

# сборка библиотеки логгера
logger_lib: $(LOGGER_LIB)

$(LOGGER_LIB): $(LOGGER_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LIB_LDFLAGS) -o $@ $^


# сборка библиотеки менеджера
manager_lib: $(MANAGER_LIB)

# линкуем менеджер с логгером
$(MANAGER_LIB): $(MANAGER_OBJS) $(LOGGER_LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LIB_LDFLAGS) -o $@ $(MANAGER_OBJS) -L$(BIN_DIR) -lLogger $(RPATH_FLAGS)


# сборка основного приложения
app: $(APP_NAME)

$(APP_NAME): $(APP_OBJS) $(LOGGER_LIB) $(MANAGER_LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(APP_OBJS) -o $@ -L$(BIN_DIR) -lManager -lLogger $(RPATH_FLAGS)


# сборка и запуск тестов
test: $(TEST_BIN) logger_lib manager_lib
	@mkdir -p $(FILES_TEST_DIR)
	./$(TEST_BIN)

# линкуем тесты со всеми нужными объектами и библиотеками
$(TEST_BIN): $(BUILD_DIR)/application.o $(TEST_OBJS) logger_lib manager_lib
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $(BUILD_DIR)/application.o $(TEST_OBJS) -L$(BIN_DIR) -lManager -lLogger $(RPATH_FLAGS) -pthread


# компиляция объектных файлов
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@


# очистка
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)