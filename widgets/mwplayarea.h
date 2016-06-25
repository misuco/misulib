#ifndef MWPLAYAREA_H
#define MWPLAYAREA_H

#include <QPainter>
#include "misuwidget.h"
#include "wlayout.h"
#include "conf/types.h"
#include "comm/isender.h"

#define MAX_COLS 255
#define MAX_ROWS 3
#define EVENT_STACK_SIZE 64

class wlayout;

class MWPlayArea : public MisuWidget
{
    Q_OBJECT

public:
    struct eventStackElement {
        int eventId;
        int voiceId;
        int midinote;
        int row;
        int col;
        float f;
    };

    MWPlayArea(wlayout *parent);
    ~MWPlayArea();
    virtual void processTouchEvent(misuTouchEvent e);

    void setOut(ISender *value);

protected:
    void paintEvent(QPaintEvent *E);
    void resizeEvent(QResizeEvent *E);

public slots:
    void setBaseNote(int b);
    void setOctConf(int bottom,int mit,int top);
    void setBscale(int n, bool v);

private:
    // INFRASTRUCTURE
    // - parental control
    wlayout * misuco;
    // - painting
    QLinearGradient linearGrad;
    QString cap;
    // - networking
    ISender * out;

    // WORKING MEMORY
    // - touch field configuration
    MWPlayfield fields[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
    // - event stack/hashmap
    eventStackElement eventStack[EVENT_STACK_SIZE];
    int chan = 0;

    // CONFIGURATION
    // - scale
    MWScale Scale;
    // - bending
    bool bendHoriz;
    int bendVertTop;
    int bendVertBot;
    // - visual
    int colwidth[MAX_COLS];
    int rowheight[MAX_ROWS];

    // helper functions
    void config();
    void setColumn(int col, int midinote);
    void calcGeo();
    void paintField(int r, int c, int x, int y);
    void paintField(int r, int c);
};

#endif // MWPLAYAREA_H
