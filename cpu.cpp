#include "cpu.h"

#include <vector>
#include <QDebug>

Cpu::Cpu(QObject *parent) : QObject(parent)
{
    memory = std::vector<u8>(1 << 16, 0);

    // Clear buses
    SBUS = 0;
    DBUS = 0;
    RBUS = 0;

    initializeRegisters();

    cgb = new CGB(this);
}

void Cpu::initializeRegisters()
{
    PC = 0;
    IR = 0;
    SP = 0;

    T = 0;

    FLAG = 0;

    ADR = 0;
    MDR = 0;
    IVR = 0;

    memset(R, 0, sizeof(R));

    // condition initialisation
    halt = false;
    reason = "Simulation finished!";

    intr = false;
}

bool Cpu::advance()
{
    resetActivatedSignals();
    switch(cgb->getPhase()) {
    case Phase::IF:
        instructionFetch();
        break;

    case Phase::OF:
        operandFetch();
        break;

    case Phase::EX:
        execute();
        break;

    case Phase::INT:
        interrupt();
        break;

    default:
        break;
    }

    return !halt;
}

QString Cpu::getReason()
{
    return reason;
}

std::vector<u8> Cpu::getMemory() {
    return memory;
}

void Cpu::setMachineCodeInMemory(u8 *data, size_t size) {
    std::fill(memory.begin(), memory.end(), 0x0);
    memory.erase(memory.begin(), memory.begin() + size);
    memory.insert(memory.begin(), data, data + size);
}

void Cpu::instructionFetch()
{
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        DBUS = PC;
        emit PdPCD(true);
        emit ALU(true, false, true, "DBUS");
        RBUS = DBUS;
        emit PdALU(true);
        ADR = RBUS;
        emit PmADR(true, ADR);
        qDebug() << "IF I1";
        break;

    case 2:
        IR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmIR(true, IR);
        PC += 2;
        emit PCchanged(true, PC);
        qDebug() << "IF I2";
        break;

    case 3: {
        bool cil = false;

        if((IR >> 15) == 0){
            instructionClass = InstructionClass::b1;
            if((IR >> 12) > 6)
                cil = true;
        }
        else {
            switch((IR >> 13) & 0x03) {
            case 0:
                instructionClass = InstructionClass::b2;
                if((IR >> 6) > 0x20E)
                    cil = true;
                break;

            case 1:
                instructionClass = InstructionClass::b3;
                if((IR >> 8) > 0xA7)
                    cil = true;
                break;

            case 2:
                instructionClass = InstructionClass::b4;
                if(IR > 0xC012)
                    cil = true;
                break;

            default:
                cil = true;
            }
        }

        if(cil) {
            halt = true;
            reason =  "CIL - illegal instruction";
            return;
        }

        if((IR & 0x8000) == 0 || ((IR >> 13) & 0x7) == 4) {
            cgb->setPhase(Phase::OF);
        }
        else {
            cgb->setPhase(Phase::EX);
        }

        qDebug() << "IF I3";
        break;
    }

    default:
        halt = true;
        reason = "Impulses out of range for Instruction Fetch phase";
        break;
    }

}

void Cpu::operandFetch()
{
    mas = (IR >> 10) & 0x3;
    mad = (IR >> 4) & 0x03;

    if ((IR & 0x8000) != 0 && (cgb->getImpulse() < 6))
        cgb->setImpluse(6);

    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        switch (mas) {
        case AM: case AX:
            SBUS = PC;
            emit PdPCS(true);
            emit ALU(true, true, false, "SBUS");

            PC += 2;
            emit PCchanged(true, PC);

            RBUS = SBUS;
            emit PdALU(true);

            ADR = RBUS;
            emit PmADR(true, ADR);

            break;
        case AD:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);
            emit ALU(true, true, false, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            cgb->setImpluse(6);

            break;
        case AI:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);
            emit ALU(true, true, false, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            ADR = RBUS;
            PmADR(true, ADR);

            break;
        default:
            break;
        }

        qDebug() << "SURSA OF I1";
        break;

    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "SURSA OF I2";
        break;

    case 3:
        switch(mas) {
        case AM: case AI:
            SBUS = MDR;
            PdMDRS(true);

            emit ALU(true, true, false, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            cgb->setImpluse(6);

            qDebug() << "SURSA OF I3";
            break;
        case AX:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);

            DBUS = MDR;
            emit PdMDRD(true);

            emit ALU(true, true, true, "SUM");

            RBUS = SBUS + DBUS;
            emit PdALU(true);

            ADR = RBUS;
            emit PmADR(true, ADR);

             qDebug() << "SURSA OF I3";
            break;
        default:
            qDebug() << "ERROR SURSA OF I3";
            break;
        }

        break;
    case 4:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "SURSA OF I4";

        break;
    case 5:
        SBUS = MDR;
        PdMDRS(true);

        emit ALU(true, true, false, "SBUS");

        RBUS = SBUS;
        emit PdALU(true);

        T = RBUS;
        emit PmT(true, T);

        qDebug() << "SURSA OF I5";
        break;
    case 6:
        switch (mad) {
        case AM: case AX:
            DBUS = PC;
            emit PdPCD(true);
            emit ALU(true, false, true, "DBUS");

            PC += 2;
            emit PCchanged(true, PC);

            RBUS = DBUS;
            emit PdALU(true);

            ADR = RBUS;
            emit PmADR(true, ADR);

            break;
        case AD:
            DBUS = R[IR & 0xf];
            emit PdRGD(true);
            emit ALU(true, false, true, "DBUS");

            RBUS = DBUS;
            emit PdALU(true);

            MDR = RBUS;
            emit PmMDR(true, MDR, true);

            cgb->setPhase(Phase::EX);
            break;
        case AI:
            DBUS = R[IR & 0xf];
            emit PdRGD(true);
            emit ALU(true, false, true, "DBUS");

            RBUS = DBUS;
            emit PdALU(true);

            ADR = RBUS;
            emit PmADR(true, ADR);
            break;

        default:
            qDebug() << "ERROR I1 MAD";
            break;
        }

        qDebug() << "DESTINATIE OF I1";
        break;
    case 7:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        if (mad == AM || mad == AI)
            cgb->setPhase(Phase::EX);

        qDebug() << "DESTINATIE OF I2";
        break;
    case 8:
        DBUS = R[IR & 0xf];
        emit PdRGD(true);

        SBUS = MDR;
        emit PdMDRS(true);

        emit ALU(true, true, true, "SUM");

        RBUS = SBUS + DBUS;
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "DESTINATIE OF I3";

        break;
    case 9:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        cgb->setPhase(Phase::EX);

        qDebug() << "DESTINATIE OF I4";

        break;
    default:
        qDebug() << "ERROR";
        break;
    }
}

