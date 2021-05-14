#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <codeeditor.h>
#include <QToolBar>
#include <QProcess>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    memoryViewerDialog = new MemoryViewerDialog(this);

    createActions();
    cpuWindow = new CPUwindow(this);
    cpu = new Cpu(this);
    cpuWindow->setCpu(cpu);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions() {
    // Menus
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QMenu *executeMenu = menuBar()->addMenu(tr("&Execute"));

    // Toolbars
    QToolBar *fileToolBar = addToolBar(tr("File"));
    QToolBar *viewToolBar = addToolBar(tr("View"));
    QToolBar *executeToolBar = addToolBar(tr("Execute"));

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
    QAction *viewArchitectureAction = new QAction(tr("Ar&chitecture"), this);
    viewArchitectureAction->setIcon(QPixmap(":/rec/resources/icons/cpu.svg"));
    viewArchitectureAction->setShortcut(QKeySequence(tr("Shift+A")));
    viewArchitectureAction->setStatusTip(tr("View processor architecture"));

    connect(viewArchitectureAction, &QAction::triggered, this, [this]() {
        //cpuWindow->setCpu(cpu);
        cpuWindow->show();
    });

    viewMenu->addAction(viewArchitectureAction);
    viewToolBar->addAction(viewArchitectureAction);

    // View memory action
    QAction *viewMemoryAction = new QAction(tr("&Memory"), this);
    viewMemoryAction->setIcon(QPixmap(":/rec/resources/icons/ram.svg"));
    viewMemoryAction->setShortcut(QKeySequence(tr("Shift+M")));
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

    // Assemble action
    QAction *assembleAction = new QAction(tr("&Assemble"), this);
    assembleAction->setIcon(QPixmap(":/rec/resources/icons/assembler.svg"));
    assembleAction->setShortcut(QKeySequence(tr("Ctrl+A")));
    assembleAction->setStatusTip(tr("Assemble the code"));

    connect(this->ui->plainTextEdit, &CodeEditor::loadFinished, this, [=]() {assembleAction->setEnabled(true);});
    connect(assembleAction, &QAction::triggered, this, [=]() {
        QProcess process;
        QString command("../xasm-simulator/resources/assembler/xasm ");
        command += this->ui->plainTextEdit->getFileName();
        process.start(command);
        //wait forever until finished
        process.waitForFinished(-1);

        QString output = process.readAllStandardOutput();
        QString errors = process.readAllStandardError();

        QMessageBox messageBox;
        if (output.endsWith("generated successfully\n")) {
            //reinitialize cpu if reassembled
            cpu = new Cpu(this);
            cpuWindow->setCpu(cpu);
            messageBox.information(this, "Success", "Assembled successfully!\nNow you can start the simulation.");
            stepAction->setEnabled(true);
            runAction->setEnabled(true);
        }
        else {
            messageBox.critical(this, "Assembler Error", errors);
        }

        this->cpu->resetActivatedSignals();
    });

    assembleAction->setEnabled(false);
    executeMenu->addAction(assembleAction);
    executeToolBar->addAction(assembleAction);

    // Step action
    stepAction = new QAction(tr("S&tep"), this);
    stepAction->setIcon(QPixmap(":/rec/resources/icons/step.svg"));
    stepAction->setShortcut(QKeySequence(tr("F7")));
    stepAction->setStatusTip(tr("Execute one impulse"));

    connect(stepAction, &QAction::triggered, this, [=]() {
        if(!cpu->advance()) {
            QMessageBox messageBox;
            messageBox.information(this, "Processor halted", cpu->getReason());
            stepAction->setEnabled(false);
            runAction->setEnabled(false);
        }
    });

    stepAction->setEnabled(false);
    executeMenu->addAction(stepAction);
    executeToolBar->addAction(stepAction);

    // Run action
    runAction = new QAction(tr("&Run"), this);
    runAction->setIcon(QPixmap(":/rec/resources/icons/run.svg"));
    runAction->setShortcut(QKeySequence(tr("F8")));
    runAction->setStatusTip(tr("Run the simulation"));

    connect(runAction, &QAction::triggered, this, [=]() {
        while(cpu->advance());

        QMessageBox messageBox;
        messageBox.information(this, "Processor halted", cpu->getReason());
        stepAction->setEnabled(false);
        runAction->setEnabled(false);
    });

    runAction->setEnabled(false);
    executeMenu->addAction(runAction);
    executeToolBar->addAction(runAction);
}
