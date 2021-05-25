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
    resetRegisters();
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

    // PdPCS
    connect(this->cpu, &Cpu::PdPCS, this, [=](bool active) {
        if(active) {
            this->ui->PClineS->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->SBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->PClineS->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->SBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // ALU
    connect(this->cpu, &Cpu::ALU, this, [=](bool active, bool source, bool destination, QString operation) {
        this->ui->ALUlabel->setText(operation);

        if(active) {
            if (destination)
                this->ui->DlineALU->setStyleSheet("color: rgb(239, 41, 41);");

            if (source)
                this->ui->SlineALU->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->DlineALU->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->SlineALU->setStyleSheet("color: rgb(0, 0, 0);");
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
        this->ui->ADRview->setText("0x" + QString::number(value, 16).toUpper());

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
        this->ui->IRview->setText("0x" + QString::number(value, 16).toUpper());

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
        this->ui->PCview->setText("0x" + QString::number(value, 16).toUpper());

        if(active) {
            this->ui->PCview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->PCview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PmT
    connect(this->cpu, &Cpu::PmT, this, [=](bool active, u16 value) {
        this->ui->Tview->setText("0x" + QString::number(value, 16).toUpper());

        if(active) {
            this->ui->TlineR->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->Tview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->TlineR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->Tview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PmMDR
    connect(this->cpu, &Cpu::PmMDR, this, [=](bool active, u16 value, bool fromBUS) {
        this->ui->MDRview->setText("0x" + QString::number(value, 16).toUpper());

        if(active) {
            if(fromBUS)
                this->ui->MDRlineR->setStyleSheet("color: rgb(239, 41, 41);");
            else
                this->ui->doutlineMDR->setStyleSheet("color: rgb(239, 41, 41);");

            this->ui->MDRview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->doutlineMDR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->MDRlineR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->MDRview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PdRGS
    connect(this->cpu, &Cpu::PdRGS, this, [=](bool active) {
        if(active) {
            this->ui->GRlineS->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->SBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->GRlineS->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->SBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PdRGD
    connect(this->cpu, &Cpu::PdRGD, this, [=](bool active) {
        if(active) {
            this->ui->GRlineD->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->DBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->GRlineD->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->DBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PdMDRS
    connect(this->cpu, &Cpu::PdMDRS, this, [=](bool active) {
        if(active) {
            this->ui->MDRlineS->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->SBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->MDRlineS->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->SBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PdMDRD
    connect(this->cpu, &Cpu::PdMDRD, this, [=](bool active) {
        if(active) {
            this->ui->MDRlineD->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->DBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->MDRlineD->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->DBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PdTS
    connect(this->cpu, &Cpu::PdTS, this, [=](bool active) {
        if(active) {
            this->ui->TlineS->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->SBUSview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->TlineS->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->SBUSview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PmRG
    connect(this->cpu, &Cpu::PmRG, this, [=](bool active, u8 index, u16 value) {
        if(active) {
            this->ui->GRlineR->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->RGview->setStyleSheet("color: rgb(239, 41, 41);");
            switch (index) {
            case 0:
                this->ui->r0->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r0->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 1:
                this->ui->r1->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r1->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 2:
                this->ui->r2->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r2->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 3:
                this->ui->r3->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r3->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 4:
                this->ui->r4->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r4->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 5:
                this->ui->r5->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r5->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 6:
                this->ui->r6->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r6->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 7:
                this->ui->r7->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r7->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 8:
                this->ui->r8->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r8->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 9:
                this->ui->r9->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r9->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 10:
                this->ui->r10->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r10->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 11:
                this->ui->r11->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r11->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 12:
                this->ui->r12->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r12->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 13:
                this->ui->r13->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r13->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 14:
                this->ui->r14->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r14->setText("0x" + QString::number(value, 16).toUpper());
                break;
            case 15:
                this->ui->r15->setStyleSheet("color: rgb(239, 41, 41);");
                this->ui->r15->setText("0x" + QString::number(value, 16).toUpper());
                break;
            default:
                break;
            }
        }
        else {
            this->ui->GRlineR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->RGview->setStyleSheet("color: rgb(0, 0, 0);");

            this->ui->r0->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r1->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r2->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r3->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r4->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r5->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r6->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r7->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r8->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r9->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r10->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r11->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r12->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r13->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r14->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->r15->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // WR
    connect(this->cpu, &Cpu::WR, this, [=](bool active, QString operation) {
        this->ui->memoryLabel->setText(operation);

        if(active) {
            this->ui->addressline->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->dinline->setStyleSheet("color: rgb(239, 41, 41);");
            this->ui->memoryLabel->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->addressline->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->dinline->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->memoryLabel->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });

    // PmFLAG
    connect(this->cpu, &Cpu::PmFLAG, this, [=](bool active, u16 value, bool fromBUS) {
        this->ui->FLAGview->setText("0b" + QString::number(value, 2).toUpper());

        if(active) {
            if(fromBUS)
                this->ui->FLAGlineR->setStyleSheet("color: rgb(239, 41, 41);");
            else
                this->ui->CONDline->setStyleSheet("color: rgb(239, 41, 41);");

            this->ui->FLAGview->setStyleSheet("color: rgb(239, 41, 41);");
        }
        else {
            this->ui->FLAGlineR->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->CONDline->setStyleSheet("color: rgb(0, 0, 0);");
            this->ui->FLAGview->setStyleSheet("color: rgb(0, 0, 0);");
        }
    });
}

void CPUwindow::resetRegisters()
{
    this->ui->r0->setText(QString::number(0));
    this->ui->r1->setText(QString::number(0));
    this->ui->r2->setText(QString::number(0));
    this->ui->r3->setText(QString::number(0));
    this->ui->r4->setText(QString::number(0));
    this->ui->r5->setText(QString::number(0));
    this->ui->r6->setText(QString::number(0));
    this->ui->r7->setText(QString::number(0));
    this->ui->r8->setText(QString::number(0));
    this->ui->r9->setText(QString::number(0));
    this->ui->r10->setText(QString::number(0));
    this->ui->r11->setText(QString::number(0));
    this->ui->r12->setText(QString::number(0));
    this->ui->r13->setText(QString::number(0));
    this->ui->r14->setText(QString::number(0));
    this->ui->r15->setText(QString::number(0));
}
