#include "touchhistoryevent.h"

TouchHistoryEvent::TouchHistoryEvent(int maxAge, QObject *parent) : QObject(parent),
    _x(0),
    _y(0),
    _hue(0),
    _t(0),
    _maxAge(maxAge)
{}

void TouchHistoryEvent::set(int x, int y, float hue, long t)
{
    _x=x;
    _y=y;
    _hue=hue;
    _t=t;
    _age=0;
    _visible=true;
    emit hueChanged();
    emit positionChanged();
    emit ageChanged();
    emit visibleChanged();
}

void TouchHistoryEvent::setTime(long t)
{
    _age=(float)(t-_t)/(float)_maxAge;
    bool visible = _age>=1.0 ? false : true;
    emit ageChanged();
    if(_visible!=visible) {
        _visible=visible;
        emit visibleChanged();
    }
}
