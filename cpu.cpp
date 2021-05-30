#include "cpu.h"

#include <vector>
#include <climits>
#include <QDebug>

Cpu::Cpu(QObject *parent) : QObject(parent)
{
    memory = std::vector<u8>(1 << 16, 0);

    // Set RETI
    memory[1000] = 0x0c;
    memory[1001] = 0xc0;

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

    // Set RETI
    memory[1000] = 0x0c;
    memory[1001] = 0xc0;
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
        case 3:
            cmp();
            break;
        case 4:
            AND();
            break;
        case 5:
            OR();
            break;
        case 6:
            XOR();
            break;
        default:
            qDebug() << "Instruction not defined";
            break;
        }
        break;
    }
    case InstructionClass::b2: {
        switch ((IR >> 6) & 0xF) {
        case 0:
            clr();
            break;
        case 1:
            neg();
            break;
        case 2:
            inc();
            break;
        case 3:
            dec();
            break;
        case 4:
            asl();
            break;
        case 5:
            asr();
            break;
        case 6:
            lsr();
            break;
        case 7:
            rol();
            break;
        case 8:
            ror();
            break;
        case 9:
            rlc();
            break;
        case 10:
            rrc();
            break;
        case 11:
            jmp();
            break;
        case 12:
            call();
            break;
        case 13:
            pushRi();
            break;
        case 14:
            popRi();
            break;
        default:
            qDebug() << "Instruction not defined";
            break;
        }
        break;
    }
    case InstructionClass::b3: {
        switch ((IR >> 7) & 0xf) {
        case 0:
            br();
            break;
        case 1:
            bne();
            break;
        case 2:
            beq();
            break;
        case 3:
            bpl();
            break;
        case 4:
            bcs();
            break;
        case 5:
            bcc();
            break;
        case 6:
            bvs();
            break;
        case 7:
            bvc();
            break;
        default:
            qDebug() << "Instruction not defined";
        }
        break;
    }
    case InstructionClass::b4: {
        switch (IR & 0xff) {
        case 0:
            clc();
            break;
        case 1:
            clv();
            break;
        case 2:
            clz();
            break;
        case 3:
            cls();
            break;
        case 4:
            ccc();
            break;
        case 5:
            sec();
            break;
        case 6:
            sev();
            break;
        case 7:
            sez();
            break;
        case 8:
            ses();
            break;
        case 9:
            scc();
            break;
        case 10:
            nop();
            break;
        case 11:
            ret();
            break;
        case 12:
            reti();
            break;
        case 13:
            uhalt();
            break;
        case 14:
            wait();
            break;
        case 15:
            pushpc();
            break;
        case 16:
            poppc();
            break;
        case 17:
            pushflag();
            break;
        case 18:
            popflag();
            break;
        default:
            qDebug() << "Instruction not defined";
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
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SP -= 2;
        emit SPchanged(true, SP);

        qDebug() << "INT I1";

        break;
    case 2:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "INT I2";

        break;
    case 3:
        SBUS = FLAG;
        emit PdFLAGS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        MDR = RBUS;
        emit PmMDR(true, MDR);

        qDebug() << "INT I3";

        break;
    case 4:
        memory[ADR] = MDR & 0xff;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");

        SP -= 2;
        emit SPchanged(true, SP);

        qDebug() << "INT I4";

        break;
    case 5:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "INT I5";

        break;
    case 6:
        SBUS = PC;
        emit PdPCS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        MDR = RBUS;
        emit PmMDR(true, MDR);

        qDebug() << "INT I6";

        break;
    case 7:
        memory[ADR] = MDR & 0xff;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");

        qDebug() << "INT I7";

        break;
    case 8:
        SBUS = IVR;
        emit PdIVRS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        PC = RBUS;
        emit PmPC(true, ADR);

        qDebug() << "INT I8";

        // Unconditional set instruction fetch
        cgb->setPhase(Phase::IF);

        break;
    default:
        qDebug() << "ERROR INTERRUPT";
        break;
    }
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

        RBUS = (short)SBUS + (short)DBUS;
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

        setC(checkC(true));
        setZ();
        setS();
        setV(true);

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

        RBUS = (short)DBUS + (short)SBUS + 1; //Cin
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

        setC(checkC(false));
        setZ();
        setS();
        setV(false);
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

void Cpu::cmp()
{
    if(cgb->getAndIncrementImpulse() == 1) {
        DBUS = MDR;
        emit PdMDRD(true);

        SBUS = ~T;
        emit PdTS(true);

        RBUS = DBUS + SBUS + 1; //Cin
        emit ALU(true, true, true, "SUM+C");

        setC(checkC(false));
        setZ();
        setS();
        setV(false);
        decideNextPhase();
        qDebug() << "EX CMP I1";
    }
}

void Cpu::AND()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        SBUS = T;
        emit PdTS(true);

        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = SBUS & DBUS;
        emit ALU(true, true, true, "AND");
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

        setZ();
        setS();
        qDebug() << "EX AND I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX AND I2";
        break;
    }
    }
}

