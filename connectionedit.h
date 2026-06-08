#ifndef CONNECTIONEDIT_H
#define CONNECTIONEDIT_H

#include <QDialog>
#include <QPlainTextEdit>
#include "portsconnection.h"
#include "serialport.h"
#include "tcpport.h"

namespace Ui {
class ConnectionEdit;
}

class ConnectionEdit : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionEdit(QWidget *parent = nullptr,
                            PortsConnection* connection = nullptr,
                            int connectionCounter = 0,
                            std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> callback = nullptr);
    ~ConnectionEdit();

public:
    PortsConnection* portsConnection;

    int updatedConnectionCounter;

    bool addedAConnection;

private slots:
    void on_ConTypeSelectionFirst_currentIndexChanged(int index);

    void on_ConTypeSelectionSecond_currentIndexChanged(int index);

    void on_SaveConnectionButton_clicked();

private:
    Ui::ConnectionEdit *ui;

    std::function<void(int connectionNumber, int errorCode, const std::string& errorMessage)> errorCallback;

    PortsConnection* CreateConnection();

    std::set<std::string> ParseIpInput(QPlainTextEdit* plainText)
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
};

#endif // CONNECTIONEDIT_H
