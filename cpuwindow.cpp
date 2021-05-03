#include "cpuwindow.h"
#include "ui_cpuwindow.h"

CPUwindow::CPUwindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPUwindow)
{
    ui->setupUi(this);
}

CPUwindow::~CPUwindow()
{
    delete ui;
}