void Cpu::OR()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        SBUS = T;
        emit PdTS(true);

        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = SBUS | DBUS;
        emit ALU(true, true, true, "OR");
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

        setZ();
        setS();
        qDebug() << "EX OR I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX OR I2";
        break;
    }
    }
}

void Cpu::XOR()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        SBUS = T;
        emit PdTS(true);

        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = SBUS ^ DBUS;
        emit ALU(true, true, true, "XOR");
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

        setZ();
        setS();
        qDebug() << "EX XOR I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX XOR I2";
        break;
    }
    }
}

void Cpu::clr()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        SBUS = 0;
        //shall we emit something?

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
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

        setZ();
        setS();
        qDebug() << "EX CLR I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX CLR I2";
        break;
    }
    }

}

void Cpu::neg()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = ~DBUS;
        emit ALU(true, false, true, "!DBUS");
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

        setZ();
        setS();
        qDebug() << "EX NEG I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX NEG I2";
        break;
    }
    }
}

void Cpu::inc()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        SBUS = 0;
        //emit something?


        RBUS = SBUS + DBUS + 1; //Cin
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

        setC(checkC(true));
        setZ();
        setS();
        setV(true);

        qDebug() << "EX INC I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX INC I2";
        break;
    }
    }
}

void Cpu::dec()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        SBUS = (short) - 1;
        //emit something?


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

        setC(checkC(false));
        setZ();
        setS();
        setV(false);

        qDebug() << "EX DEC I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX DEC I2";
        break;
    }
    }
}

void Cpu::asl()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS << 1;
        emit ALU(true, false, true, "ST");
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

        setC(MDR >> 15);

        qDebug() << "EX ASL I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX ASL I2";
        break;
    }
    }
}

void Cpu::asr()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS >> 1;
        RBUS |= (DBUS & 0x8000);
        emit ALU(true, false, true, "DR");
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

        setC(MDR & 1);

        qDebug() << "EX ASR I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX ASR I2";
        break;
    }
    }
}

void Cpu::lsr()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS >> 1;
        emit ALU(true, false, true, "DR");
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

        setC(MDR & 1);

        qDebug() << "EX LSR I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX LSR I2";
        break;
    }
    }
}

void Cpu::rol()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS << 1;
        RBUS |= DBUS >> 15;
        emit ALU(true, false, true, "ST");
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

        setC(MDR >> 15);

        qDebug() << "EX ROL I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX ROL I2";
        break;
    }
    }
}

void Cpu::ror()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS >> 1;
        RBUS |= MDR << 15;
        emit ALU(true, false, true, "DR");
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

        setC(MDR & 1);

        qDebug() << "EX ROR I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX ROR I2";
        break;
    }
    }
}

void Cpu::rlc()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS << 1;
        RBUS |= (FLAG >> 3) & 0x1;
        emit ALU(true, false, true, "ST");
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

        setC(MDR >> 15);

        qDebug() << "EX RLC I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX RLC I2";
        break;
    }
    }
}

void Cpu::rrc()
{
    switch (cgb->getAndIncrementImpulse()) {
    case 1: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS >> 1;
        RBUS |= MDR << 15;
        RBUS |= (FLAG & 0x0008) << 12;
        emit ALU(true, false, true, "DR");
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

        setC(MDR & 1);

        qDebug() << "EX ROR I1";
        break;
    }
    case 2: {
        memory[ADR] = MDR & 0xFF;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();
        qDebug() << "EX ROR I2";
        break;
    }
    }
}

void Cpu::jmp()
{
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        PC = RBUS;
        emit PmPC(true, PC);

        decideNextPhase();
        qDebug() << "EX JMP I1";
        break;
    default:
        qDebug() << "ERROR EX JMP";
        break;
    }
}

