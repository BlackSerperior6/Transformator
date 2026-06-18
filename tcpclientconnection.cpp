#include "tcpclientconnection.h"

TCPClientConnection::TCPClientConnection(std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> callback,
                                         int conId, ThreadPool* providedThreadPool)
    : isRunning(false), errorCallback(callback), connectionId(conId), pool(providedThreadPool)
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
}

void TCPClientConnection::SendData(const std::vector<char> &data)
{
    pool->AddTask([this, data]{this->SendDataWithRetries(data, this->timeoutMs);});
}

void TCPClientConnection::SendDataWithRetries(const std::vector<char>& data, int timeoutMs)
{
    if (!isRunning)
        return;

    TcpStatusCode finalCode = TcpStatusCode::TIMEOUT;

    const int BUFFER_SIZE = 65536;
    std::vector<char> buffer(BUFFER_SIZE);

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

        int bytesReceived = recv(socket, buffer.data(), BUFFER_SIZE, 0);

        if (bytesReceived <= 0)
        {
            finalCode = TcpStatusCode::RECEIVE_ERROR;
            closesocket(socket);
            continue;
        }
        else
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
                }
            }
        }

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
