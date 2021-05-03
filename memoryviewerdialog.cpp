#include "memoryviewerdialog.h"
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

void MemoryViewerDialog::setMemoryViewerData (const QByteArray &ba) {
    memoryViewer->setData(ba);
}
