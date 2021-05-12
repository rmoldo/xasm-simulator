#include "cgb.h"

CGB::CGB(QObject *parent) : QObject(parent)
{
    crtPhase = Phase::IF;
    crtImpulse = 1;
}

Phase CGB::getPhase()
{
    return crtPhase;
}

void CGB::nextPhase(Phase phase)
{
    crtPhase = phase;
    crtImpulse = 1;
}

u8 CGB::getImpulse()
{
    return crtImpulse++;
}
