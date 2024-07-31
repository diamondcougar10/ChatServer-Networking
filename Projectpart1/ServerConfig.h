#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

class ServerConfig 
{
public:
    int port;
    int capacity;
    char commandChar;

    void getServerInfo();
    void printServerInfo();
};

#endif
