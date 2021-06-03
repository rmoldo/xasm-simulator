#include <memory-viewer/memoryviewerdialog.h>
#include "ui_memoryviewerdialog.h"

MemoryViewerDialog::MemoryViewerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MemoryViewerDialog)
{
    ui->setupUi(this);
    memoryViewer = new MemoryViewer(this);
    memoryViewer->setData("Hello World");
    this->layout()->addWidget(memoryViewer);
}

MemoryViewerDialog::~MemoryViewerDialog()
{
    delete ui;
}

void MemoryViewerDialog::setCpu(Cpu *cpu)
{
    this->cpu = cpu;
    connectBackend();
}

void MemoryViewerDialog::setMemoryViewerData (const QByteArray &ba) {
    memoryViewer->setData(ba);
}

void MemoryViewerDialog::connectBackend()
{
    connect(this->cpu, &Cpu::PmMem, this, [&](std::vector<u8> mem) {
        this->setMemoryViewerData({reinterpret_cast<char*>(mem.data()), (int)mem.size()});
    });
}
