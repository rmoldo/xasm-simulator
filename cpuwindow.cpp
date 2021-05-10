#include "cpuwindow.h"
#include "ui_cpuwindow.h"

CPUwindow::CPUwindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPUwindow)
{
    ui->setupUi(this);
    QWidget::setFixedSize(this->size());
}

CPUwindow::~CPUwindow()
{
    delete ui;
}

void CPUwindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QPolygon alu;
    alu << QPoint(1050, 80) << QPoint(1050, 140) << QPoint(1065, 150) << QPoint(1050, 160)
        << QPoint(1050, 220) << QPoint(1130, 175) << QPoint(1130, 125) << QPoint(1050, 80);
    painter.drawPolygon(alu);
}
