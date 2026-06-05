#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::WriteToLogs(std::string logText)
{
    std::lock_guard<std::mutex> lock(mtx);
    new QListWidgetItem(tr(logText.c_str()), ui->listWidget);
}

