#include "portsconnection.h"

PortsConnection::PortsConnection(AbstractPort* portOne,  AbstractPort* portTwo) : firstPort(portOne),
    secondPort(portTwo) {}

PortsConnection::~PortsConnection()
{
    delete firstPort;
    delete secondPort;
}
