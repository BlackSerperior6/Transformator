#include "tcpclientconnection.h"

TCPClientConnection::TCPClientConnection() : socket(INVALID_SOCKET), connected(false),
    lastStatusCode(TcpStatusCode::SUCCESS), waitingForResponse(false) {}

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
    lastStatusCode = TcpStatusCode::SUCCESS;
    return true;
}

void TCPClientConnection::Disconnect()
{
    connected = false;

    if (waitingForResponse)
        responseCV.notify_all();

    if (socket != INVALID_SOCKET)
    {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }

    if (receiveThread.joinable())
    {
        receiveThread.join();
    }
}

bool TCPClientConnection::SendData(const std::vector<char>& data, TcpStatusCode& responseCode, int timeoutMs)
{
    if (!connected || socket == INVALID_SOCKET)
        return false;

    std::unique_lock<std::mutex> lock(responseMutex);
    waitingForResponse = true;
    lastStatusCode = TcpStatusCode::TIMEOUT;

    {
        std::lock_guard<std::mutex> sendLock(sendMutex);

        if (send(socket, data.data(), static_cast<int>(data.size()), 0) == SOCKET_ERROR)
        {
            waitingForResponse = false;
            return false;
        }
    }

    if (responseCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                           [this] { return !waitingForResponse; }))
    {
        responseCode = lastStatusCode;
        return true;
    }

    waitingForResponse = false;
    responseCode = TcpStatusCode::SUCCESS;
    return false;
}

void TCPClientConnection::OnResponseReceived(TcpStatusCode code)
{
    std::lock_guard<std::mutex> lock(responseMutex);
    lastStatusCode = code;
    waitingForResponse = false;
    responseCV.notify_one();
}

void TCPClientConnection::StartReceive(std::function<void(const std::vector<char>&)> callback,
                                       std::function<void(TcpStatusCode)> responseCallback)
{
    receiveThread = std::thread([this, callback, responseCallback]()
    {
        const int BUFFER_SIZE = 65536;
        std::vector<char> buffer(BUFFER_SIZE);

        while (connected)
        {
            int bytesReceived = recv(socket, buffer.data(), BUFFER_SIZE, 0);

            if (bytesReceived > 0)
            {
                std::vector<char> receivedData(buffer.begin(), buffer.begin() + bytesReceived);

                if (bytesReceived >= 4 && receivedData[0] == 'S' && receivedData[1] == 'T' &&
                    receivedData[2] == 'A' && receivedData[3] == 'T')
                {
                    if (bytesReceived >= 8)
                    {
                        int code = 0;
                        for (int i = 4; i < 8 && i < bytesReceived; i++)
                            code = code * 10 + (receivedData[i] - '0');

                        responseCallback(static_cast<TcpStatusCode>(code));
                    }
                }
                else
                {
                    if (callback)
                        callback(receivedData);
                }
            }
            else if (bytesReceived == 0)
                break;
            else
            {
                int error = WSAGetLastError();

                if (error != WSAEWOULDBLOCK && error != WSAETIMEDOUT)
                    break;
            }
        }

        connected = false;
    });

}
