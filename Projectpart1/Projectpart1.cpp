#include <iostream>
#include "ServerConfig.h"
#include "UserManager.h"
#include "Server.h"
#include "ClientUdpReceiver.h"

int main() {
    ServerConfig config;
    config.getServerInfo();
    config.printServerInfo();

    UserManager userManager;
    Server server(config, userManager);

    // Start the server in a separate thread
    std::thread server_thread([&server]() { server.start(); });

    // Start the UDP receiver for demonstration
    ClientUdpReceiver udpReceiver;
    udpReceiver.startReceiver();

    // Join the server thread
    server_thread.join();

    return 0;
}
