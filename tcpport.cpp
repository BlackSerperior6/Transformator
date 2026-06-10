#include "tcpport.h"

TcpPort::TcpPort(int listenPortNum, int conId, PortType type, AbstractPort* target,
                 const std::set<std::string>& targetIPsList) :  AbstractPort(conId, type, target), targetNetworkPort(listenPortNum),
    listenSocket(INVALID_SOCKET)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    targetIPs = targetIPsList;
}

TcpPort::~TcpPort()
{
    Stop();
    WSACleanup();
}

void TcpPort::SetReceiveCallback(std::function<void(const std::vector<char>&)> callback)
{
    receiveCallback = callback;
}

void TcpPort::SetServerReceiveCallback(std::function<void (const std::string &, const std::vector<char> &)> callback)
{
    serverReceiveCallback = callback;
}

bool TcpPort::Start()
{
    if (isRunning)
        return true;

    isRunning = true;

    if (targetPort == nullptr)
        return StartClient();
    else
        return StartServer();
}

void TcpPort::Stop()
{
    isRunning = false;

    if (targetPort == nullptr)
        StopClient();
    else
        StopServer();
}

bool TcpPort::Accept(const std::vector<char> &data)
{
    if (!isRunning)
        return false;

    bool allSuccess = true;

    for (size_t i = 0; i < clientConnections.size(); i++)
    {
        if (!SendData(i, data))
            allSuccess = false;
    }

    return allSuccess;
}

bool TcpPort::SendData(size_t targetIndex, const std::vector<char> &data)
{
    if (targetIndex >= connectedClients.size())
    {
        if (errorCallback)
            errorCallback(connectionId, 400, "Invalid target index: " + std::to_string(targetIndex));

        return false;
    }

    TcpStatusCode responseCode;

    for (int attempt = 1; attempt <= maxRetryCount; attempt++)
    {
        if (clientConnections[targetIndex]->SendData(data, responseCode, 5000))
        {
            if (responseCode == TcpStatusCode::SUCCESS)
                return true;
            else
            {
                if (errorCallback)
                {
                    std::string errorMsg = "Failed to send to " + clientConnections[targetIndex]->ipAddress +
                                          " (attempt " + std::to_string(attempt) + "/" + std::to_string(maxRetryCount) +
                                          "): Status code " + std::to_string(static_cast<int>(responseCode));

                    errorCallback(connectionId, static_cast<int>(responseCode), errorMsg);
                }

                if (attempt < maxRetryCount)
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            }
        }
        else
        {
            if (errorCallback)
            {
                std::string errorMsg = "Send timeout/failed to " + clientConnections[targetIndex]->ipAddress +
                                      " (attempt " + std::to_string(attempt) + "/" + std::to_string(maxRetryCount) + ")";

                errorCallback(connectionId, static_cast<int>(TcpStatusCode::TIMEOUT), errorMsg);
            }

            if (attempt < maxRetryCount)
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        }
    }

    return false;
}

void TcpPort::CallErrorCallback(int errorCode, const std::string& errorMessage)
{
    if (errorCallback)
        errorCallback(connectionId, errorCode, errorMessage);
}

bool TcpPort::StartServer()
{
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (listenSocket == INVALID_SOCKET)
    {
        CallErrorCallback(WSAGetLastError(), "Failed to create server socket");
        return false;
    }

    int reuse = 1;

    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) == SOCKET_ERROR)
        CallErrorCallback(WSAGetLastError(), "Failed to set SO_REUSEADDR");

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(targetNetworkPort);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        CallErrorCallback(WSAGetLastError(), "Failed to bind to port " + std::to_string(targetNetworkPort));
        closesocket(listenSocket);
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        CallErrorCallback(WSAGetLastError(), "Failed to listen on socket");
        closesocket(listenSocket);
        return false;
    }

    acceptThread = std::thread(&TcpPort::ServerAcceptLoop, this);

    return true;
}

void TcpPort::StopServer()
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
            thread.join();
    }

    clientThreads.clear();
}

void TcpPort::ServerAcceptLoop()
{
    while (isRunning)
    {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET)
        {
            if (isRunning)
                CallErrorCallback(WSAGetLastError(), "Accept failed");

            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            connectedClients[clientSocket] = clientAddr;
        }

        clientThreads.emplace_back(&TcpPort::ServerHandleClient, this, clientSocket, std::string(clientIP));
    }
}

void TcpPort::ServerHandleClient(SOCKET clientSocket, std::string clientIP)
{
    const int BUFFER_SIZE = 65536;
    std::vector<char> buffer(BUFFER_SIZE);

    while (isRunning)
    {
        int bytesReceived = recv(clientSocket, buffer.data(), BUFFER_SIZE, 0);

        if (bytesReceived > 0)
        {
            TcpStatusCode statusCode;

            if (!targetIPs.empty() && targetIPs.find(clientIP) == targetIPs.end())
            {
                std::string errorMsg = "Rejected connection from unauthorized IP: " + std::string(clientIP);
                statusCode = TcpStatusCode::FORBIDDEN;
                CallErrorCallback(403, errorMsg);
            }

            std::vector<char> receivedData(buffer.begin(), buffer.begin() + bytesReceived);

            {
                std::lock_guard<std::mutex> lock(dataMutex);

                receiveQueue.push(receivedData);

                if (targetPort != nullptr)
                    targetPort->Accept(receivedData);

                dataCV.notify_one();

                statusCode = TcpStatusCode::SUCCESS;
            }

            std::string statusResponse = "STAT" + std::to_string(static_cast<int>(statusCode));
            send(clientSocket, statusResponse.c_str(), static_cast<int>(statusResponse.size()), 0);
        }
        else if (bytesReceived == 0)
            break;
        else
        {
            int error = WSAGetLastError();

            if (error != WSAEWOULDBLOCK && error != WSAETIMEDOUT)
            {
                CallErrorCallback(error, "Receive error from client " + clientIP);
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

bool TcpPort::StartClient()
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
                        receiveCallback(data);
                },
                [this](TcpStatusCode code)
            {
                    if (code != TcpStatusCode::SUCCESS)
                    {
                        std::string errorMsg = "Received error status code: " + std::to_string(static_cast<int>(code));
                        CallErrorCallback(static_cast<int>(code), errorMsg);
                    }
                }
            );

            clientConnections.push_back(std::move(connection));
        }
        else
        {
            std::string errorMsg = "Failed to connect to target: " + ip + ":" + std::to_string(targetNetworkPort);
            CallErrorCallback(1001, errorMsg);
        }
    }

    return !clientConnections.empty();
}

void TcpPort::StopClient()
{
    for (auto& connection : clientConnections)
        connection->Disconnect();

    clientConnections.clear();
}

int TcpPort::GetTargetNetworkPort()
{
    return targetNetworkPort;
}

std::set<std::string>& TcpPort::GetTargetIps()
{
    return targetIPs;
}
