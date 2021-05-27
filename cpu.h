#ifndef CPU_H
#define CPU_H

#include <QObject>
#include "assembler/defs.h"
#include <cgb.h>

enum AddressingModes {
       AM = 0x0,
       AD = 0x1,
       AI = 0x2,
       AX = 0x3
   };

enum class InstructionClass {
    b1,
    b2,
    b3,
    b4
};

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
    void PdPCS(bool active);
    void ALU(bool active, bool source, bool destination, QString operation = "ALU");
    void PdALU(bool active);
    void PmADR(bool active, u16 value = 0);
    void RD(bool active, QString operation = "MEMORY");
    void PmIR(bool active, u16 value = 0);
    void PCchanged(bool active, u16 value = 0);
    void PmT(bool active, u16 value = 0);
    void PmMDR(bool active, u16 value = 0, bool fromBUS = false);
    void PdRGS(bool active);
    void PdRGD(bool active);
    void PdMDRS(bool active);
    void PdMDRD(bool active);
    void PdTS(bool active);
    void PmRG(bool active, u8 index = 17, u16 value = 0);
    void WR(bool active, QString operation = "MEMORY");
    void PmFLAG(bool active, u16 value = 0, bool fromBUS = false);

    void PmMem(std::vector<u8> mem);

private:
    // Phases
    void instructionFetch();
    void operandFetch();
    void execute();
    void interrupt();

    /* instructions */
    // b1 class
    void mov();
    void add();
    void sub();
    void cmp();
    void AND();
    void OR();
    void XOR();

    // b2
    void clr();
    void neg();
    void inc();
    void dec();
    void asl();
    void asr();
    void lsr();
    void ror();
    void rlc();
    void rrc();
    void jmp();
    void call();
    void pushRi();
    void popRi();

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

    /* misc */
    void decideNextPhase();
    void setC(bool value);
    void setZ();
    void setS();
    void setV(bool isAdding);
    bool checkC(bool isAdding);
    bool checkZ();
    bool checkS();
    bool checkV(bool isAdding);

    int mas;
    int mad;
    InstructionClass instructionClass;
    bool intr;

    bool halt;
    QString reason;
};

#endif // CPU_H
