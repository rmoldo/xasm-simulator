#ifndef CPU_H
#define CPU_H

#include <QObject>
#include "assembler/defs.h"
#include <cgb.h>

class Cpu : public QObject
{
    Q_OBJECT
public:
    explicit Cpu(QObject *parent = nullptr);

    void initializeRegisters();
    std::vector<u8> getMemory();

    //// Executes next impulse
    bool advance();

    //// Contains the reason for halting
    QString getReason();

    //// Resets all cpuwindow activated components
    void resetActivatedSignals();

public slots:
    void setMachineCodeInMemory(u8 *data, size_t size);

signals:
    void memoryChanged();

    // Commands
    void PdPCD(bool active);
    void ALU(bool active, QString operation = "ALU");
    void PdALU(bool active);
    void PmADR(bool active, u16 value = 0);
    void RD(bool active, QString operation = "MEMORY");
    void PmIR(bool active, u16 value = 0);
    void PCchanged(bool active, u16 value = 0);


private:
    // Phases
    void instructionFetch();
    void operandFetch();
    void execute();
    void interrupt();

    /* Memory */
    std::vector<u8> memory;

    /* Buses */
    u16 SBUS; // Source Bus
    u16 DBUS; // Destination Bus
    u16 RBUS; // Result Bus

    /* Registers */
    u16 FLAG; // Flag Register

    u16 R[16]; // General Registers
    u16 PC;    // Program Counter
    u16 SP;    // Stack Pointer
    u16 T;     // Tampon Register

    u16 IR;  // Instruction Register
    u16 MDR; // Memory Data Register
    u16 ADR; // Address Register
    u16 IVR; // Interrupt Vector Register

    /* Command generator block */
    CGB *cgb;

    /* Conditions */
    bool cil;
    bool iop;

    bool halt;
    QString reason;
};

#endif // CPU_H
