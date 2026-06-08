#include "connectionedit.h"
#include "ui_connectionedit.h"

ConnectionEdit::ConnectionEdit(QWidget *parent, PortsConnection* connection, int connectionCounter,
                               std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> callback) :
    QDialog(parent),
    ui(new Ui::ConnectionEdit),
    portsConnection(connection),
    addedAConnection(false),
    errorCallback(callback)
{
    ui->setupUi(this);

    PortType portOneType;
    PortType portTwoType;

    updatedConnectionCounter = connectionCounter;

    if (portsConnection == nullptr)
    {
        portOneType = PortType::COMPort;
        portTwoType = PortType::COMPort;
    }
    else
    {
        portOneType = portsConnection->firstPort->portType;
        portTwoType = portsConnection->secondPort->portType;
    }

    if (portOneType == PortType::COMPort)
    {
        ui->IPEdit1->setVisible(false);
        ui->IPLabel1->setVisible(false);
        ui->NetLabel1->setVisible(false);
        ui->NETPortEdit1->setVisible(false);

        ui->COMEdit1->setVisible(true);
        ui->ComLabel1->setVisible(true);
    }
    else if (portOneType == PortType::TCPPort)
    {
        ui->COMEdit1->setVisible(false);
        ui->ComLabel1->setVisible(false);

        ui->IPEdit1->setVisible(true);
        ui->IPLabel1->setVisible(true);
        ui->NetLabel1->setVisible(true);
        ui->NETPortEdit1->setVisible(true);
    }

    if (portTwoType == PortType::TCPPort)
    {
        ui->IPEdit2->setVisible(false);
        ui->IPLabel2->setVisible(false);
        ui->NetLabel2->setVisible(false);
        ui->NETPortEdit2->setVisible(false);

        ui->COMEdit2->setVisible(true);
        ui->ComLabel2->setVisible(true);
    }
    else if (portTwoType == PortType::TCPPort)
    {
        ui->COMEdit2->setVisible(false);
        ui->ComLabel2->setVisible(false);

        ui->IPEdit2->setVisible(true);
        ui->IPLabel2->setVisible(true);
        ui->NetLabel2->setVisible(true);
        ui->NETPortEdit2->setVisible(true);
    }
}

ConnectionEdit::~ConnectionEdit()
{
    delete ui;
}

void ConnectionEdit::on_ConTypeSelectionFirst_currentIndexChanged(int index)
{
    switch (index)
    {
        case 0: // COM port selected
            ui->IPEdit1->setVisible(false);
            ui->IPLabel1->setVisible(false);
            ui->NetLabel1->setVisible(false);
            ui->NETPortEdit1->setVisible(false);

            ui->COMEdit1->setVisible(true);
            ui->ComLabel1->setVisible(true);
            break;

        case 1: // TCP port selected
            ui->COMEdit1->setVisible(false);
            ui->ComLabel1->setVisible(false);

            ui->IPEdit1->setVisible(true);
            ui->IPLabel1->setVisible(true);
            ui->NetLabel1->setVisible(true);
            ui->NETPortEdit1->setVisible(true);
            break;
    }
}

void ConnectionEdit::on_ConTypeSelectionSecond_currentIndexChanged(int index)
{
    switch (index)
    {
        case 0: // COM port selected
            ui->IPEdit2->setVisible(false);
            ui->IPLabel2->setVisible(false);
            ui->NetLabel2->setVisible(false);
            ui->NETPortEdit2->setVisible(false);

            ui->COMEdit2->setVisible(true);
            ui->ComLabel2->setVisible(true);
            break;

        case 1: // TCP port selected
            ui->COMEdit2->setVisible(false);
            ui->ComLabel2->setVisible(false);

            ui->IPEdit2->setVisible(true);
            ui->IPLabel2->setVisible(true);
            ui->NetLabel2->setVisible(true);
            ui->NETPortEdit2->setVisible(true);
            break;
    }
}

void ConnectionEdit::on_SaveConnectionButton_clicked()
{
    if (portsConnection != nullptr)
    {
        portsConnection->firstPort->Stop();
        portsConnection->secondPort->Stop();
    }
    else
    {
        addedAConnection = true;
        updatedConnectionCounter++;
    }

    portsConnection = CreateConnection();

    portsConnection->firstPort->SetErrorCallback(errorCallback);
    portsConnection->secondPort->SetErrorCallback(errorCallback);

    portsConnection->secondPort->Start();
    portsConnection->firstPort->Start();
}

PortsConnection* ConnectionEdit::CreateConnection()
{
    AbstractPort* firstPort;
    AbstractPort* secondport;

    switch (ui->ConTypeSelectionFirst->currentIndex())
    {
        case 0: // Must make a COM port
            firstPort = new SerialPort(ui->COMEdit1->text().toStdString(), 0, PortType::COMPort, CBR_9600, secondport);
            break;
        case 1: // Must make a TCP port
            firstPort = new TcpPort(ui->NETPortEdit1->text().toInt(), 0, PortType::TCPPort, secondport,
                                    ParseIpInput(ui->IPEdit1));
            break;
    }

    switch (ui->ConTypeSelectionSecond->currentIndex())
    {
        case 0: // Must make a COM port
            secondport = new SerialPort(ui->COMEdit2->text().toStdString(), 0, PortType::COMPort, CBR_9600, nullptr);
            break;
        case 1: // Must make a TCP port
            secondport = new TcpPort(ui->NETPortEdit2->text().toInt(), 0, PortType::TCPPort, nullptr,
                                    ParseIpInput(ui->IPEdit2));
            break;
    }

    return new PortsConnection(firstPort, secondport);
}


