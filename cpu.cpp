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
        emit ALU(true, "DBUS");
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
    int mas = (IR >> 10) & 0x3;
    int mad = (IR >> 4) & 0x03;

    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        if ((IR & 0x8000) != 0) {
            cgb->setImpluse(4);
            return;
        }

        switch (mas) {
        case AM: case AX:
            SBUS = PC;
            emit ALU(true, "SBUS");

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
            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            break;
        case AI:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);
            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            ADR = RBUS;
            PmADR(true, ADR);

            break;
        default:
            break;
        }

        std::cout<<"SURSA OF I1" <<std::endl;
        break;

    case 2:
        if (mas == AD)
            break;

        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        std::cout<<"SURSA OF I2" <<std::endl;
        break;

    case 3:
        switch(mas) {
        case AM: case AI:
            SBUS = MDR;
            PdMDRS(true);

            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            std::cout<<"SURSA OF I3" <<std::endl;
            break;
        case AX:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);

            DBUS = MDR;
            emit PdMDRS(true);

            emit ALU(true, "SBUS");

            RBUS = SBUS + DBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            std::cout<<"SURSA OF I3" <<std::endl;
            break;
        default:
            std::cout << "Ceva eroare pula mea\n";
            break;
        }

        break;
    case 4:
        switch (mad) {
        case AM: case AX:
            DBUS = PC;
            emit ALU(true, "DBUS");

            PC += 2;
            emit PCchanged(true, PC);

            RBUS = DBUS;
            emit PdALU(true);

            MDR = RBUS;
            emit PmMDR(true, ADR);

            break;
        case AD:
            SBUS = R[IR & 0xf];
            emit PdRGS(true);
            emit ALU(true, "DBUS");

            RBUS = DBUS;
            emit PdALU(true);

            MDR = RBUS;
            emit PmMDR(true, T);

            cgb->setPhase(Phase::EX);
            break;
        case AI:
            DBUS = R[IR & 0xf];
            emit PdRGS(true);
            emit ALU(true, "DBUS");

            RBUS = DBUS;
            emit PdALU(true);

            ADR = RBUS;
            emit PmADR(true, ADR);

            break;
        default:
            break;
        }

        std::cout<<"DESTINATIE OF I1" <<std::endl;
        break;
    case 5:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        if (mad == AM || mad == AI)
            cgb->setPhase(Phase::EX);

        std::cout << "DESTINATIE OF I2";
        break;
    case 6:
        DBUS = R[IR & 0xf];
        emit PdRGD(true);

        DBUS = MDR;
        // PdMDRD

        emit ALU(true, "SBUS");

        RBUS = SBUS + DBUS;
        emit PdALU(true);

        MDR = RBUS;
        emit PmMDR(true, T);

        cgb->setPhase(Phase::EX);

        std::cout << "DESTINATIE OF I3";
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

void Cpu::fetchSourceOperand()
{
    int mas = (IR >> 10) & 0x3;

    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        switch (mas) {
        case AM: case AX:
            SBUS = PC;
            emit ALU(true, "SBUS");

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
            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            return;
            break;
        case AI:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);
            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            ADR = RBUS;

            break;
        default:
            break;
        }

        std::cout<<"SURSA OF I1" <<std::endl;
        break;

    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        std::cout<<"SURSA OF I2" <<std::endl;
        break;

    case 3:
        switch(mas) {
        case AM: case AI:
            SBUS = MDR;
            // PdMDRS

            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            std::cout<<"SURSA OF I3" <<std::endl;
            break;
        case AX:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);

            DBUS = MDR;
            // PdMDRD

            emit ALU(true, "SBUS");

            RBUS = SBUS + DBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            std::cout<<"SURSA OF I3" <<std::endl;
            break;
        default:
            std::cout << "Ceva eroare pula mea\n";
            break;
        }

        std::cout<<"SURSA OF I2" <<std::endl;
        break;

    default:
        halt = true;
        reason = "Impulses out of range for Operand Fetch phase";
        break;
    }
}

void Cpu::fetchDestinationOperand()
{
    int mad = (IR >> 4) & 0x3;

    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        switch (mad) {
        case AM: case AX:
            DBUS = PC;
            emit ALU(true, "DBUS");

            PC += 2;
            emit PCchanged(true, PC);

            RBUS = DBUS;
            emit PdALU(true);

            ADR = RBUS;
            emit PmADR(true, ADR);

            break;
        case AD:
            SBUS = R[IR & 0xf];
            emit PdRGS(true);
            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            return;
            break;
        case AI:
            SBUS = R[IR & 0xf];
            emit PdRGS(true);
            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            ADR = RBUS;

            break;
        default:
            break;
        }

        std::cout<<"SURSA OF I1" <<std::endl;
        break;

    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        std::cout<<"SURSA OF I2" <<std::endl;
        break;

    case 3:
        switch(mad) {
        case AM: case AI:
            SBUS = MDR;
            // PdMDRS

            emit ALU(true, "SBUS");

            RBUS = SBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            std::cout<<"SURSA OF I3" <<std::endl;
            break;
        case AX:
            SBUS = R[(IR >> 6) & 0xf];
            emit PdRGS(true);

            DBUS = MDR;
            // PdMDRD

            emit ALU(true, "SBUS");

            RBUS = SBUS + DBUS;
            emit PdALU(true);

            T = RBUS;
            emit PmT(true, T);

            std::cout<<"SURSA OF I3" <<std::endl;
            break;
        default:
            std::cout << "Ceva eroare pula mea\n";
            break;
        }

        std::cout<<"SURSA OF I2" <<std::endl;
        break;

    default:
        halt = true;
        reason = "Impulses out of range for Operand Fetch phase";
        break;
    }
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
    emit PmT(false, T);
    emit PdRGS(false);
    emit PmMDR(false, MDR);
    emit PdRGS(false);
    emit PdMDRS(false);
    emit PdRGD(false);
}

