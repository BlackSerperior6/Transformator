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
                            PortsConnection* connection = nullptr);
    ~ConnectionEdit();

private slots:
    void on_ConTypeSelectionFirst_currentIndexChanged(int index);

    void on_ConTypeSelectionSecond_currentIndexChanged(int index);

    void on_SaveConnectionButton_clicked();

    PortsConnection* CreatConnection();

    PortsConnection* portsConnection;

private:
    Ui::ConnectionEdit *ui;

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