void Cpu::call()
{
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        T = RBUS;
        emit PmT(true, T);

        qDebug() << "EX CALL I1";
        break;
    case 2:
        SP -= 2;
        emit SPchanged(true, SP);

        qDebug() << "EX CALL I2";
        break;
    case 3:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX CALL I3";
        break;
    case 4:
        SBUS = PC;
        emit PdPCS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        MDR = RBUS;
        emit PmMDR(true, MDR);

        qDebug() << "EX CALL I4";
        break;
    case 5:
        memory[ADR] = MDR & 0xff;
        memory[ADR + 1] = MDR >> 8;

        emit WR(true, "WRITE");
        emit PmMem(memory);

        qDebug() << "EX CALL I5";
        break;
    case 6:
        SBUS = T;
        emit PdTS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        PC = RBUS;
        emit PmPC(true, PC);

        decideNextPhase();

        qDebug() << "EX CALL I6";
        break;
    default:
        qDebug() << "ERROR EX CALL";
        break;
    }
}

void Cpu::pushRi()
{
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SP -= 2;
        emit SPchanged(true, SP);

        qDebug() << "EX PUSH I1";
        break;
    case 2:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX PUSH I2";
        break;
    case 3:
        SBUS = R[IR & 0xff];
        emit PdRGS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        MDR = RBUS;
        emit PmMDR(true, MDR);

        qDebug() << "EX PUSH I3";
        break;
    case 4:
        memory[ADR] = MDR & 0xff;
        memory[ADR + 1] = MDR >> 8;

        emit WR(true, "WRITE");
        emit PmMem(memory);

        decideNextPhase();

        qDebug() << "EX PUSH I4";
        break;
    default:
        qDebug() << "ERROR EX PUSH";
        break;
    }
}

void Cpu::popRi()
{
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX POP I1";
        break;
    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "EX POP I2";
        break;
    case 3: {
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        u8 index = IR & 0xff;
        R[index] = RBUS;
        emit PmRG(true, index, R[index]);

        SP += 2;
        emit SPchanged(true, SP);

        decideNextPhase();

        qDebug() << "EX POP I3";
        break;
    }
    default:
        qDebug() << "ERROR EX POP";
    }
}

void Cpu::br() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend if negative
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        PC = RBUS;

        emit PmPC(true, PC);

        decideNextPhase();

        qDebug() << "EX BR I1";
        break;
    default:
        qDebug() << "ERROR EX BR";
        break;
    }
}

void Cpu::bne() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend if negative
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        if (((FLAG >> 2) & 0x1) == 0) {
            PC = RBUS;
            emit PmPC(true, PC);
        }

        decideNextPhase();
        qDebug() << "EX BNE I1";

        break;
    default:
        qDebug() << "ERROR EX BNQ";
        break;
    }
}

void Cpu::beq() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend if negative
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        if (((FLAG >> 2) & 1) == 1) {
            PC = RBUS;
            emit PmPC(true, PC);
        }

        decideNextPhase();
        qDebug() << "EX BEQ I1";

        break;
    default:
        qDebug() << "ERROR EX BEQ";
        break;
    }
}

void Cpu::bpl() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend if negative
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        if (((FLAG >> 1) & 1) == 0) {
            PC = RBUS;
            emit PmPC(true, PC);
        }

        decideNextPhase();
        qDebug() << "EX BPL I1";

        break;
    default:
        qDebug() << "ERROR EX BPL";
        break;
    }
}

void Cpu::bcs() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        if (((FLAG >> 3) & 1) == 1) {
            PC = RBUS;
            emit PmPC(true, PC);
        }

        decideNextPhase();
        qDebug() << "EX BCS I1";

        break;
    default:
        qDebug() << "ERROR EX BCS";
        break;
    }
}

void Cpu::bcc() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        if (((FLAG >> 3) & 1) == 0) {
            PC = RBUS;
            emit PmPC(true, PC);
        }

        decideNextPhase();
        qDebug() << "EX BCC I1";

        break;
    default:
        qDebug() << "ERROR EX BCC";
        break;
    }
}

void Cpu::bvs() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend if negative
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        if ((FLAG & 1) == 1) {
            PC = RBUS;
            emit PmPC(true, PC);
        }

        decideNextPhase();
        qDebug() << "EX BVS I1";

        break;
    default:
        qDebug() << "ERROR EX BVS";
        break;
    }
}

void Cpu::bvc() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = IR & 0xff;
        emit PmSBUS(true);

        // sign extend
        if (SBUS & 0x80)
            SBUS |= 0xff00;

        DBUS = PC;
        emit PdPCD(true);

        RBUS = (u16)((short)SBUS + (short)DBUS);
        emit ALU(true, true, true, "SUM");
        emit PdALU(true);

        if ((FLAG & 1) == 0) {
            PC = RBUS;
            emit PmPC(true, PC);
        }

        decideNextPhase();
        qDebug() << "EX BVC I1";

        break;
    default:
        qDebug() << "ERROR EX BVC";
        break;
    }
}

