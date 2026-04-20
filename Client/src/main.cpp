#include "Client.h"
#include <iostream>

int main() {
    Client client("127.0.0.1", 8080);

    if (!client.connectToServer()) {
        std::cerr << "Could not connect to server" << std::endl;
        return 1;
    }

    client.run();
    return 0;
}
