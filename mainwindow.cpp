#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <codeeditor.h>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    memoryViewerDialog = new MemoryViewerDialog(this);

    createActions();
    cpuWindow = new CPUwindow(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions() {
    // Menus
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    // Toolbars
    QToolBar *fileToolBar = addToolBar(tr("File"));
    QToolBar *viewToolBar = addToolBar(tr("View"));

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

    // View processor architecture action
    QAction *viewArchitectureAction = new QAction(tr("&Architecture"), this);
    viewArchitectureAction->setIcon(QPixmap(":/rec/resources/icons/cpu.svg"));
    viewArchitectureAction->setShortcut(QKeySequence(tr("Ctrl+A")));
    viewArchitectureAction->setStatusTip(tr("View processor architecture"));

    connect(viewArchitectureAction, &QAction::triggered, this, [this]() {cpuWindow->show();});

    viewMenu->addAction(viewArchitectureAction);
    viewToolBar->addAction(viewArchitectureAction);

    // View memory action
    QAction *viewMemoryAction = new QAction(tr("&Memory"), this);
    viewMemoryAction->setIcon(QPixmap(":/rec/resources/icons/ram.svg"));
    viewMemoryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    viewMemoryAction->setStatusTip(tr("View memory content"));

    connect(viewMemoryAction, &QAction::triggered, this->memoryViewerDialog,
            [this]() {
        // TOOD(Moldo) change this when we implement the processor
        unsigned char test[20] = {0x12, 0x10, 0x74, 0x44, 0x66, 0x43, 0x24, 0x76, 0x99, 0x56, 0x44, 0x22, 0x10, 0x12, 0x14, 0x24, 0x66};
        memoryViewerDialog->setMemoryViewerData(reinterpret_cast<char*>(test));
        memoryViewerDialog->show();
    });

    viewMenu->addAction(viewMemoryAction);
    viewToolBar->addAction(viewMemoryAction);
}
