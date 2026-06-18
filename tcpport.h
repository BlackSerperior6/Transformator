#ifndef TCPPORT_H
#define TCPPORT_H

#include "abstractport.h"
#include "threadpool.h"
#include "tcpclientconnection.h"
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
#include <queue>
#include <condition_variable>

#pragma comment(lib, "ws2_32.lib")

class TcpPort : public AbstractPort
{
public:
    TcpPort(int targetPortNum, int conId, PortType type, AbstractPort* target = nullptr,
            void* parentConnection = nullptr, const std::set<std::string>& targetIPsList = {});

    ~TcpPort();

    void Accept(const std::vector<char>& data) override;

    bool Start() override;

    void Stop() override;

    int GetTargetNetworkPort();

    std::set<std::string>& GetTargetIps();

    void SetReceiveCallback(std::function<void(const std::vector<char>&)> callback);

    void SetServerReceiveCallback(std::function<void(const std::string&, const std::vector<char>&)> callback);

private:
    std::atomic<bool> isRunning;

    // For client mode - repesents IPs to forward data to, for server mode - repsenets IPs from which we accept data
    std::set<std::string> targetIPs;

    int targetNetworkPort;
    int maxRetryCount;
    int retryDelayMs;

    // Server side (we listen, have target port)
    SOCKET listenSocket;
    std::thread listenThread;
    ThreadPool* clientThreads;
    std::map<SOCKET, sockaddr_in> clientSockets;
    std::mutex clientsSocketMutex;

    //Client side (we send, no target port)
    std::vector<std::unique_ptr<TCPClientConnection>> connectionsToServers;
    ThreadPool* sharedClientsThreadPool;

    std::function<void(const std::vector<char>&)> receiveCallback;
    std::function<void(const std::string&, const std::vector<char>&)> serverReceiveCallback;

    void CallErrorCallback(int errorCode, const std::string& errorMessage);

    void SendData(size_t targetIndex, const std::vector<char>& data);

    bool StartServer();

    void StopServer();

    void ServerAcceptLoop();

    void ServerHandleClient(SOCKET clientSocket, std::string clientIP);

    bool StartClient();

    void StopClient();
};

#endif // TCPPORT_H
