#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mutex>

#include "connectionedit.h"
#include "QListWidgetItem"

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

private:
    Ui::MainWindow *ui;

    int connectionCounter;

    int currentConnectionIndex;
};
#endif // MAINWINDOW_H
