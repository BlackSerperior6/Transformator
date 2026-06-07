#include "tcpport.h"

TcpPort::TcpPort(int listenPortNum, int conId, PortType type, AbstractPort* target,
                 const std::set<std::string>& targetIPsList) : targetNetworkPort(listenPortNum),
    listenSocket(INVALID_SOCKET), AbstractPort(conId, type, target)
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

void TcpPort::SetErrorCallback(std::function<void (int, int, const std::string &)> callback)
{
    errorCallback = callback;
}

void TcpPort::SetRetryConfig(int maxRetries, int delayMs)
{
    maxRetryCount = maxRetries;
    retryDelayMs = delayMs;
}

void TcpPort::SetReceiveCallback(std::function<void(const std::vector<char>&)> callback)
{
    receiveCallback = callback;
}

void TcpPort::SetServerReceiveCallback(std::function<void (const std::string &, const std::vector<char> &)> callback)
{
    serverreceiveCallback = callback;
}

bool TcpPort::Start()
{
    if (isRunning)
        return true;

    isRunning = true;

    if (targetPort == nullptr)
    {
        return StartClient();
    }
    else
    {
        return StartServer();
    }
}

void TcpPort::Stop()
{
    isRunning = false;

    if (targetPort == nullptr)
    {
        StopClient();
    }
    else
    {
        StopServer();
    }
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
            {
                return true;
            }
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
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                }
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
