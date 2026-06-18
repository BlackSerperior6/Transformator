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
#include "threadpool.h"

class TCPClientConnection
{
public:
    TCPClientConnection(std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> errorCallback,
                        int conId, ThreadPool* providedThreadPool);

    ~TCPClientConnection();

    ThreadPool* pool;

    int connectionId;

    std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> errorCallback;

    void Connect(const std::string& ip, int portNum, int timeout);

    void Disconnect();

    void SendData(const std::vector<char>& data);

private:
    std::string ipAddress;
    int port;
    int timeoutMs;
    int comAttempts;
    std::atomic<bool> isRunning;

    void SendDataWithRetries(const std::vector<char>& data, int timeoutMs);
};

#endif // TCPCLIENTCONNECTION_H
