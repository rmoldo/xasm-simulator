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
    delete cpu;
}

void CPUwindow::setCpu(Cpu *cpu)
{
    this->cpu = cpu;
    connectBackend();
}

void CPUwindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QPolygon alu;
    alu << QPoint(1050, 80) << QPoint(1050, 140) << QPoint(1065, 150) << QPoint(1050, 160)
        << QPoint(1050, 220) << QPoint(1130, 175) << QPoint(1130, 125) << QPoint(1050, 80);

    painter.drawPolygon(alu);
}

void CPUwindow::connectBackend()
{
    // PdPCD
    connect(this->cpu, &Cpu::PdPCD, this, [=](bool active) {
        if(active) {
            this->ui->PClineD->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->DBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->PClineD->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->DBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // ALU
    connect(this->cpu, &Cpu::ALU, this, [=](bool active, QString operation) {
        this->ui->ALUlabel->setText(operation);

        if(active) {
            this->ui->DlineALU->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->DlineALU->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PdALU
    connect(this->cpu, &Cpu::PdALU, this, [=](bool active) {
        if(active) {
            this->ui->ALUlineR->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->RBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->ALUlineR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->RBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PmADR
    connect(this->cpu, &Cpu::PmADR, this, [=](bool active, u16 value) {
        this->ui->ADRview->setText(QString::number(value));

        if(active) {
            this->ui->ADRlineR->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->ADRview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->ADRlineR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->ADRview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // RD
    connect(this->cpu, &Cpu::RD, this, [=](bool active, QString operation) {
        this->ui->memoryLabel->setText(operation);

        if(active) {
            this->ui->addressline->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->memoryLabel->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->addressline->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->memoryLabel->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PmIR
    connect(this->cpu, &Cpu::PmIR, this, [=](bool active, u16 value) {
        this->ui->IRview->setText(QString::number(value));

        if(active) {
            this->ui->doutlineIR->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->IRview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->doutlineIR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->IRview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // +2PC
    connect(this->cpu, &Cpu::PCchanged, this, [=](bool active, u16 value) {
        this->ui->PCview->setText(QString::number(value));

        if(active) {
            this->ui->PCview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->PCview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });
}
