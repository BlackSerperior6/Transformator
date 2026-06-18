#ifndef ABSTRACTPORT_H
#define ABSTRACTPORT_H

#include "PortType.h"

#include <string>
#include <vector>
#include <atomic>
#include <functional>

class AbstractPort
{
public:
    AbstractPort(int conId, PortType type, void* parentConnection, AbstractPort* target = nullptr);

    virtual ~AbstractPort();

    virtual void Accept(const std::vector<char> & data);

    virtual bool Start() = 0;

    virtual void Stop() = 0;

    void SetErrorCallback(std::function<void(int, int, const std::string&)> callback);

    PortType portType;

protected:
    AbstractPort* targetPort;

    void* ParentConnection;

    int connectionId;

    std::atomic<bool> isRunning;

    std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> errorCallback;
};

#endif // ABSTRACTPORT_H
