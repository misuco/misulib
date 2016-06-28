#include "mwplayarea.h"
#include <QDebug>
#include <QLinearGradient>
#include <QRadialGradient>
#include "comm/senderdebug.h"
#include "comm/sendermobilesynth.h"

MWPlayArea::MWPlayArea(wlayout *parent) : MisuWidget(parent),
    linearGrad(QPointF(0,1),QPointF(0,1))
{
    misuco=parent;

    Scale.basenote=1;
    Scale.baseoct=3;
    Scale.topoct=4;
    for(int i=0;i<BSCALE_SIZE;i++) {
        Scale.bscale[i]=false;
    }

    bendHoriz=true;
    bendVertTop=2;
    bendVertBot=-2;

    for(int i=0;i<EVENT_STACK_SIZE;i++) {
        eventStack[i].eventId=-1;
        eventStack[i].row=-1;
        eventStack[i].col=-1;
        eventStack[i].f=0;
    }

    out = new SenderDebug();

    config();
}

MWPlayArea::~MWPlayArea()
{

}

void MWPlayArea::config()
{
    /*
     *  |.|.||.|.|.| |
     */
    cols=0;
    for(int oct=Scale.baseoct;oct<Scale.topoct;oct++) {
        setColumn(cols,Scale.basenote+oct*12);
        if(bendHoriz) {
            cols+=2;
        } else {
            cols++;
        }
        for(int note=0;note<BSCALE_SIZE;note++) {
            if(Scale.bscale[note]) {
                setColumn(cols,Scale.basenote+oct*12+note+1);
                if(bendHoriz) {
                    cols+=2;
                } else {
                    cols++;
                }
            }
        }
    }
    int topnote=Scale.basenote+(Scale.topoct)*12;
    //qDebug() << "basenote: " << Scale.basenote << "topnote: " << topnote;
    setColumn(cols,topnote);
    cols++;
    calcGeo();
    update();
}

void MWPlayArea::setColumn(int col, int midinote) {
    //qDebug() << "setColumn " << col << " " << midinote;
    rows=0;
    if(bendVertTop!=0) {
        fields[rows][col].type=BEND_VERT;
        fields[rows][col].f1=misuco->getNote(midinote);
        fields[rows][col].hue1bent=fields[rows][col].f1->getHue()+HUE_NOTES*bendVertTop;
        if(fields[rows][col].hue1bent>359) fields[rows][col].hue1bent-=359;
        if(fields[rows][col].hue1bent<0) fields[rows][col].hue1bent+=359;
        fields[rows][col].pressed=0;
        if(col>1 && bendHoriz) {
            fields[rows][col-1].type=BEND_VERT_HORIZ;
            fields[rows][col-1].f1=fields[rows][col-2].f1;
            fields[rows][col-1].hue1bent=fields[rows][col-1].f1->getHue()+HUE_NOTES*bendVertTop;
            if(fields[rows][col-1].hue1bent>359) fields[rows][col-1].hue1bent-=359;
            if(fields[rows][col-1].hue1bent<0) fields[rows][col-1].hue1bent+=359;
            fields[rows][col-1].f2=misuco->getNote(midinote);
            fields[rows][col-1].hue2bent=fields[rows][col-1].f2->getHue()+HUE_NOTES*bendVertTop;
            if(fields[rows][col-1].hue2bent>359) fields[rows][col-1].hue2bent-=359;
            if(fields[rows][col-1].hue2bent<0) fields[rows][col-1].hue2bent+=359;
            fields[rows][col-1].pressed=0;
        }
        rows++;
    }
    fields[rows][col].type=NORMAL;
    fields[rows][col].f1=misuco->getNote(midinote);
    qDebug() << "set f1 " << midinote << " " << fields[rows][col].f1;
    fields[rows][col].pressed=0;
    if(col>1 && bendHoriz) {
        fields[rows][col-1].type=BEND_HORIZ;
        fields[rows][col-1].f1=fields[rows][col-2].f1;
        fields[rows][col-1].f2=misuco->getNote(midinote);
        fields[rows][col-1].pressed=0;
    }
    rows++;
    if(bendVertBot!=0) {
        fields[rows][col].type=BEND_VERT;
        fields[rows][col].f1=misuco->getNote(midinote);
        fields[rows][col].hue1bent=fields[rows][col].f1->getHue()+HUE_NOTES*bendVertBot;
        if(fields[rows][col].hue1bent>359) fields[rows][col].hue1bent-=359;
        if(fields[rows][col].hue1bent<0) fields[rows][col].hue1bent+=359;
        fields[rows][col].pressed=0;
        if(col>1 && bendHoriz) {
            fields[rows][col-1].type=BEND_VERT_HORIZ;
            fields[rows][col-1].f1=fields[rows][col-2].f1;
            fields[rows][col-1].hue1bent=fields[rows][col-1].f1->getHue()+HUE_NOTES*bendVertBot;
            if(fields[rows][col-1].hue1bent>359) fields[rows][col-1].hue1bent-=359;
            if(fields[rows][col-1].hue1bent<0) fields[rows][col-1].hue1bent+=359;
            fields[rows][col-1].f2=misuco->getNote(midinote);
            fields[rows][col-1].hue2bent=fields[rows][col-1].f2->getHue()+HUE_NOTES*bendVertBot;
            if(fields[rows][col-1].hue2bent>359) fields[rows][col-1].hue2bent-=359;
            if(fields[rows][col-1].hue2bent<0) fields[rows][col-1].hue2bent+=359;
            fields[rows][col-1].pressed=0;
        }
        rows++;
    }
}

