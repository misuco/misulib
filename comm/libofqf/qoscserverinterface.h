#ifndef QOSCSERVERINTERFACE_H
#define QOSCSERVERINTERFACE_H

#include "qosctypes.h"

class QOscServerInterface
{
public:
    virtual ~QOscServerInterface() {}

signals:
    virtual void oscData( QString, QList<QVariant>, QHostAddress, quint16 ) = 0;

};

#endif // QOSCSERVERINTERFACE_H
