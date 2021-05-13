#ifndef CGB_H
#define CGB_H

#include <QObject>

#include "assembler/defs.h"

enum class Phase { IF = 1, OF = 2, EX = 3, INT = 4};

class CGB : public QObject
{
    Q_OBJECT
public:
    explicit CGB(QObject *parent = nullptr);

    Phase getPhase();
    void setPhase(Phase phase);
    u8 getAndIncrementImpulse();

signals:

private:
    Phase crtPhase;
    u8 crtImpulse;
};

#endif // CGB_H
