#ifndef PORTSCONNECTION_H
#define PORTSCONNECTION_H

#include "abstractport.h"
#include <vector>
#include <QWidget>

class PortsConnection : public QWidget
{
    Q_OBJECT
public:
    PortsConnection();

    ~PortsConnection();

    void SetPorts(AbstractPort* portOne,  AbstractPort* portTwo);

    std::vector<std::string> StoredData;

    AbstractPort* firstPort;
    AbstractPort* secondPort;

signals:
};
#endif // PORTSCONNECTION_H
