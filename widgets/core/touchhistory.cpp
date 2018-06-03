#include "touchhistory.h"
#include <QDateTime>
#include <QDebug>
#include "lib/misulib/models/touchhistoryevent.h"

TouchHistory::TouchHistory(QObject *parent) : QObject(parent),
    nextInsertAt(0)
{
    for(int i=0;i<100;i++) {
        _events.append(new TouchHistoryEvent(1500));
    }
    startTimer(30);
}

void TouchHistory::insert(int x, int y, float hue)
{
    TouchHistoryEvent * event = qobject_cast<TouchHistoryEvent *>(_events.at(nextInsertAt));
    if(event) {
        //qDebug() << "TouchHistory::insert " << x << " " << y << " " << hue;
        event->set(x,y,hue,QDateTime::currentMSecsSinceEpoch());
        nextInsertAt++;
        if(nextInsertAt>=_events.size()) nextInsertAt=0;
    }
}

void TouchHistory::timerEvent(QTimerEvent *)
{
    long now = QDateTime::currentMSecsSinceEpoch();
    for(auto e:_events) {
        TouchHistoryEvent * event = qobject_cast<TouchHistoryEvent *>(e);
        if(event) event->setTime(now);
    }
}
