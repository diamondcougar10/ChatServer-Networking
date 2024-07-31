#ifndef SERVER_H
#define SERVER_H

#include "ServerConfig.h"
#include "UserManager.h"
#include <fstream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <WinSock2.h>

class Server {
public:
    Server(const ServerConfig& config, UserManager& userManager);
    void start();
private:
    SOCKET server_fd;
    SOCKET udp_socket;
    int capacity;
    char commandChar;
    UserManager& userManager;
    std::unordered_map<SOCKET, std::string> clientUsernames;
    std::vector<SOCKET> clients;

    std::ofstream commandLog;
    std::ofstream messageLog;

    std::thread udp_broadcast_thread;
    bool stop_udp_broadcast;

    void setupServer(int port);
    void setupUdpBroadcast();
    void udpBroadcastLoop();
    void serverLoop();
    void handleMessage(SOCKET sd, char* buffer);
    void sendMessage(SOCKET sd, const char* message);
    int receiveMessage(SOCKET sd, char* buffer);
    void logCommand(const std::string& command);
    void logMessage(const std::string& message);
    void listActiveClients(SOCKET sd);
    void logoutClient(SOCKET sd);
    void sendLog(SOCKET sd);
    void relayMessage(SOCKET sd, const std::string& message);
    void sendPrivateMessage(SOCKET sd, const std::string& username, const std::string& message);
    void handleNewConnection();
    void handleClientDisconnection(SOCKET sd);
};

#endif
