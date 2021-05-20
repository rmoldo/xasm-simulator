#include "cpu.h"

#include <vector>

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

void Cpu::setMemory(u8 *data, size_t size) {
    memory.insert(memory.begin(), data, data + size);
}

#include<iostream>
void Cpu::instructionFetch()
{
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        DBUS = PC;
        emit PdPCD(true);
        emit ALU(true, "DBUS");
        RBUS = DBUS;
        emit PdALU(true);
        ADR = RBUS;
        emit PmADR(true, ADR);
        std::cout<<"IF I1" <<std::endl;
        break;

    case 2:
        IR = (memory[ADR] << 8) + memory[ADR + 1];
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

        if(iop) {
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
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        /*
            AM: PdIR[OP], SBUS, PdALU, PmM, TEX
            AD: PdRG, DBUS, PdALU, PmM, TEX
            AI: PdRG, DBUS, PdALU, PmADR
            AX: PdIR[IND], PdRG, SUM, PdALU, PmADR
        */
        std::cout<<"OF I1" <<std::endl;
        break;

    case 2:
        /* RD, PdMEM, PmM */
        cgb->setPhase(Phase::EX);
        std::cout<<"OF I2" <<std::endl;
        break;

    default:
        halt = true;
        reason = "Impulses out of range for Operand Fetch phase";
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
    emit ALU(false);
    emit PdALU(false);
    emit PmADR(false, ADR);
    emit RD(false);
    emit PmIR(false, IR);
    emit PCchanged(false, PC);
}