void MWPlayArea::calcGeo()
{
    int cw=width()/cols;
    int rh=height()/rows;
    for(int i=0;i<cols;i++) {
        colwidth[i]=cw;
    }
    for(int i=0;i<rows;i++) {
        rowheight[i]=rh;
    }
}

void MWPlayArea::paintField(int r, int c, int x, int y) {
    QPainter painter(this);
    //qDebug() << "MWPlayArea::paintField r " << r << " c " << c;
    int l=127;
    if(fields[r][c].pressed>0) l=180;
    switch(fields[r][c].type) {
    case NORMAL:
        painter.setPen(Qt::black);
        painter.setBrush(QColor::fromHsl(fields[r][c].f1->getHue(),180,l));
        //qDebug() << "setBrush hue " << fields[r][c].f1->getHue();
        painter.drawRect(x,y,colwidth[c],rowheight[r]);
        cap.sprintf("%d\n%5.2f",fields[r][c].f1->getMidinote(),fields[r][c].f1->getFreq());
        painter.drawText(x,y,colwidth[c],rowheight[r],Qt::AlignCenter,cap);
        break;

    case BEND_VERT:
        linearGrad.setStart(QPointF(x,y));
        linearGrad.setFinalStop(QPointF(x, y+rowheight[r]));
        if(0==r) {
            linearGrad.setColorAt(0, QColor::fromHsl(fields[r][c].hue1bent,180,l));
            linearGrad.setColorAt(1, QColor::fromHsl(fields[r][c].f1->getHue(),180,l));
        } else {
            linearGrad.setColorAt(0, QColor::fromHsl(fields[r][c].f1->getHue(),180,l));
            linearGrad.setColorAt(1, QColor::fromHsl(fields[r][c].hue1bent,180,l));
        }
        painter.setPen(Qt::black);
        painter.setBrush(linearGrad);
        painter.drawRect(x,y,colwidth[c],rowheight[r]);

        if(0==r) {
            cap.sprintf("%d",fields[r][c].f1->getMidinote());
            painter.drawText(x,y,colwidth[c],rowheight[r],Qt::AlignHCenter|Qt::AlignBottom,cap);
            cap.sprintf("%d",fields[r][c].f1->getMidinote()+bendVertTop);
            painter.drawText(x,y,colwidth[c],rowheight[r],Qt::AlignHCenter|Qt::AlignTop,cap);
        } else {
            cap.sprintf("%d",fields[r][c].f1->getMidinote());
            painter.drawText(x,y,colwidth[c],rowheight[r],Qt::AlignHCenter|Qt::AlignTop,cap);
            cap.sprintf("%d",fields[r][c].f1->getMidinote()+bendVertBot);
            painter.drawText(x,y,colwidth[c],rowheight[r],Qt::AlignHCenter|Qt::AlignBottom,cap);

        }
        break;

    case BEND_HORIZ:
        linearGrad.setStart(QPointF(x,y));
        linearGrad.setFinalStop(QPointF(QPointF(x+colwidth[c], y)));
        linearGrad.setColorAt(0, QColor::fromHsl(fields[r][c].f1->getHue(),180,l));
        linearGrad.setColorAt(1, QColor::fromHsl(fields[r][c].f2->getHue(),180,l));
        painter.setPen(Qt::black);
        painter.setBrush(linearGrad);
        painter.drawRect(x,y,colwidth[c],rowheight[r]);
        cap.sprintf("%d",fields[r][c].f1->getMidinote());
        painter.drawText(x,y,colwidth[c],rowheight[r],Qt::AlignLeft,cap);
        cap.sprintf("%d",fields[r][c].f2->getMidinote());
        painter.drawText(x,y,colwidth[c],rowheight[r],Qt::AlignRight|Qt::AlignBottom,cap);
        break;

    case BEND_VERT_HORIZ:
        linearGrad.setStart(QPointF(x,y));
        linearGrad.setFinalStop(QPointF(QPointF(x+colwidth[c], y)));
        float hue1=fields[r][c].f1->getHue();
        float hue2=fields[r][c].f2->getHue();
        float inchue;
        painter.setPen(Qt::NoPen);
        if(0==r) {
            inchue=(float)(bendVertTop*HUE_NOTES)/(float)rowheight[r];
            //qDebug() << "bendVertTop inchue " << inchue;
            for(int y1=y+rowheight[r];y1>=y;y1--) {
                //qDebug() << "hue 1 " << hue1 << " hue2 " << hue2;
                linearGrad.setColorAt(0, QColor::fromHsl(hue1,180,l));
                linearGrad.setColorAt(1, QColor::fromHsl(hue2,180,l));
                painter.setBrush(linearGrad);
                painter.drawRect(x,y1,colwidth[c],1);
                hue1+=inchue;
                if(hue1>359) hue1-=359;
                if(hue1<0) hue1+=359;
                hue2+=inchue;
                if(hue2>359) hue2-=359;
                if(hue2<0) hue2+=359;
            }
        } else {
            inchue=(float)(bendVertBot*HUE_NOTES)/(float)rowheight[r];
            //qDebug() << "bendVertBot inchue " << inchue;
            for(int y1=y;y1<=y+rowheight[r];y1++) {
                //qDebug() << "hue 1 " << hue1 << " hue2 " << hue2;
                linearGrad.setColorAt(0, QColor::fromHsl(hue1,180,l));
                linearGrad.setColorAt(1, QColor::fromHsl(hue2,180,l));
                painter.setBrush(linearGrad);
                painter.drawRect(x,y1,colwidth[c],1);
                hue1+=inchue;
                if(hue1>359) hue1-=359;
                if(hue1<0) hue1+=359;
                hue2+=inchue;
                if(hue2>359) hue2-=359;
                if(hue2<0) hue2+=359;
            }
        }

        painter.setPen(Qt::black);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(x,y,colwidth[c],rowheight[r]);
        break;
    }
}

