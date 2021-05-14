#include "cpuwindow.h"
#include "ui_cpuwindow.h"
#include <QAction>

CPUwindow::CPUwindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPUwindow)
{
    ui->setupUi(this);
    QWidget::setFixedSize(this->size());
    aluColor = Qt::transparent;
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

    QBrush brush;
    brush.setColor(aluColor);
    brush.setStyle(Qt::SolidPattern);

    QPainterPath path;
    path.addPolygon(alu);

    painter.drawPolygon(alu);
    painter.fillPath(path, brush);
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
            aluColor = QColor(239, 41, 41);
        }
        else {
            this->ui->DlineALU->setStyleSheet("color: rgb(0, 0, 0);");
            aluColor = QColor(250, 250, 250);
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
}
