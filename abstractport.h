#ifndef ABSTRACTPORT_H
#define ABSTRACTPORT_H

#include <string>
#include <vector>

class AbstractPort
{
public:
    AbstractPort();

    virtual bool Accept(std::string errorMessage, const std::vector<char> & data);

protected:
    AbstractPort* targetPort;
};

#endif // ABSTRACTPORT_H
