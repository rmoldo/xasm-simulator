#include "cpu.h"

#include <vector>
#include <climits>
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
            //lsr();
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

        SBUS = ~T + 1; //Cin
        emit PdTS(true);

        RBUS = (short)DBUS + (short)SBUS;
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

//Think this doesn't work
bool Cpu::checkC(bool isAdding)
{
    bool carry;

    if(isAdding)
        carry = (((short)SBUS > 0) && ((short)DBUS > SHRT_MAX - (short)SBUS));
    else
        carry = (((short)SBUS < 0) && ((short)DBUS > SHRT_MIN + (short)SBUS));

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
}

