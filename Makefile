# директории
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build
BIN_DIR = bin

# имена итоговых файлов
LIB_NAME = $(BIN_DIR)/libLogger.so
APP_NAME = $(BIN_DIR)/journal_app
TEST_BIN = $(BIN_DIR)/test_app

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Isrc/headers -fPIC -DLIBRARY_PATH='"$(LIB_NAME)"'
LDFLAGS = -shared

# файлы библиотеки
LIB_SRCS = $(SRC_DIR)/logger.cpp
LIB_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(LIB_SRCS))

# файлы приложения
APP_SRCS = $(SRC_DIR)/application.cpp #$(SRC_DIR)/parser.cpp
APP_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(APP_SRCS))

# файл точки входа приложения
MAIN_OBJ = $(BUILD_DIR)/main.o

# тестовые файлы
TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.cpp, $(BUILD_DIR)/test_%.o, $(TEST_SRCS))

.PHONY: all library app test clean

all: library app

# сборка библиотеки
library: $(LIB_NAME)

$(LIB_NAME): $(LIB_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^

# сборка основного приложения
app: $(APP_NAME)

$(APP_NAME): $(APP_OBJS) $(MAIN_OBJ) library
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $(APP_OBJS) $(MAIN_OBJ) -ldl -pthread

# сборка и запуск тестов
test: $(TEST_BIN) library
	./$(TEST_BIN)

# линкуем тесты с объектами приложения
$(TEST_BIN): $(APP_OBJS) $(TEST_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ -ldl -pthread

# компиляция объектных файлов
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) -std=c++17 -Isrc/headers -c $< -o $@

# очистка
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
