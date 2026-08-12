#include <iostream>
#include <cassert>
#include <fstream>


#include "testSources.h"
#include "application.h"


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
    try {
        ParserCommandTest test(arrInput, arrAnswer);

        bool result = test.successfull();
        printStatus(nameTest, result); 

        if (!result) {
            std::ofstream fileError(fileErrorPath, std::ios::app);
            if (fileError.is_open()) { fileError << "[ " << nameTest << " ]: result != answer\n"; }
        }

    } catch (const std::exception& e) {
        printStatus(nameTest, false);

        std::ofstream fileError(fileErrorPath, std::ios::app);
        if (fileError.is_open()) { fileError << "[ " << nameTest << " ]: " << e.what() << "\n"; }
    }
}


void parserTests() {
    std::cout << YELLOW << "--- Running ParserCommand Tests ---" << RESET << "\n";

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

    std::cout << YELLOW << "--- ParserCommand tests passed successfully! ---" << RESET << "\n\n";
}