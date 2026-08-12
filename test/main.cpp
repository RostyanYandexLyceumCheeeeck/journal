#include "testSources.h"
#include "application.h"
#include "virtualLogger.h"


extern void parserTests();
extern void runAsyncLoggerTests();


int main() {
    parserTests();
    runAsyncLoggerTests();

    std::filesystem::remove(fileTemp);
 
    return 0;
}