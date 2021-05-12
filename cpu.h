#ifndef CPU_H
#define CPU_H

#include <QObject>
#include "assembler/defs.h"

class Cpu : public QObject
{
    Q_OBJECT
public:
    explicit Cpu(QObject *parent = nullptr);

    void initializeRegisters();
    std::vector<u8> getMemory();

public slots:
    void setMachineCodeInMemory(u8 *data, size_t size);

signals:
    void memoryChanged();

private:
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
    // TODO(Silvan) add bgc when ready
};

#endif // CPU_H
