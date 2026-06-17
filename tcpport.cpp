#include "tcpport.h"

TcpPort::TcpPort(int listenPortNum, int conId, PortType type, AbstractPort* target,
                 const std::set<std::string>& targetIPsList) :  AbstractPort(conId, type, target), targetNetworkPort(listenPortNum),
                 listenSocket(INVALID_SOCKET), clientThreads(new ThreadPool(4))
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

void TcpPort::Accept(const std::vector<char> &data)
{
    if (!isRunning || targetPort != nullptr)
        return;

    for (size_t i = 0; i < connectionsToServers.size(); i++)
        SendData(i, data);
}

void TcpPort::SendData(size_t targetIndex, const std::vector<char> &data)
{
    if (targetIndex >= connectionsToServers.size())
    {
        if (errorCallback)
            errorCallback(connectionId, 400, "Invalid target index: " + std::to_string(targetIndex));

        return;
    }

    connectionsToServers[targetIndex]->SendData(data);
}

void TcpPort::CallErrorCallback(int errorCode, const std::string& errorMessage)
{
    if (errorCallback)
        errorCallback(connectionId, errorCode, errorMessage);
}

bool TcpPort::StartServer()
{
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

    listenThread = std::thread(&TcpPort::ServerAcceptLoop, this);

    return true;
}

void TcpPort::StopServer()
{
    if (listenSocket != INVALID_SOCKET)
    {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }

    delete  clientThreads;

    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        for (auto& pair : connectionsToClients)
            closesocket(pair.first);

        connectionsToClients.clear();
    }

    if (listenThread.joinable())
        listenThread.join();
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
            connectionsToClients[clientSocket] = clientAddr;
        }

        clientThreads->AddTask(&TcpPort::ServerHandleClient, clientSocket, std::string(clientIP));
    }
}

void TcpPort::ServerHandleClient(SOCKET clientSocket, std::string clientIP)
{
    const int BUFFER_SIZE = 65536;
    std::vector<char> buffer(BUFFER_SIZE);

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
        else
        {
            std::vector<char> receivedData(buffer.begin(), buffer.begin() + bytesReceived);

            statusCode = TcpStatusCode::SUCCESS;

            if (targetPort != nullptr)
                targetPort->Accept(receivedData);
        }

        std::string statusResponse = "Status:" + std::to_string(static_cast<int>(statusCode));
        int bytesSend = send(clientSocket, statusResponse.c_str(), static_cast<int>(statusResponse.size()), 0);

        if (bytesSend <= 0)
        {
            CallErrorCallback(400, "Failed to send a response to client with IP: " + clientIP);
            return;
        }
    }
    else if (bytesReceived == 0)
        return;
    else
    {
        int error = WSAGetLastError();

        if (error != WSAEWOULDBLOCK && error != WSAETIMEDOUT && error != WSAECONNABORTED)
            CallErrorCallback(error, "Receive error from client " + clientIP);
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        connectionsToClients.erase(clientSocket);
    }

    closesocket(clientSocket);
}

bool TcpPort::StartClient()
{
    for (const auto& ip : targetIPs)
    {
        auto connection = std::make_unique<TCPClientConnection>();

        connection->Connect(ip, targetNetworkPort, 5000);

        connectionsToServers.push_back(std::move(connection));
    }

    return true;
}

void TcpPort::StopClient()
{
    for (auto& connection : connectionsToServers)
        connection->Disconnect();

    connectionsToServers.clear();
}

int TcpPort::GetTargetNetworkPort()
{
    return targetNetworkPort;
}

std::set<std::string>& TcpPort::GetTargetIps()
{
    return targetIPs;
}
