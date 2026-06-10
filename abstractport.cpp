#include "abstractport.h"

AbstractPort::AbstractPort(int conId, PortType type, AbstractPort* target) : connectionId(conId), portType(type),
    targetPort(target), isRunning(false) {}

AbstractPort::~AbstractPort()
{

}

void AbstractPort::SetErrorCallback(std::function<void(int, int, const std::string&)> callback)
{
    errorCallback = callback;
}


