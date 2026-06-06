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

    bool Accept(const std::vector<char>& data);

    bool Start();

    void Stop();

    bool SendData(size_t targetIndex, const std::vector<char>& data);

    bool SendDataToSpecificIp(const std::string& ip, const std::vector<char>& data);

    bool SendDataToTarget(size_t targetIndex, const std::vector<char>& data);

    void SetReceiveCallback(std::function<void(const std::vector<char>&)> callback);

    void SetServerReceiveCallback(std::function<void(const std::string&, const std::vector<char>&)> callback);

    void SetErrorCallback(std::function<void(int, int, const std::string&)> callback);

    void SetRetryConfig(int maxRetries, int delayMs);

private:
    std::atomic<bool> isRunning;
    std::mutex dataMutex;
    std::condition_variable dataCV;
    std::queue<std::vector<char>> receiveQueue;

    // For client mode - repesents IPs to forward data to, for server mode - repsenets IPs from which we accept data
    std::set<std::string> targetIPs;

    int targetNetworkPort;
    int maxRetryCount;
    int retryDelayMs;

    // Server side (we listen, have target port)
    SOCKET listenSocket;
    std::thread acceptThread;
    std::vector<std::thread> clientThreads;
    std::mutex clientsMutex;
    std::map<SOCKET, sockaddr_in> connectedClients;

    //Client side (we send, no target port)
    std::vector<std::unique_ptr<TCPClientConnection>> clientConnections;

    std::function<void(const std::vector<char>&)> receiveCallback;
    std::function<void(const std::string&, const std::vector<char>&)> serverreceiveCallback;
    std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> errorCallback;

    void callErrorCallback(int errorCode, const std::string& errorMessage)
    {
        if (errorCallback)
            errorCallback(connectionId, errorCode, errorMessage);
    }

    bool StartServer()
    {
        listenSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (listenSocket == INVALID_SOCKET)
        {
            callErrorCallback(WSAGetLastError(), "Failed to create server socket");
            return false;
        }

        int reuse = 1;
        if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) == SOCKET_ERROR)
        {
            callErrorCallback(WSAGetLastError(), "Failed to set SO_REUSEADDR");
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(targetNetworkPort);

        if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            callErrorCallback(WSAGetLastError(), "Failed to bind to port " + std::to_string(targetNetworkPort));
            closesocket(listenSocket);
            return false;
        }

        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            callErrorCallback(WSAGetLastError(), "Failed to listen on socket");
            closesocket(listenSocket);
            return false;
        }

        acceptThread = std::thread(&TcpPort::ServerAcceptLoop, this);

        return true;
    }

    void StopServer()
    {
        if (listenSocket != INVALID_SOCKET)
        {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);

            for (auto& pair : connectedClients)
            {
                closesocket(pair.first);
            }

            connectedClients.clear();
        }

        if (acceptThread.joinable())
        {
            acceptThread.join();
        }

        for (auto& thread : clientThreads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        clientThreads.clear();
    }

    void ServerAcceptLoop()
    {
        while (isRunning)
        {
            sockaddr_in clientAddr;
            int clientAddrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket == INVALID_SOCKET)
            {
                if (isRunning)
                {
                    callErrorCallback(WSAGetLastError(), "Accept failed");
                }
                continue;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

            if (!targetIPs.empty() && targetIPs.find(clientIP) == targetIPs.end())
            {
                std::string errorMsg = "Rejected connection from unauthorized IP: " + std::string(clientIP);
                callErrorCallback(403, errorMsg);
                closesocket(clientSocket);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                connectedClients[clientSocket] = clientAddr;
            }

            clientThreads.emplace_back(&TcpPort::ServerHandleClient, this, clientSocket, std::string(clientIP));
        }
    }

    void ServerHandleClient(SOCKET clientSocket, std::string clientIP)
    {
        const int BUFFER_SIZE = 65536;
        std::vector<char> buffer(BUFFER_SIZE);

        while (isRunning)
        {
            int bytesReceived = recv(clientSocket, buffer.data(), BUFFER_SIZE, 0);

            if (bytesReceived > 0)
            {
                std::vector<char> receivedData(buffer.begin(), buffer.begin() + bytesReceived);

                {
                    std::lock_guard<std::mutex> lock(dataMutex);
                    receiveQueue.push(receivedData);
                    dataCV.notify_one();
                }

                TcpStatusCode statusCode = TcpStatusCode::SUCCESS;
                std::string statusResponse = "STAT" + std::to_string(static_cast<int>(statusCode));
                send(clientSocket, statusResponse.c_str(), static_cast<int>(statusResponse.size()), 0);
            }
            else if (bytesReceived == 0)
            {
                break;
            }
            else
            {
                int error = WSAGetLastError();

                if (error != WSAEWOULDBLOCK && error != WSAETIMEDOUT)
                {
                    callErrorCallback(error, "Receive error from client " + clientIP);
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

    bool StartClient()
    {
        for (const auto& ip : targetIPs)
        {
            auto connection = std::make_unique<TCPClientConnection>();

            if (connection->Connect(ip, targetNetworkPort, 5000))
            {
                connection->StartReceive(
                    [this](const std::vector<char>& data)
                {
                        {
                            std::lock_guard<std::mutex> lock(dataMutex);
                            receiveQueue.push(data);
                            dataCV.notify_one();
                        }

                        if (receiveCallback)
                        {
                            receiveCallback(data);
                        }
                    },
                    [this](TcpStatusCode code)
                {
                        if (code != TcpStatusCode::SUCCESS)
                        {
                            std::string errorMsg = "Received error status code: " + std::to_string(static_cast<int>(code));
                            callErrorCallback(static_cast<int>(code), errorMsg);
                        }
                    }
                );

                clientConnections.push_back(std::move(connection));
            }
            else
            {
                std::string errorMsg = "Failed to connect to target: " + ip + ":" + std::to_string(targetNetworkPort);
                callErrorCallback(1001, errorMsg);
            }
        }

        return !clientConnections.empty();
    }

    void StopClient()
    {
        for (auto& connection : clientConnections)
        {
            connection->Disconnect();
        }

        clientConnections.clear();
    }

    void reconnectClients()
    {
        StopClient();
        StartClient();
    }
};

#endif // TCPPORT_H
