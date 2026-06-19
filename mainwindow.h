#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mutex>

#include "connectionedit.h"
#include "QListWidgetItem"
#include "QMessageBox"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    std::mutex mtx;

    void WriteErrorToLogs(int, int, const std::string&);

private slots:
    void on_CreateConnectionButton_clicked();

    void on_DeleteConnection_clicked();

    void on_ConnectionsList_currentRowChanged(int currentRow);

    void on_ConnectionsList_itemDoubleClicked(QListWidgetItem *item);

    void on_ClearErrorLogsButton_clicked();

    void on_UpdateCurrentDataView_clicked();

private:
    Ui::MainWindow *ui;

    int connectionCounter;

    int currentConnectionIndex;

    void UpdateDataView();
};
#endif // MAINWINDOW_H
