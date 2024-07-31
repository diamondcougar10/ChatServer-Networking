#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Server.h"
#include <thread>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")

Server::Server(const ServerConfig& config, UserManager& userManager)
    : capacity(config.capacity), commandChar(config.commandChar), userManager(userManager), stop_udp_broadcast(false) 
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "WSAStartup failed.\n";
        exit(EXIT_FAILURE);
    }
    setupServer(config.port);
    setupUdpBroadcast();
    commandLog.open("commandLog.txt", std::ios::app);
    messageLog.open("messageLog.txt", std::ios::app);
}

void Server::setupServer(int port) 
{
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
    {
        int err = WSAGetLastError();
        std::cerr << "socket failed with error: " << err << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        std::cerr << "setsockopt failed with error: " << err << std::endl;
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        std::cerr << "bind failed with error: " << err << std::endl;
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, capacity) == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        std::cerr << "listen failed with error: " << err << std::endl;
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is listening on port " << port << "...\n";
}

void Server::setupUdpBroadcast() {
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) 
    {
        int err = WSAGetLastError();
        std::cerr << "UDP socket failed with error: " << err << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    int broadcast = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        std::cerr << "setsockopt for UDP broadcast failed with error: " << err << std::endl;
        closesocket(udp_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    udp_broadcast_thread = std::thread(&Server::udpBroadcastLoop, this);
}

void Server::udpBroadcastLoop() 
{
    struct sockaddr_in broadcast_addr;
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(31337);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    char broadcast_message[128];
    while (!stop_udp_broadcast) {
        snprintf(broadcast_message, sizeof(broadcast_message), "Server IP: %s, Port: %d", "127.0.0.1", 31337);

        if (sendto(udp_socket, broadcast_message, strlen(broadcast_message), 0,
            (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) == SOCKET_ERROR) 
        {
            int err = WSAGetLastError();
            std::cerr << "sendto failed with error: " << err << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    closesocket(udp_socket);
    WSACleanup();
}

void Server::start() 
{
    serverLoop();
    stop_udp_broadcast = true;
    if (udp_broadcast_thread.joinable()) {
        udp_broadcast_thread.join();
    }
}

void Server::serverLoop() {
    fd_set masterSet, readySet;
    FD_ZERO(&masterSet);
    FD_ZERO(&readySet);
    FD_SET(server_fd, &masterSet);
    SOCKET max_sd = server_fd;
    int client_count = 0;

    while (true) {
        readySet = masterSet;
        if (select(static_cast<int>(max_sd + 1), &readySet, NULL, NULL, NULL) == SOCKET_ERROR) {
            perror("select");
            closesocket(server_fd);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        for (SOCKET sd = 0; sd <= max_sd; ++sd) {
            if (FD_ISSET(sd, &readySet)) {
                if (sd == server_fd) {
                    if (client_count < capacity) {
                        struct sockaddr_in client_addr;
                        int addrlen = sizeof(client_addr);
                        SOCKET new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
                        if (new_socket == INVALID_SOCKET) {
                            perror("accept");
                            closesocket(server_fd);
                            WSACleanup();
                            exit(EXIT_FAILURE);
                        }
                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(client_addr.sin_addr), ipStr, INET_ADDRSTRLEN);
                        std::cout << "New connection, socket fd is " << new_socket
                            << ", ip is : " << ipStr
                            << ", port : " << ntohs(client_addr.sin_port) << std::endl;
                        FD_SET(new_socket, &masterSet);
                        if (new_socket > max_sd) {
                            max_sd = new_socket;
                        }
                        client_count++;
                        char welcomeMessage[256];
                        sprintf_s(welcomeMessage, sizeof(welcomeMessage), "Welcome to the chat server! Command character is %c\n", commandChar);
                        sendMessage(new_socket, welcomeMessage);
                    }
                    else {
                        std::cout << "Chat capacity reached. Rejecting new connection." << std::endl;
                    }
                }
                else {
                    char buffer[1024] = { 0 };
                    int valread = receiveMessage(sd, buffer);
                    if (valread <= 0) {
                        struct sockaddr_in client_addr;
                        int addrlen = sizeof(client_addr);
                        getpeername(sd, (struct sockaddr*)&client_addr, &addrlen);
                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(client_addr.sin_addr), ipStr, INET_ADDRSTRLEN);
                        std::cout << "Host disconnected, ip " << ipStr
                            << ", port " << ntohs(client_addr.sin_port) << std::endl;
                        closesocket(sd);
                        FD_CLR(sd, &masterSet);
                        client_count--;
                        if (clientUsernames.find(sd) != clientUsernames.end()) {
                            userManager.logoutUser(clientUsernames[sd]);
                            clientUsernames.erase(sd);
                        }
                    }
                    else {
                        handleMessage(sd, buffer);
                    }
                }
            }
        }
    }
}

void Server::handleMessage(SOCKET sd, char* buffer) {
    std::string command(buffer);
    logCommand(command);

    if (command.compare(0, 5, "~help") == 0) {
        const char* helpMessage = "Available commands: ~help, ~register <username> <password>, ~login <username> <password>, ~logout, ~getlist, ~getlog\n";
        sendMessage(sd, helpMessage);
    }
    else if (command.compare(0, 9, "~register") == 0) {
        char username[50], password[50];
        sscanf_s(buffer + 10, "%49s %49s", username, (unsigned)_countof(username), password, (unsigned)_countof(password));
        if (userManager.userExists(username)) {
            const char* existsMessage = "Username already exists. Please choose another one.\n";
            sendMessage(sd, existsMessage);
        }
        else {
            userManager.addUser(username, password);
            const char* successMessage = "Registration successful. You can now log in.\n";
            sendMessage(sd, successMessage);
        }
    }
    else if (command.compare(0, 6, "~login") == 0) {
        char username[50], password[50];
        sscanf_s(buffer + 7, "%49s %49s", username, (unsigned)_countof(username), password, (unsigned)_countof(password));
        if (clientUsernames.find(sd) != clientUsernames.end()) {
            const char* alreadyLoggedInMessage = "You are already logged in.\n";
            sendMessage(sd, alreadyLoggedInMessage);
        }
        else if (!userManager.userExists(username)) {
            const char* notFoundMessage = "User not found. Please register first.\n";
            sendMessage(sd, notFoundMessage);
        }
        else if (!userManager.authenticateUser(username, password)) {
            const char* incorrectPasswordMessage = "Incorrect password. Please try again.\n";
            sendMessage(sd, incorrectPasswordMessage);
        }
        else {
            userManager.loginUser(username);
            clientUsernames[sd] = username;
            const char* loginSuccessMessage = "Login successful.\n";
            sendMessage(sd, loginSuccessMessage);
        }
    }
    else if (command.compare(0, 7, "~logout") == 0) {
        logoutClient(sd);
    }
    else if (command.compare(0, 8, "~getlist") == 0) {
        listActiveClients(sd);
    }
    else if (command.compare(0, 7, "~getlog") == 0) {
        sendLog(sd);
    }
    else if (command.compare(0, 5, "~send") == 0) {
        char username[50], message[1024];
        sscanf_s(buffer + 6, "%49s %1023[^\n]", username, (unsigned)_countof(username), message, (unsigned)_countof(message));
        sendPrivateMessage(sd, username, message);
    }
    else {
        relayMessage(sd, buffer);
    }
}

void Server::sendMessage(SOCKET sd, const char* message) {
    uint8_t messageLength = static_cast<uint8_t>(strlen(message));
    send(sd, reinterpret_cast<const char*>(&messageLength), 1, 0);
    send(sd, message, messageLength, 0);
}

int Server::receiveMessage(SOCKET sd, char* buffer) {
    uint8_t messageLength;
    if (recv(sd, reinterpret_cast<char*>(&messageLength), 1, 0) <= 0) {
        return -1;
    }
    int totalReceived = 0, bytesReceived;
    while (totalReceived < messageLength) {
        bytesReceived = recv(sd, buffer + totalReceived, messageLength - totalReceived, 0);
        if (bytesReceived <= 0) {
            return -1;
        }
        totalReceived += bytesReceived;
    }
    buffer[totalReceived] = '\0';
    return totalReceived;
}

void Server::logCommand(const std::string& command) {
    commandLog << command << std::endl;
}

void Server::logMessage(const std::string& message) {
    messageLog << message << std::endl;
}

void Server::listActiveClients(SOCKET sd) {
    std::string activeClients = "Active clients:\n";
    for (const auto& user : userManager.getLoggedInUsers()) {
        activeClients += user + "\n";
    }
    sendMessage(sd, activeClients.c_str());
}

void Server::logoutClient(SOCKET sd) {
    if (clientUsernames.find(sd) != clientUsernames.end()) {
        userManager.logoutUser(clientUsernames[sd]);
        clientUsernames.erase(sd);
        const char* logoutSuccessMessage = "Logout successful.\n";
        sendMessage(sd, logoutSuccessMessage);
    }
    else {
        const char* notLoggedInMessage = "You are not logged in.\n";
        sendMessage(sd, notLoggedInMessage);
    }
    closesocket(sd);
}

void Server::sendLog(SOCKET sd) {
    std::ifstream messageLog("messageLog.txt");
    std::string line;
    while (std::getline(messageLog, line)) {
        sendMessage(sd, line.c_str());
    }
}

void Server::relayMessage(SOCKET sd, const std::string& message) {
    logMessage(message);
    for (const auto& client : clientUsernames) {
        if (client.first != sd) {
            sendMessage(client.first, message.c_str());
        }
    }
}

void Server::sendPrivateMessage(SOCKET sd, const std::string& username, const std::string& message) {
    for (const auto& client : clientUsernames) {
        if (client.second == username) {
            sendMessage(client.first, message.c_str());
            return;
        }
    }
    const char* userNotFoundMessage = "User not found.\n";
    sendMessage(sd, userNotFoundMessage);
}
