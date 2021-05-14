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

void CGB::setPhase(Phase phase)
{
    crtPhase = phase;
    crtImpulse = 1;
}

u8 CGB::getAndIncrementImpulse()
{
    return crtImpulse++;
}