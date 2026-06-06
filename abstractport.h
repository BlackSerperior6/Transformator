#ifndef ABSTRACTPORT_H
#define ABSTRACTPORT_H

#include <string>
#include <vector>

class AbstractPort
{
public:
    AbstractPort();

    virtual bool Accept(const std::vector<char> & data);

protected:
    AbstractPort* targetPort;

    int connectionId;
};

#endif // ABSTRACTPORT_H
