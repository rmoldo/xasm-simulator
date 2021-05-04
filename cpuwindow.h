#ifndef CPUWINDOW_H
#define CPUWINDOW_H

#include <QDialog>
#include <QPainter>

namespace Ui {
class CPUwindow;
}

class CPUwindow : public QDialog
{
    Q_OBJECT

public:
    explicit CPUwindow(QWidget *parent = nullptr);
    ~CPUwindow();

    virtual void paintEvent(QPaintEvent * event);

private:
    Ui::CPUwindow *ui;
};

#endif // CPUWINDOW_H
