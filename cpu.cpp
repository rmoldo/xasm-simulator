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
    cil = false;
    iop = true;
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

#include<iostream>
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
        std::cout<<"IF I1" <<std::endl;
        break;

    case 2:
        IR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmIR(true, IR);
        PC += 2;
        emit PCchanged(true, PC);
        std::cout<<"IF I2" <<std::endl;
        break;

    case 3:
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

        std::cout<<"IF I3" <<std::endl;
        break;

    default:
        halt = true;
        reason = "Impulses out of range for Instruction Fetch phase";
        break;
    }

}

void Cpu::operandFetch()
{
    int mas = (IR >> 10) & 0x3;
    int mad = (IR >> 4) & 0x03;

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

            std::cout<<"SURSA OF I3" <<std::endl;
            break;
        case AX:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);

            DBUS = MDR;
            emit PdMDRS(true);

            emit ALU(true, true, true, "SUM");

            RBUS = SBUS + DBUS;
            emit PdALU(true);

            ADR = RBUS;
            emit PmADR(true, ADR);

            std::cout<< "SURSA OF I3" <<std::endl;
            break;
        default:
            std::cout << "Ceva eroare pula mea\n";
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
            emit PdRGS(true);
            emit ALU(true, true, false, "DBUS");

            RBUS = DBUS;
            emit PdALU(true);

            MDR = RBUS;
            emit PmMDR(true, T);

            cgb->setPhase(Phase::EX);
            break;
        case AI:
            DBUS = R[IR & 0xf];
            emit PdRGS(true);
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
        std::cout << "ERROR";
        break;
    }
}

void Cpu::execute()
{
    halt = true;
    std::cout<<"EX I1" <<std::endl;
    return;
}

void Cpu::interrupt()
{
    std::cout<<"INT I1" <<std::endl;
    return;
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
    emit PdRGD(false);
    emit PdPCS(false);
}