void Cpu::clc() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Reset C bit in flag register
        FLAG &= 0xfff7;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX CLC I1";

        break;
    default:
        qDebug() << "ERROR EX CLC";
        break;
    }
}

void Cpu::clv() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Reset V bit in flag register
        FLAG &= 0xfffe;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX CLV I1";

        break;
    default:
        qDebug() << "ERROR EX CLV";
        break;
    }
}

void Cpu::clz() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Reset Z bit in flag register
        FLAG &= 0xfffb;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX CLZ I1";

        break;
    default:
        qDebug() << "ERROR EX CLZ";
        break;
    }
}

void Cpu::cls() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Reset S bit in flag register
        FLAG &= 0xfffd;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX CLS I1";

        break;
    default:
        qDebug() << "ERROR EX CLS";
        break;
    }
}

void Cpu::ccc() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Reset CZSV bit in flag register
        FLAG &= 0xfff0;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX CCC I1";

        break;
    default:
        qDebug() << "ERROR EX CCC";
        break;
    }
}

void Cpu::sec() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Set C bit in flag register
        FLAG |= 0x0008;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX SEC I1";

        break;
    default:
        qDebug() << "ERROR EX SEC";
        break;
    }
}

void Cpu::sev() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Set V bit in flag register
        FLAG |= 0x0001;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX SEV I1";

        break;
    default:
        qDebug() << "ERROR EX SEV";
        break;
    }
}

void Cpu::sez() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Get Z bit in flag register
        FLAG |= 0x0004;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX SEZ I1";

        break;
    default:
        qDebug() << "ERROR EX SEZ";
        break;
    }
}

void Cpu::ses() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Set S bit in flag register
        FLAG |= 0x0002;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX SES I1";

        break;
    default:
        qDebug() << "ERROR EX SES";
        break;
    }
}

void Cpu::scc() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        // Set CZSV bits in flag register
        FLAG |= 0x000f;
        emit PmFLAG(true, FLAG);

        decideNextPhase();
        qDebug() << "EX SCC I1";

        break;
    default:
        qDebug() << "ERROR EX SCC";
        break;
    }
}

void Cpu::nop() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        decideNextPhase();
        qDebug() << "EX NOP I1";

        break;
    default:
        qDebug() << "ERROR EX NOP";
        break;
    }
}

void Cpu::ret() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX RET I1";

        break;
    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "EX RET I2";

        break;
    case 3:
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        PC = RBUS;
        emit PmPC(true, PC);

        SP += 2;
        emit SPchanged(true, SP);

        decideNextPhase();

        qDebug() << "EX RET I3";

        break;
    default:
        qDebug() << "ERROR EX RET";
        break;
    }
}

void Cpu::reti() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        intr = false;

        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX RETI I1";

        break;
    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "EX RETI I2";

        break;
    case 3:
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        PC = RBUS;
        emit PmPC(true, PC);

        SP += 2;
        emit SPchanged(true, SP);

        qDebug() << "EX RETI I3";

        break;
    case 4:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX RETI I4";

        break;
    case 5:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "EX RETI I5";

        break;
    case 6:
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        FLAG = RBUS;
        emit PmFLAG(true, FLAG, true);

        SP += 2;
        emit SPchanged(true, SP);

        decideNextPhase();

        qDebug() << "EX RETI I6";

        break;
    default:
        qDebug() << "ERROR EX RETI";
    }
}

void Cpu::uhalt()
{
    halt = true;
    reason = "Halt encounted. Simulation finished!";
}

//this is not designed to function in run
void Cpu::wait()
{
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        if(intr)
            cgb->setPhase(Phase::INT);
        else
            cgb->setImpluse(cgb->getImpulse() - 1);

        qDebug() << "EX WAIT I1";

        break;
    default:
        qDebug() << "ERROR EX WAITP";
        break;
    }
}

void Cpu::pushpc() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SP -= 2;
        emit SPchanged(true, SP);

        qDebug() << "EX PUSHPC I1";

        break;
    case 2:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX PUSHPC I2";

        break;
    case 3:
        SBUS = PC;
        emit PdPCS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        MDR = RBUS;
        emit PmMDR(true, MDR);

        qDebug() << "EX PUSHPC I3";

        break;
    case 4:
        memory[ADR] = MDR & 0xff;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");

        decideNextPhase();

        qDebug() << "EX PUSHPC I4";

        break;
    default:
        qDebug() << "ERROR EX PUSHPC";
        break;
    }
}

