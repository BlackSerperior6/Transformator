#include "tcpclientconnection.h"

TCPClientConnection::TCPClientConnection() : socket(INVALID_SOCKET), connected(false) {}

TCPClientConnection::~TCPClientConnection()
{
    Disconnect();
}

bool TCPClientConnection::Connect(const std::string &ip, int portNum, int timeout)
{
    ipAddress = ip;
    port = portNum;

    socket = ::socket(AF_INET, SOCK_STREAM, 0);

    if (socket == INVALID_SOCKET)
        return false;

    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        closesocket(socket);
        return false;
    }

    if (::connect(socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(socket);
        return false;
    }

    connected = true;
    return true;
}

void TCPClientConnection::Disconnect()
{
    connected = false;

    if (socket != INVALID_SOCKET)
    {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }
}

void TCPClientConnection::SendData(const std::vector<char>& data, int timeBetweenAttempts)
{
    if (!connected || socket == INVALID_SOCKET)
        return;

    TcpStatusCode finalCode = TcpStatusCode::UNKNOWN;

    for (int i = 0; i <= comAttempts; i++)
    {
        {
            std::lock_guard<std::mutex> socketLock(socketMutex);

            if (socket == INVALID_SOCKET)
                break;

            int byteSend = send(socket, data.data(), static_cast<int>(data.size()), 0);

            if (byteSend == SOCKET_ERROR)
                break;

            char buffer[4096] = {0};
            int bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0)
                break;

            buffer[bytesReceived] = '\0';

            finalCode = Utils::ParseStatusCode(std::string(buffer));
        }

        timeBetweenAttempts *= 2;

        std::this_thread::sleep_for(std::chrono::milliseconds(timeBetweenAttempts));
    }
}
