#include "tcpport.h"

TcpPort::TcpPort(int listenPortNum, int conId, PortType type, AbstractPort* target,
                 void* parentConnection, const std::set<std::string>& targetIPsList) :  AbstractPort(conId, type, parentConnection, target),
                 targetNetworkPort(listenPortNum),
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

void TcpPort::Accept(const std::vector<char> &data)
{
    if (!isRunning || targetPort != nullptr)
        return;

    AbstractPort::Accept(data);

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
    clientThreads = new ThreadPool(4);

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
    for (auto i : clientSockets)
        closesocket(i.first);

    clientSockets.clear();

    delete clientThreads;

    if (listenSocket != INVALID_SOCKET)
    {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
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

        {
            std::lock_guard<std::mutex> lock(clientsSocketMutex);
            clientSockets[clientSocket] = clientAddr;
        }


        if (clientSocket == INVALID_SOCKET)
        {
            if (isRunning)
                CallErrorCallback(WSAGetLastError(), "Accept failed");

            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

        clientThreads->AddTask([this, clientSocket, clientIP]{this->ServerHandleClient(clientSocket, std::string(clientIP));});
    }
}

void TcpPort::ServerHandleClient(SOCKET clientSocket, std::string clientIP)
{
    if (!isRunning)
        return;

    const int BUFFER_SIZE = 65536;
    std::vector<char> buffer(BUFFER_SIZE);

    int bytesReceived = recv(clientSocket, buffer.data(), BUFFER_SIZE, 0);

    if (bytesReceived > 0)
    {
        TcpStatusCode statusCode;

        if (!targetIPs.empty() && targetIPs.find(clientIP) == targetIPs.end())
            statusCode = TcpStatusCode::FORBIDDEN;
        else
        {
            std::vector<char> receivedData(buffer.begin(), buffer.begin() + bytesReceived);

            statusCode = TcpStatusCode::SUCCESS;

            if (targetPort != nullptr)
                targetPort->Accept(receivedData);
        }

        std::string statusResponse = "STAT" + std::to_string(static_cast<int>(statusCode));
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
        std::lock_guard<std::mutex> lock(clientsSocketMutex);
        clientSockets.erase(clientSocket);
    }

    closesocket(clientSocket);
}

bool TcpPort::StartClient()
{
    sharedClientsThreadPool = new ThreadPool(4);

    for (const auto& ip : targetIPs)
    {
        auto connection = std::make_unique<TCPClientConnection>(errorCallback, connectionId, sharedClientsThreadPool);

        connection->Connect(ip, targetNetworkPort, 5000);

        connectionsToServers.push_back(std::move(connection));
    }

    return true;
}

void TcpPort::StopClient()
{
    for (auto& connection : connectionsToServers)
        connection->Disconnect();

    delete sharedClientsThreadPool;

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
