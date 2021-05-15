#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "memoryviewerdialog.h"
#include "cpuwindow.h"
#include "cpu.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void createActions();

    Ui::MainWindow *ui;
    MemoryViewerDialog *memoryViewerDialog;
    CPUwindow *cpuWindow;
    QAction *stepAction;
    QAction *runAction;

    Cpu *cpu;
};
#endif // MAINWINDOW_H
