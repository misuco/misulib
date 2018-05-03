#ifndef TOUCHHISTORYEVENT_H
#define TOUCHHISTORYEVENT_H

#include <QObject>

class TouchHistoryEvent : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int x MEMBER _x NOTIFY positionChanged)
    Q_PROPERTY(int y MEMBER _y NOTIFY positionChanged)
    Q_PROPERTY(float hue MEMBER _hue NOTIFY hueChanged)
    Q_PROPERTY(bool visible MEMBER _visible NOTIFY visibleChanged)
    Q_PROPERTY(float age MEMBER _age NOTIFY ageChanged)

public:
    explicit TouchHistoryEvent(int maxAge, QObject *parent = 0);

    void set(int x, int y, float hue, long t);
    void setTime(long t);

signals:
    void positionChanged();
    void visibleChanged();
    void ageChanged();
    void hueChanged();

private:
    int _x;
    int _y;
    float _hue;
    long _t;
    float _age;
    int _maxAge;
    bool _visible;

};

#endif // TOUCHHISTORYEVENT_H
