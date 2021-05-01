#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <codeeditor.h>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    createActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));

    // Open file action
    QAction *openAction = new QAction(tr("&Open"), this);
    openAction->setIcon(QPixmap(":/rec/resources/icons/folder.svg"));
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(tr("Open an XASM file"));

    connect(openAction, &QAction::triggered, this->ui->plainTextEdit, &CodeEditor::open);

    fileMenu->addAction(openAction);
    fileToolBar->addAction(openAction);

    // Quit action
    QAction *quitAction = new QAction(tr("&Quit"), this);
    quitAction->setIcon(QPixmap(":/rec/resources/icons/quit.svg"));
    quitAction->setShortcut(QKeySequence::Quit);
    quitAction->setStatusTip(tr("Quit XASM Simulator"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::closeAllWindows, Qt::QueuedConnection);
    fileMenu->addAction(quitAction);
}
