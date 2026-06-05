#include "tcpport.h"

TcpPort::TcpPort() : listenSocket(INVALID_SOCKET), clientSocket(INVALID_SOCKET),
isRunning(false), remotePort(0), localPort(0)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

TcpPort::~TcpPort()
{
    Stop();


}