void Cpu::poppc() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX POPPC I1";

        break;
    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "EX POPPC I2";

        break;
    case 3:
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        PC = RBUS;
        emit PmPC(true, PC);

        SP += 2;
        emit SPchanged(true, SP);

        decideNextPhase();

        break;
    default:
        qDebug() << "ERROR EX POPPC";
    }
}

void Cpu::pushflag() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SP -= 2;
        emit SPchanged(true, SP);

        qDebug() << "EX PUSHFLAG I1";

        break;
    case 2:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX PUSHFLAG I2";

        break;
    case 3:
        SBUS = FLAG;
        emit PdFLAGS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        MDR = RBUS;
        emit PmMDR(true, MDR);

        qDebug() << "EX PUSHFLAG I3";

        break;
    case 4:
        memory[ADR] = MDR & 0xff;
        memory[ADR + 1] = MDR >> 8;
        emit WR(true, "WRITE");

        decideNextPhase();

        qDebug() << "EX PUSHFLAG I4";

        break;
    default:
        qDebug() << "ERROR EX PUSHFLAG";
        break;
    }
}

void Cpu::popflag() {
    switch(cgb->getAndIncrementImpulse()) {
    case 1:
        SBUS = SP;
        emit PdSPS(true);

        RBUS = SBUS;
        emit ALU(true, true, false, "SBUS");
        emit PdALU(true);

        ADR = RBUS;
        emit PmADR(true, ADR);

        qDebug() << "EX POPFLAG I1";

        break;
    case 2:
        MDR = (memory[ADR + 1] << 8) | memory[ADR];
        emit RD(true, "READ");
        emit PmMDR(true, MDR);

        qDebug() << "EX POPFLAG I2";

        break;
    case 3:
        DBUS = MDR;
        emit PdMDRD(true);

        RBUS = DBUS;
        emit ALU(true, false, true, "DBUS");
        emit PdALU(true);

        FLAG = RBUS;
        emit PmFLAG(true, FLAG, true);

        SP += 2;
        emit SPchanged(true, SP);

        decideNextPhase();

        qDebug() << "EX POPFLAG I3";

        break;
    default:
        qDebug() << "ERROR EX POPPC";
    }
}

void Cpu::decideNextPhase()
{
    if(intr)
        cgb->setPhase(Phase::INT);
    else
        cgb->setPhase(Phase::IF);
}

void Cpu::setC(bool value)
{
    if(value) {
        FLAG |= 0b1000;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b0111;
        emit PmFLAG(true, FLAG);
    }
}

void Cpu::setZ()
{
    if(checkZ()) {
        FLAG |= 0b0100;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b1011;
        emit PmFLAG(true, FLAG);
    }
}

void Cpu::setS()
{
    if(checkS()) {
        FLAG |= 0b0010;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b1101;
        emit PmFLAG(true, FLAG);
    }
}

void Cpu::setV(bool isAdding)
{
    if(checkV(isAdding)) {
        FLAG |= 0b0001;
        emit PmFLAG(true, FLAG);
    }
    else {
        FLAG &= 0b1110;
        emit PmFLAG(true, FLAG);
    }
}

bool Cpu::checkC(bool isAdding)
{
    bool carry;
    u16 cin = (!isAdding);

    for (int i = 0; i < 16; ++i) {
        bool sum = (((DBUS ^ SBUS) & u16(1 << i)) >> i) ^ cin;
        u16 mask = 1 << i;

        if (i == 15 && !isAdding)
            return sum;

        carry = (((DBUS & SBUS) & mask) >> i) ||
                ((((DBUS ^ SBUS) & mask) >> i) && cin);

        cin = carry;
    }

    return carry;


    qDebug() << "CARRY: " << carry;
    return carry;
}

bool Cpu::checkZ()
{
    return RBUS == 0;
}

bool Cpu::checkS()
{
    return RBUS >> 15;
}

bool Cpu::checkV(bool isAdding)
{
    bool dcr;
    bool carry = checkC(isAdding);

    SBUS++; //convert to 2's complement

    if(isAdding)
        dcr = ((~(SBUS ^ DBUS)) >> 15) & ((RBUS >> 15) ^ carry);
    else
        dcr = ((SBUS >> 15) ^ (DBUS >> 15)) & ((RBUS >> 15) ^ carry);

    return dcr;
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
    emit SPchanged(false, SP);
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
    emit PmPC(false, PC);
    emit PdSPS(false);
    emit PdFLAGS(false);
    emit PdIVRS(false);
    emit loadIVR(false, IVR);
}

void Cpu::setInterrupt()
{
    intr = true;
    IVR = 1000;
    emit loadIVR(true, IVR);
}
