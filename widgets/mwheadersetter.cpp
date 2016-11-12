#include "mwheadersetter.h"
#include <QPainter>

MWHeaderSetter::MWHeaderSetter(int headerId, QWidget *parent) : MisuWidget(parent)
{
    this->headerId=headerId;
    pressed=0;
    state=0;
}

void MWHeaderSetter::processTouchEvent(misuTouchEvent e)
{
    switch(e.state) {
    case Qt::TouchPointPressed:
        switch(headerId) {
        case 0:
        case 1:
        case 2:
            emit currentHeader(headerId);
            break;
        case 3:
            if(state==0) {
                state=1;
            } else {
                state=0;
            }
            emit setBendHori(state);
            break;
        case 4:
            if(state==0) {
                state=2;
            } else {
                state=0;
            }
            emit setBendVertTop(state);
            break;
        case 5:
            if(state==0) {
                state=-2;
            } else {
                state=0;
            }
            emit setBendVertBot(state);
            break;
        case 6:
            emit currentMainView(0);
            break;
        case 7:
            emit currentMainView(1);
            break;
        }

        pressed++;
        update();
        break;
    case Qt::TouchPointReleased:
        pressed--;
        update();
        break;
    }
}

void MWHeaderSetter::paintEvent(QPaintEvent *E)
{
    QPainter painter(this);
    QString cap;
    if(pressed>0) painter.setBrush(Qt::white);
    else painter.setBrush(Qt::gray);
    painter.drawRect(0,0,width(),height());
    cap.sprintf("%d",headerId);
    painter.drawText(0,0,width(),height(),Qt::AlignTop|Qt::AlignLeft,cap);
}

void MWHeaderSetter::resizeEvent(QResizeEvent *E)
{

}
