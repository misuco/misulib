#ifndef TOUCHHISTORY_H
#define TOUCHHISTORY_H

#include <QObject>
#include "lib/misulib/models/types.h"

class TouchHistory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<QObject *> events MEMBER _events CONSTANT)

public:
    explicit TouchHistory(QObject *parent = 0);
    void insert(int x, int y, float hue);

protected:
    void timerEvent(QTimerEvent *);

private:
    QList<QObject *> _events;
    int nextInsertAt;
};

#endif // TOUCHHISTORY_H
