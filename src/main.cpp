#include <iostream>

#include "headers/application.h"


int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "error! proper launch: " << argv[0] << " <file path> <level importance>\n";
        return 1; 
    }

    std::string path = argv[1];
    std::string level = argv[2];
    
    if (!(std::filesystem::exists(path) && std::filesystem::is_regular_file(path))) {
        std::cerr << "file not found!\n";
        return 1;
    }

    auto optLevel = VirtualLogger::str2lvl(level);
    
    if (!optLevel) {
        std::cerr << "level " << argv[2] << " not found!\n";
        return 1;
    }

    Application app(path, optLevel.value());
    app.run();
    return 0;
}

