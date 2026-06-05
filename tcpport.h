#ifndef TCPPORT_H
#define TCPPORT_H

#include "abstractport.h"
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
    TcpPort(int targetPortNum, const std::set<std::string>& targetIPsList = {});

    ~TcpPort();

    bool Accept(std::string errorMessage, const std::vector<char>& data);

    bool Start();

    void Stop();

    bool SendData(const std::vector<char>& data);

    bool SendDataToSpecificIp(const std::string& ip, const std::vector<char>& data);

    bool SendDataToTarget(size_t targetIndex, const std::vector<char>& data);

    bool waitForData(std::vector<char>& outData, int timeoutMs);

    bool HasData();

    void AddTargetIP(const std::string& ip);

    void RemoveTargetIp(const std::string& ip);

    void SetTargetedIps(); // TODO Implement

    void IsIpTargeted();

private:
    std::atomic<bool> isRunning;
    std::mutex dataMutex;
    std::condition_variable dataCV;
    std::queue<std::vector<char>> receiveQueue;

    // For client mode - repesents IPs to forward data to, for server mode - repsenets IPs from which we accept data
    std::set<std::string> targetIPs;

    // Server side (we listen)
    SOCKET listenSocket;
    std::thread acceptThread;
    std::vector<std::thread> clientThreads;
    std::mutex clientsMutex;
    std::map<SOCKET, sockaddr_in> connectedClients;
    int listenPort;

    //Client side (we send)
    std::vector<std::unique_ptr<TCPClientConnection>> clientConnections;
    int targetPort;

    bool StartServer(std::string errorMessage)
    {
        listenSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (listenSocket == INVALID_SOCKET)
        {
            errorMessage = "Ivalid Socket";
            return false;
        }

        int reuse = 1;

        setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(listenPort);

        if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            errorMessage = "Socket error";
            closesocket(listenSocket);
            return false;
        }

        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            errorMessage = "Socket error";
            closesocket(listenSocket);
            return false;
        }

        acceptThread = std::thread(&TcpPort::serverAcceptLoop, this);

        return true;
    }

    void stopServer()
    {
        if (listenSocket != INVALID_SOCKET)
        {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);

            for (auto& pair : connectedClients)
                closesocket(pair.first);

            connectedClients.clear();
        }

        if (acceptThread.joinable())
            acceptThread.join();

        for (auto& thread : clientThreads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        clientThreads.clear();
    }

    void serverAcceptLoop()
    {
        while (isRunning)
        {
            sockaddr_in clientAddr;
            int clientAddrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket == INVALID_SOCKET) {
                continue;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

            if (!targetIPs.empty() && targetIPs.find(clientIP) == targetIPs.end())
            {
                closesocket(clientSocket);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                connectedClients[clientSocket] = clientAddr;
            }

            clientThreads.emplace_back(&TcpPort::serverHandleClient, this, clientSocket, std::string(clientIP));
        }
    }

    void serverHandleClient(SOCKET clientSocket)
    {
        const int BUFFER_SIZE = 65536;
        std::vector<char> buffer(BUFFER_SIZE);

        while (isRunning)
        {
            int bytesReceived = recv(clientSocket, buffer.data(), BUFFER_SIZE, 0);

            if (bytesReceived > 0) {
                std::vector<char> receivedData(buffer.begin(), buffer.begin() + bytesReceived);

                {
                    std::lock_guard<std::mutex> lock(dataMutex);
                    receiveQueue.push(receivedData);
                    dataCV.notify_one();
                }
            }
            else if (bytesReceived == 0) {
                break;  // Client disconnected
            }
            else {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    break;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            connectedClients.erase(clientSocket);
        }

        closesocket(clientSocket);
    }

    bool startClient(std::string errorMessage)
    {
        for (const auto& ip : targetIPs)
        {
            auto connection = std::make_unique<TCPClientConnection>();

            if (connection->connect(ip, targetPort))
            {
                connection->startReceive([this](const std::vector<char>& data)
                {
                    {
                        std::lock_guard<std::mutex> lock(dataMutex);
                        receiveQueue.push(data);
                        dataCV.notify_one();
                    }

                });

                clientConnections.push_back(std::move(connection));
            }
        }

        return !clientConnections.empty();
    }
};

#endif // TCPPORT_H
