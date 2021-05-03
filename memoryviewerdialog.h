#ifndef MEMORYVIEWERDIALOG_H
#define MEMORYVIEWERDIALOG_H

#include <QDialog>
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

public slots:
    void setMemoryViewerData(const QByteArray &ba);

private:
    Ui::MemoryViewerDialog *ui;
    MemoryViewer *memoryViewer;
};

#endif // MEMORYVIEWERDIALOG_H
