#include "abstractport.h"

AbstractPort::AbstractPort(int conId, PortType type, AbstractPort* target) : connectionId(conId), portType(type),
    targetPort(target), isRunning(false) {}


