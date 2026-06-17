#include "tcpclientconnection.h"

TCPClientConnection::TCPClientConnection(std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> callback,
                                         int conId)
    : isRunning(false), errorCallback(callback), connectionId(conId), pool(new ThreadPool(4))
{}

TCPClientConnection::~TCPClientConnection()
{
    Disconnect();
}

void TCPClientConnection::Connect(const std::string &ip, int portNum, int timeout)
{
    ipAddress = ip;
    port = portNum;
    timeoutMs = timeout;

    isRunning = true;
}

void TCPClientConnection::Disconnect()
{
    isRunning = false;

    delete pool;
}

void TCPClientConnection::SendData(const std::vector<char> &data)
{
    pool->AddTask(&TCPClientConnection::SendDataWithRetries, data, timeoutMs);
}

void TCPClientConnection::SendDataWithRetries(const std::vector<char>& data, int timeoutMs)
{
    if (!isRunning)
        return;

    TcpStatusCode finalCode = TcpStatusCode::TIMEOUT;

    for (int i = 0; i <= comAttempts; i++)
    {
        SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) <= 0)
        {
            finalCode = TcpStatusCode::INVALID_IP_ERROR;
            closesocket(socket);
            break;
        }

        if (::connect(socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            finalCode = TcpStatusCode::CONNECTION_ERROR;
            closesocket(socket);
            break;
        }

        int byteSend = send(socket, data.data(), static_cast<int>(data.size()), 0);

        if (byteSend == SOCKET_ERROR)
        {
            finalCode = TcpStatusCode::SEND_ERROR;
            closesocket(socket);
            continue;
        }

        char buffer[4096] = {0};
        int bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0)
        {
            finalCode = TcpStatusCode::RECEIVE_ERROR;
            closesocket(socket);
            continue;
        }

        buffer[bytesReceived] = '\0';

        finalCode = Utils::ParseStatusCode(std::string(buffer));

        closesocket(socket);

        if (finalCode != TcpStatusCode::SUCCESS)
        {
            timeoutMs *= 2;

            std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
        }
        else
            break;
    }

    if (finalCode != TcpStatusCode::SUCCESS)
    {
        if (errorCallback)
            errorCallback(connectionId, (int) finalCode, "An error has occured during TCP data transfer. Check the error code.");
    }
}
