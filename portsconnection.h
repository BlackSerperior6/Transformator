#ifndef PORTSCONNECTION_H
#define PORTSCONNECTION_H

#include "abstractport.h"
#include <vector>
#include <QWidget>

class PortsConnection : public QWidget
{
    Q_OBJECT
public:
    PortsConnection(AbstractPort* portOne,  AbstractPort* portTwo);

    ~PortsConnection();

    std::vector<std::string> StoredData;

    AbstractPort* firstPort;
    AbstractPort* secondPort;

private:


signals:
};
#endif // PORTSCONNECTION_H
