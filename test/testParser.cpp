#include <iostream>
#include <cassert>


#include "application.h"

// константы цветов
constexpr auto RESET  = "\033[0m";
constexpr auto RED    = "\033[31m";
constexpr auto GREEN  = "\033[32m";
constexpr auto YELLOW = "\033[33m";
constexpr auto BLUE   = "\033[34m";


void printStatus(std::string nameTest, bool successfull) {
    if (successfull) {
        std::cout << GREEN << "+----------+" <<                    std::endl
                           << "|    OK    |" << ' ' << nameTest << std::endl 
                           << "+----------+";
    } else {
        std::cout << RED   << "############" <<                    std::endl
                           << "## FAILED ##" << ' ' << nameTest << std::endl 
                           << "############";
    }
    std::cout << RESET << std::endl;
}

using vecStr = std::vector<std::string>; 
class ParserCommandTest {
    vecStr input_ = {};
    vecStr output_ = {};
    vecStr answer_ = {};
    std::string st_ = "";

public:
    ParserCommandTest(vecStr arrInput, vecStr arrAnswer): input_(arrInput),
                                                          answer_(arrAnswer) {}

    bool successfull() {
        concatenateArr();
        output_ = parseCommand(st_);
        return output_ == answer_;
    }

private:
    void concatenateArr() {
        if (input_.empty()) { return; }
        for (auto i: input_) { st_ += i + ' '; }
        st_.pop_back(); // delete last space
    }
};


void FabricTestsParser(std::string nameTest, vecStr arrInput, vecStr arrAnswer) {
    ParserCommandTest test(arrInput, arrAnswer);
    
    bool result = test.successfull();
    printStatus(nameTest, result); 
    assert(result);
}


void parserTests() {
    // тесты на пустую строку
    FabricTestsParser("EmptyStringTest_01", {""},                                        {});
    FabricTestsParser("EmptyStringTest_02", {" "},                                       {});
    FabricTestsParser("EmptyStringTest_03", {"    ", " ", "", "    "},                   {});
    FabricTestsParser("EmptyStringTest_04", {"    ", " ", "", "    ", "\n \t \n \n \n"}, {});
    
    // тесты на скобки
    FabricTestsParser("QuotesTest_01", {"\"zxc\""},           {"zxc"}          );
    FabricTestsParser("QuotesTest_02", {"\" zxc  \""},        {" zxc  "}       );
    FabricTestsParser("QuotesTest_03", {"\" zxc  zxc\""},     {" zxc  zxc"}    );
    FabricTestsParser("QuotesTest_04", {"\" zxc  \n\tzxc\""}, {" zxc  \n\tzxc"});
    
    // тесты на то, чтоб был только 1 токен
    FabricTestsParser("OneTokenTest_01", {"msg"},              {"msg"});
    FabricTestsParser("OneTokenTest_02", {"msg",  ""},         {"msg"});
    FabricTestsParser("OneTokenTest_03", {"\n", "msg", "   "}, {"msg"});
    
    // тесты на то, чтоб парсер увидел несколько токенов
    FabricTestsParser("MoreTokenTest_01", {"\n", "msg", "zxc"},                   {"msg", "zxc"}              );
    FabricTestsParser("MoreTokenTest_02", {"\n", "msg", "zxc", "\'axaxaxaxax\'"}, {"msg", "zxc", "axaxaxaxax"});
}