#include "connectionedit.h"
#include "ui_connectionedit.h"

ConnectionEdit::ConnectionEdit(QWidget *parent, PortsConnection* connection, int connectionCounter,
                               std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> callback) :
    QDialog(parent),
    portsConnection(connection),
    addedAConnection(false),
    ui(new Ui::ConnectionEdit),
    errorCallback(callback)
{
    ui->setupUi(this);

    PortType portOneType;
    PortType portTwoType;

    bool isConnectionPresent = portsConnection != nullptr;

    updatedConnectionCounter = connectionCounter;

    if (!isConnectionPresent)
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


        if (isConnectionPresent)
        {
            SerialPort* portOne = (SerialPort*) portsConnection->firstPort;
            ui->COMEdit1->setText(QString::fromStdString(portOne->GetPortName()));
        }

    }
    else if (portOneType == PortType::TCPPort)
    {
        ui->IPEdit2->setVisible(false);
        ui->IPLabel2->setVisible(false);
        ui->NetLabel2->setVisible(false);
        ui->NETPortEdit2->setVisible(false);

        ui->COMEdit2->setVisible(true);
        ui->ComLabel2->setVisible(true);

        if (isConnectionPresent)
        {
            TcpPort* portOne = (TcpPort*) portsConnection->firstPort;

            QStringList lines;

            ui->NetLabel1->setText(QString::fromStdString(std::to_string(portOne->GetTargetNetworkPort())));

            for (const auto& item : portOne->GetTargetIps())
                 lines.append(QString::fromStdString(item));

            ui->IPEdit1->setPlainText(lines.join('\n'));
        }
    }

    if (portTwoType == PortType::COMPort)
    {
        ui->IPEdit2->setVisible(false);
        ui->IPLabel2->setVisible(false);
        ui->NetLabel2->setVisible(false);
        ui->NETPortEdit2->setVisible(false);

        ui->COMEdit2->setVisible(true);
        ui->ComLabel2->setVisible(true);

        if (isConnectionPresent)
        {
            SerialPort* portTwo = (SerialPort*) portsConnection->secondPort;
            ui->COMEdit2->setText(QString::fromStdString(portTwo->GetPortName()));
        }
    }
    else if (portTwoType == PortType::TCPPort)
    {
        ui->COMEdit2->setVisible(false);
        ui->ComLabel2->setVisible(false);

        ui->IPEdit2->setVisible(true);
        ui->IPLabel2->setVisible(true);
        ui->NetLabel2->setVisible(true);
        ui->NETPortEdit2->setVisible(true);

        if (isConnectionPresent)
        {
            TcpPort* portTwo = (TcpPort*) portsConnection->secondPort;

            QStringList lines;

            ui->NetLabel2->setText(QString::fromStdString(std::to_string(portTwo->GetTargetNetworkPort())));

            for (const auto& item : portTwo->GetTargetIps())
                 lines.append(QString::fromStdString(item));

            ui->IPEdit2->setPlainText(lines.join('\n'));
        }
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
        updatedConnectionCounter++;
    }

    addedAConnection = true;

    portsConnection = CreateConnection();

    portsConnection->firstPort->SetErrorCallback(errorCallback);
    portsConnection->secondPort->SetErrorCallback(errorCallback);

    close();
}

PortsConnection* ConnectionEdit::CreateConnection()
{
    AbstractPort* firstPort;
    AbstractPort* secondport;

    switch (ui->ConTypeSelectionSecond->currentIndex())
    {
        case 0: // Must make a COM port
            secondport = new SerialPort(ui->COMEdit2->text().toStdString(), updatedConnectionCounter, PortType::COMPort, CBR_9600, nullptr);
            break;
        case 1: // Must make a TCP port
            secondport = new TcpPort(ui->NETPortEdit2->text().toInt(), updatedConnectionCounter, PortType::TCPPort, nullptr,
                                    ParseIpInput(ui->IPEdit2));
            break;
    }

    switch (ui->ConTypeSelectionFirst->currentIndex())
    {
        case 0: // Must make a COM port
            firstPort = new SerialPort(ui->COMEdit1->text().toStdString(), updatedConnectionCounter, PortType::COMPort, CBR_9600, secondport);
            break;
        case 1: // Must make a TCP port
            firstPort = new TcpPort(ui->NETPortEdit1->text().toInt(), updatedConnectionCounter, PortType::TCPPort, secondport,
                                    ParseIpInput(ui->IPEdit1));
            break;
    }

    return new PortsConnection(firstPort, secondport);
}

std::set<std::string> ConnectionEdit::ParseIpInput(QPlainTextEdit* plainText)
{
    std::set<std::string> result;

    QString text = plainText->toPlainText();

    QStringList lines = text.split('\n');

    for (const QString& line : lines)
    {
        std::string str = line.trimmed().toStdString();

        if (!str.empty())
            result.insert(str);
    }

    return result;
}


