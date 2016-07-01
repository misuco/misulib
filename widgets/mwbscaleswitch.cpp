#include "mwbscaleswitch.h"
#include <QPainter>
#include "comm/senderdebug.h"

MWBScaleSwitch::MWBScaleSwitch(int Id)
{
    bscaleId=Id;
    value=false;
    pressed=0;
    chan=0;
    f=new FreqTriple();
    setBaseNote(0);
    out=new SenderDebug();
}

MWBScaleSwitch::~MWBScaleSwitch()
{
    delete(f);
}

void MWBScaleSwitch::processTouchEvent(misuTouchEvent e)
{
    switch(e.state) {
    case Qt::TouchPointPressed:
        vId=out->noteOn(chan,f->getFreq(),f->getMidinote(),f->getPitch(),127);
        value=!value;
        emit setBscale(bscaleId,value);
        pressed++;
        update();
        break;
    case Qt::TouchPointReleased:
        out->noteOff(vId);
        pressed--;
        update();
        break;
    }
}

void MWBScaleSwitch::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QString cap;
    int l=127;
    if(value) l=200;
    painter.setBrush(QColor::fromHsl(f->getHue(),127,l));
    painter.drawRect(0,0,width(),height());
    cap.sprintf("%d",f->getBasenote());
    painter.drawText(0,0,width(),height(),Qt::AlignCenter|Qt::AlignHCenter,cap);
}

void MWBScaleSwitch::setOut(ISender *value)
{
    out = value;
}

void MWBScaleSwitch::setBaseNote(int b)
{
    basenote=b;
    int newBaseNote=basenote+bscaleId+1%12;
    f->setBasenote(newBaseNote);
    update();
}

void MWBScaleSwitch::setOctMid(int o)
{
    f->setOct(o);
}