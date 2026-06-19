#include "portsconnection.h"

PortsConnection::PortsConnection()
{
    StoredData.clear();
}

PortsConnection::~PortsConnection()
{
    delete firstPort;
    delete secondPort;
}

void PortsConnection::SetPorts(AbstractPort *portOne, AbstractPort *portTwo)
{
    firstPort = portOne;
    secondPort = portTwo;
}
