#ifndef ABSTRACTPORT_H
#define ABSTRACTPORT_H

#include "PortType.h"

#include <string>
#include <vector>
#include <atomic>

class AbstractPort
{
public:
    AbstractPort(int conId, PortType type, AbstractPort* target = nullptr);

    virtual bool Accept(const std::vector<char> & data);

    virtual bool Start();

    virtual void Stop();

protected:
    AbstractPort* targetPort;

    PortType portType;

    int connectionId;

    std::atomic<bool> isRunning;
};

#endif // ABSTRACTPORT_H