void Cpu::execute()
{
    switch (instructionClass) {
    case InstructionClass::b1: {
        switch (IR >> 12) {
        case 0:
            mov();
            break;
        case 1:
            add();
            break;
        case 2:
            sub();
            break;
        default:
            qDebug() << "Instruction not defined";
            break;
        }
        break;
    }
    default:
        qDebug() << "Instruction class not defined";
        break;
    }
}

void Cpu::interrupt()
{
   qDebug() << "INT I1";
   return;
}

void Cpu::mov()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        SBUS = T;
        emit PdTS(true);
        emit ALU(true, true, false, "SBUS");

        RBUS = SBUS;
        emit PdALU(true);

        switch (mad) {
        case AD: {
            u8 index = IR & 0xF;
            R[index] = RBUS;
            emit PmRG(true, index, R[index]);

            decideNextPhase();
            break;
        }
        case AI: case AX:
            MDR = RBUS;
            emit PmMDR(true, MDR, true);
            break;
        }

        qDebug() << "EX MOV I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX MOV I2";
        break;
    }
    }
}

void Cpu::add()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        SBUS = T;
        emit PdTS(true);

        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = SBUS + DBUS;
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        switch (mad) {
        case AD: {
            u8 index = IR & 0xF;
            R[index] = RBUS;
            emit PmRG(true, index, R[index]);

            decideNextPhase();
            break;
        }
        case AI: case AX:
            MDR = RBUS;
            emit PmMDR(true, MDR, true);
            break;
        }

        setConditions(true);
        qDebug() << "EX ADD I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX ADD I2";
        break;
    }
    }
}

void Cpu::sub()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        SBUS = ~T;
        emit PdTS(true);

        RBUS = DBUS + SBUS + 1; //Cin
        emit ALU(true, true, true, "SUM+C");
        emit PdALU(true);

        switch (mad) {
        case AD: {
            u8 index = IR & 0xF;
            R[index] = RBUS;
            emit PmRG(true, index, R[index]);

            decideNextPhase();
            break;
        }
        case AI: case AX:
            MDR = RBUS;
            emit PmMDR(true, MDR, true);
            break;
        }

        setConditions(true);
        qDebug() << "EX SUB I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX SUB I2";
        break;
    }
    }
}

void Cpu::decideNextPhase()
{
    if(intr)
        cgb->setPhase(Phase::INT);
    else
        cgb->setPhase(Phase::IF);
}

void Cpu::setConditions(bool CarryOverflow, bool isADD)
{
    u8 Cout;
    if(isADD)
        Cout = CarryOverflow && (RBUS != (u32)SBUS + (u32)DBUS);
    else
        Cout = CarryOverflow && (RBUS != (u32)SBUS + (u32)DBUS + 1);

    if(Cout) {
        setC(true);
    }
    else
        setC(false);

    if(RBUS == 0)
        setZ(true);
    else
        setZ(false);

    if(RBUS >> 15)
        setS(true);
    else
        setS(false);

    u8 dcr;
    if(isADD)
        dcr = CarryOverflow &&  ((!((SBUS >> 15) ^ (DBUS >> 15))) & ((RBUS >> 15) ^ Cout));
    else
        dcr = CarryOverflow &&  (((SBUS >> 15) ^ (DBUS >> 15)) & ((RBUS >> 15) ^ Cout));

    if(dcr)
        setV(true);
    else
        setV(false);
}

void Cpu::setC(bool set)
{
    if(set) {
        FLAG |= 0b1000;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b0111;
        emit PmFLAG(true, FLAG);
    }
}

void Cpu::setZ(bool set)
{
    if(set) {
        FLAG |= 0b0100;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b1011;
        emit PmFLAG(true, FLAG);
    }
}

void Cpu::setS(bool set)
{
    if(set) {
        FLAG |= 0b0010;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b1101;
        emit PmFLAG(true, FLAG);
    }
}

void Cpu::setV(bool set)
{
    if(set) {
        FLAG |= 0b0001;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b1110;
        emit PmFLAG(true, FLAG);
    }
}

void Cpu::resetActivatedSignals()
{
    emit PdPCD(false);
    emit ALU(false, false, false);
    emit PdALU(false);
    emit PmADR(false, ADR);
    emit RD(false);
    emit PmIR(false, IR);
    emit PCchanged(false, PC);
    emit PmT(false, T);
    emit PdRGS(false);
    emit PmMDR(false, MDR);
    emit PdRGS(false);
    emit PdMDRS(false);
    emit PdMDRD(false);
    emit PdRGD(false);
    emit PdPCS(false);
    emit PmMem(memory);
    emit PdTS(false);
    emit PmRG(false);
    emit WR(false);
    emit PmFLAG(false, FLAG);
}

