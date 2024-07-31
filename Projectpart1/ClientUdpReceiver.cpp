#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ClientUdpReceiver.h"

#pragma comment(lib, "Ws2_32.lib")

void ClientUdpReceiver::startReceiver() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return;
    }

    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == INVALID_SOCKET) {
        int err = WSAGetLastError();
        std::cerr << "UDP socket failed with error: " << err << std::endl;
        WSACleanup();
        return;
    }

    int reuse = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "setsockopt for UDP reuse failed with error: " << err << std::endl;
        closesocket(udp_socket);
        WSACleanup();
        return;
    }

    struct sockaddr_in recv_addr;
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(31337);
    recv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "bind failed with error: " << err << std::endl;
        closesocket(udp_socket);
        WSACleanup();
        return;
    }

    char buffer[128];
    struct sockaddr_in sender_addr;
    int sender_addr_size = sizeof(sender_addr);

    if (recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
        (struct sockaddr*)&sender_addr, &sender_addr_size) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "recvfrom failed with error: " << err << std::endl;
    }
    else {
        buffer[127] = '\0'; 
        std::cout << "Received broadcast message: " << buffer << std::endl;
    }

    closesocket(udp_socket);
    WSACleanup();
}
