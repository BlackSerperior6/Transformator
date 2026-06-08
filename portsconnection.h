#ifndef PORTSCONNECTION_H
#define PORTSCONNECTION_H

#include "abstractport.h"
#include <QWidget>

class PortsConnection : public QWidget
{
    Q_OBJECT
public:
    PortsConnection(AbstractPort* portOne,  AbstractPort* portTwo);

    ~PortsConnection();

    AbstractPort* firstPort;
    AbstractPort* secondPort;

signals:
};
#endif // PORTSCONNECTION_H
