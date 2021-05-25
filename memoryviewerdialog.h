#ifndef MEMORYVIEWERDIALOG_H
#define MEMORYVIEWERDIALOG_H

#include <QDialog>
#include "cpu.h"
#include "memoryviewer.h"

namespace Ui {
class MemoryViewerDialog;
}

class MemoryViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MemoryViewerDialog(QWidget *parent = nullptr);
    ~MemoryViewerDialog();

    void setCpu(Cpu *cpu);

public slots:
    void setMemoryViewerData(const QByteArray &ba);

private:
    void connectBackend();

    Ui::MemoryViewerDialog *ui;
    MemoryViewer *memoryViewer;
    Cpu *cpu;
};

#endif // MEMORYVIEWERDIALOG_H
