#pragma once

#include <iostream>


#ifndef TEMP_FILE_PATH
#define TEMP_FILE_PATH "build/test/temp_file_tests.txt"
#endif

#ifndef ERROR_FILE_PATH
#define ERROR_FILE_PATH "build/test/test_errors.txt"
#endif

// временный файл для тестов. также фалй для ошибок тестов.
inline std::string fileTemp = TEMP_FILE_PATH;
inline std::string fileErrorPath = ERROR_FILE_PATH;


// константы цветов
constexpr auto RESET  = "\033[0m";
constexpr auto RED    = "\033[31m";
constexpr auto GREEN  = "\033[32m";
constexpr auto YELLOW = "\033[33m";
constexpr auto BLUE   = "\033[34m";


inline void printStatus(std::string nameTest, bool successfull, std::ostream& os = std::cout) {
    if (successfull) {
        os << GREEN << "+----------+" <<                    std::endl
                    << "|    OK    |" << ' ' << nameTest << std::endl 
                    << "+----------+";
    } else {
        os << RED   << "############" <<                    std::endl
                    << "## FAILED ##" << ' ' << nameTest << std::endl 
                    << "############";
    }
    os << RESET << std::endl;
}
