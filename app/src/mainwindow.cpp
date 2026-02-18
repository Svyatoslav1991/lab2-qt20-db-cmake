#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setupMenus_();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onCreateConnection()
{

}

void MainWindow::onCloseConnection()
{

}

void MainWindow::onCreateTable()
{

}

void MainWindow::onInsertInto()
{

}

void MainWindow::onPrintTable()
{

}

void MainWindow::onDropTable()
{

}

void MainWindow::onInitTableModel()
{

}

void MainWindow::onSelectTable()
{

}

void MainWindow::onInsertRow()
{

}

void MainWindow::onRemoveRow()
{

}

void MainWindow::onDoQuery()
{

}

void MainWindow::setupMenus_()
{
    // --- BD ---
    QMenu* mBd = menuBar()->addMenu("BD");
    QAction* aCreateConn = mBd->addAction("Create connection");
    QAction* aCloseConn  = mBd->addAction("Close connection");
    mBd->addSeparator();
    QAction* aCreateTbl  = mBd->addAction("Create table");
    QAction* aInsertInto = mBd->addAction("Insert into");
    QAction* aPrintTbl   = mBd->addAction("Print table");
    QAction* aDropTbl    = mBd->addAction("Drop table");

    // --- Model ---
    QMenu* mModel = menuBar()->addMenu("Model");
    QAction* aInitModel   = mModel->addAction("Init table model");
    QAction* aSelectTable = mModel->addAction("Select table");
    QAction* aInsertRow   = mModel->addAction("Insert row");
    QAction* aRemoveRow   = mModel->addAction("Remove row");

    // --- Query ---
    QMenu* mQuery = menuBar()->addMenu("Query");
    QAction* aDoQuery = mQuery->addAction("Do query");

    // Связи: triggered -> слоты
    connect(aCreateConn, &QAction::triggered, this, &MainWindow::onCreateConnection);
    connect(aCloseConn,  &QAction::triggered, this, &MainWindow::onCloseConnection);
    connect(aCreateTbl,  &QAction::triggered, this, &MainWindow::onCreateTable);
    connect(aInsertInto, &QAction::triggered, this, &MainWindow::onInsertInto);
    connect(aPrintTbl,   &QAction::triggered, this, &MainWindow::onPrintTable);
    connect(aDropTbl,    &QAction::triggered, this, &MainWindow::onDropTable);

    connect(aInitModel,   &QAction::triggered, this, &MainWindow::onInitTableModel);
    connect(aSelectTable, &QAction::triggered, this, &MainWindow::onSelectTable);
    connect(aInsertRow,   &QAction::triggered, this, &MainWindow::onInsertRow);
    connect(aRemoveRow,   &QAction::triggered, this, &MainWindow::onRemoveRow);

    connect(aDoQuery, &QAction::triggered, this, &MainWindow::onDoQuery);
}