void MWPlayArea::paintField(int r, int c)
{
    int x=r*colwidth[0];
    int y=c*rowheight[0];
    paintField(r,c,x,y);
}

void MWPlayArea::paintEvent(QPaintEvent *E)
{
    int x,y=0;
    for(int r=0;r<rows;r++) {
        x=0;
        for(int c=0;c<cols;c++) {
            paintField(r,c,x,y);
            x+=colwidth[c];
        }
        y+=rowheight[r];
    }
}

void MWPlayArea::resizeEvent(QResizeEvent *E)
{
    calcGeo();
}

void MWPlayArea::processTouchEvent(misuTouchEvent e)
{
    //qDebug() << "MWPlayArea::processPoint " << e.id << " x " << e.x << " y " << e.y << " t " << e.t;
    FreqTriple fcalc;

    int eventHash=e.id%64;
    eventStackElement * es = &eventStack[eventHash];

    int row=e.y*rows/height();
    int col=e.x*cols/width();

    float yrel=(float)(e.y-row*rowheight[row])/(float)rowheight[row];
    float xrel=(float)(e.x-col*colwidth[col])/(float)colwidth[col];

    MWPlayfield * pf = &fields[row][col];
    //qDebug() << "row " << row << " col " << col << " eventHash " << eventHash;

    float freq;
    int midinote;
    int pitch;
    int velocity=127;

    float freqdiff;
    int pitchdiff;

    switch(pf->type) {
    case BEND_HORIZ:
        freqdiff=pf->f2->getFreq()-pf->f1->getFreq();
        freqdiff*=xrel;
        freq=pf->f1->getFreq()+freqdiff;
        pitchdiff=pf->f2->getMidinote()*8192-pf->f1->getMidinote()*8192;
        pitchdiff+=pf->f2->getPitch();
        pitchdiff-=pf->f1->getMidinote();
        pitchdiff*=xrel;
        pitchdiff+=pf->f1->getMidinote()*8192;
        midinote=round(pitchdiff/8192);
        pitch=pitchdiff-midinote*8192;
        break;

    case BEND_VERT_HORIZ:
        pitchdiff=pf->f2->getMidinote()*8192-pf->f1->getMidinote()*8192;
        pitchdiff+=pf->f2->getPitch();
        pitchdiff-=pf->f1->getMidinote();
        pitchdiff*=xrel;
        pitchdiff+=pf->f1->getMidinote()*8192;
        if(0==row) {
            pitchdiff+=bendVertTop*8192*(1-yrel);
        } else {
            pitchdiff+=bendVertBot*8192*yrel;
        }
        midinote=round(pitchdiff/8192);
        pitch=pitchdiff-midinote*8192;
        fcalc.setMidinote(midinote,pitch);
        freq=fcalc.getFreq();
        break;

    case BEND_VERT:
        if(0==row) {
            pitchdiff=bendVertTop*8192;
            yrel=1-yrel;
        } else {
            pitchdiff=bendVertBot*8192;
        }

        pitchdiff*=yrel;
        pitchdiff+=pf->f1->getMidinote()*8192;
        midinote=round(pitchdiff/8192);
        pitch=pitchdiff-midinote*8192;
        fcalc.setMidinote(midinote,pitch);
        freq=fcalc.getFreq();

        qDebug() << "midinote " << midinote << " pitch " << pitch << " freq " << freq ;

        break;

    default:
        freq=pf->f1->getFreq();
        midinote=pf->f1->getMidinote();
        pitch=pf->f1->getPitch();
        break;
    }

    switch(e.state) {
    case Qt::TouchPointPressed:
        es->eventId=e.id;
        es->midinote=midinote;
        es->row=row;
        es->col=col;
        es->f=freq;
        pf->pressed++;
        es->voiceId=out->noteOn(chan,freq,midinote,pitch,velocity);
        //paintField(row,col);
        update();
        break;
    case Qt::TouchPointMoved:
        if(row!=es->row || col!=es->col) {
            MWPlayfield * ppf = &fields[es->row][es->col];
            ppf->pressed--;
            out->noteOff(es->voiceId);

            es->midinote=midinote;
            es->voiceId=out->noteOn(chan,freq,midinote,pitch,velocity);

            es->row=row;
            es->col=col;
            es->f=freq;
            pf->pressed++;
            //paintField(row,col);
            update();
        } else if(freq!=es->f) {
            out->pitch(chan,es->voiceId,freq,midinote,pitch);
            qDebug() << "pitch " << freq;
            es->f=freq;
        }
        break;
    case Qt::TouchPointReleased:
        out->noteOff(es->voiceId);
        es->eventId=-1;
        es->row=-1;
        es->col=-1;
        pf->pressed--;
        //paintField(row,col);
        update();
        break;
    }
}

void MWPlayArea::setBaseNote(int b)
{
    if(b>=0 && b<128) {
        Scale.basenote=b;
    }
    config();
}


void MWPlayArea::setOctConf(int bottom, int mit, int top)
{
    Scale.baseoct=bottom;
    Scale.topoct=top;
    config();

}

void MWPlayArea::setBscale(int n, bool v)
{
    Scale.bscale[n]=v;
    config();
}

void MWPlayArea::setOut(ISender *value)
{
    out = value;
    qDebug() << "MWPlayArea::setOut:" << out;
}
