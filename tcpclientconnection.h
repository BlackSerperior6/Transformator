#ifndef TCPCLIENTCONNECTION_H
#define TCPCLIENTCONNECTION_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <set>
#include <functional>
#include <algorithm>
#include <queue>
#include <condition_variable>

class TCPClientConnection
{
public:
    TCPClientConnection();

    ~TCPClientConnection();

    SOCKET socket;
    std::string ipAddress;
    int port;
    std::atomic<bool> connected;
    std::thread receiveThread;
    std::mutex sendMutex;

    bool Connect(const std::string& ip, int portNum);

    void Disconnect();

    bool SendData(const std::vector<char>& data);
};

#endif // TCPCLIENTCONNECTION_H
