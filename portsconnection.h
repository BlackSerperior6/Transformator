#ifndef PORTSCONNECTION_H
#define PORTSCONNECTION_H

#include "abstractport.h"

class PortsConnection
{
public:
    PortsConnection(AbstractPort* portOne,  AbstractPort* portTwo);

    AbstractPort* firstPort;
    AbstractPort* secondPort;
};

#endif // PORTSCONNECTION_H
