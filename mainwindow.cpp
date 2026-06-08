#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    currentConnectionIndex = -1;

    connectionCounter = ui->ConnectionsList->count();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::WriteErrorToLogs(int connectionId, int errorCode, const std::string& errorText)
{
    std::lock_guard<std::mutex> lock(mtx);

    std::string finalErrorText = "Error in connection " + std::to_string(connectionId) + ": "
            + "Error code - " + std::to_string(errorCode) + ", " + errorText;

    new QListWidgetItem(tr(finalErrorText.c_str()), ui->ErrorsLogs);
}


void MainWindow::on_CreateConnectionButton_clicked()
{
    ConnectionEdit* editWindow = new ConnectionEdit(this, nullptr, connectionCounter);

    editWindow->setModal(true);
    editWindow->exec();

    if (editWindow->addedAConnection)
    {
        QListWidgetItem *item = new QListWidgetItem;
        PortsConnection* connection = editWindow->portsConnection;
        ui->ConnectionsList->addItem(item);
        ui->ConnectionsList->setItemWidget(item, connection);
    }
}

void MainWindow::on_DeleteConnection_clicked()
{
    if (currentConnectionIndex == -1)
        return;

    QListWidgetItem *it = ui->ConnectionsList->item(currentConnectionIndex);

    QWidget *widget = ui->ConnectionsList->itemWidget(it);

    delete widget;

    delete ui->ConnectionsList->takeItem(currentConnectionIndex);

    ui->ConnectionsList->setCurrentRow(-1);
}

void MainWindow::on_ConnectionsList_currentRowChanged(int currentRow)
{
    currentConnectionIndex = currentRow;
}

void MainWindow::on_ConnectionsList_itemDoubleClicked(QListWidgetItem *item)
{
    PortsConnection *connection =  (PortsConnection*) ui->ConnectionsList->itemWidget(item);

    ConnectionEdit *win = new ConnectionEdit(this, connection, connectionCounter);

    win->setModal(true);
    win->exec();
}
