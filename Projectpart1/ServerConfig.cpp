#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ServerConfig.h"
#include <string>

#pragma comment(lib, "Ws2_32.lib")

void ServerConfig::getServerInfo() {
    std::cout << "Enter server port: ";
    std::cin >> port;
    std::cin.ignore(10000, '\n'); 

    std::cout << "Enter chat capacity: ";
    std::cin >> capacity;
    std::cin.ignore(10000, '\n');

    std::cout << "Enter command character (default is ~): ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        commandChar = '~';
    }
    else {
        commandChar = input[0];
    }
}

void ServerConfig::printServerInfo() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        exit(EXIT_FAILURE);
    }

    char hostname[1024];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "gethostname failed with error: " << err << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    hostname[sizeof(hostname) - 1] = '\0'; // Ensure null-termination
    std::cout << "Hostname: " << hostname << std::endl;

    struct addrinfo hints, * info, * p;
    char host[256], service[256];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int addrinfo_result = getaddrinfo(hostname, NULL, &hints, &info);
    if (addrinfo_result != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(addrinfo_result) << std::endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    for (p = info; p != NULL; p = p->ai_next) {
        getnameinfo(p->ai_addr, (socklen_t)p->ai_addrlen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST);
        std::cout << "IP Address: " << host << std::endl;
    }
    freeaddrinfo(info);
    std::cout << "Port: " << port << std::endl;

    WSACleanup();
}
