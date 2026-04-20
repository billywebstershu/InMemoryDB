#include "Server.h"
#include <iostream>

int main() {
    Server server(8080);

    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    // keep main thread alive while server runs
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit" || input == "exit") break;
    }

    server.stop();
    return 0;
}