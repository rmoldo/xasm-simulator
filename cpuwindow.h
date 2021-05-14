#ifndef CPUWINDOW_H
#define CPUWINDOW_H

#include <QDialog>
#include <QPainter>
#include <cpu.h>

namespace Ui {
class CPUwindow;
}

class CPUwindow : public QDialog
{
    Q_OBJECT

public:
    explicit CPUwindow(QWidget *parent = nullptr);
    ~CPUwindow();

    void setCpu(Cpu *cpu);
    virtual void paintEvent(QPaintEvent * event);

private:
    void connectBackend();

    Ui::CPUwindow *ui;
    Cpu *cpu;
    QColor aluColor;
};

#endif // CPUWINDOW_H
