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
#include "tcpstatuscode.h"
#include "utils.cpp"

class TCPClientConnection
{
public:
    TCPClientConnection();

    ~TCPClientConnection();

    SOCKET socket;
    std::string ipAddress;
    int port;
    int comAttempts;
    std::atomic<bool> connected;
    std::mutex socketMutex;

    bool Connect(const std::string& ip, int portNum, int timeout);

    void Disconnect();

    void SendData(const std::vector<char>& data, int timeoutMs);

    void StartReceive(std::function<void(const std::vector<char>&)> callback,
                      std::function<void(TcpStatusCode)> responseCallback);

    void SetErrorCallback(std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)>);
};

#endif // TCPCLIENTCONNECTION_H
