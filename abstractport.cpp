#include "abstractport.h"
#include "portsconnection.h"

AbstractPort::AbstractPort(int conId, PortType type, void* parentConnection, AbstractPort* target) : connectionId(conId), portType(type),
    targetPort(target), isRunning(false), ParentConnection(parentConnection) {}

AbstractPort::~AbstractPort()
{

}

void AbstractPort::Accept(const std::vector<char> & data)
{
    PortsConnection* con = (PortsConnection*) ParentConnection;

    con->StoredData.push_back(std::string(data.begin(), data.end()));
}

void AbstractPort::SetErrorCallback(std::function<void(int, int, const std::string&)> callback)
{
    errorCallback = callback;
}


