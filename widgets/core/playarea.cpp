/*
 * Copyright (c) 2018 by misuco.org, Claudio Zopfi, Zurich, Switzerland, c@misuco.org
 *
 * This file is part of MISUCO2.
 *
 * MISUCO2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MISUCO2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MISUCO2.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "playarea.h"
#include <QDebug>
#include <QLinearGradient>
#include <QTouchEvent>
#include <QDateTime>
#include "lib/misulib/comm/mastersender.h"
#include <math.h>

PlayArea::PlayArea(MasterSender * mastersender, TouchHistory * touchhistory, QObject *parent) : QObject(parent),
    _touchHistory(touchhistory),
    _out(mastersender),
    _fcalc(0,this),
    _sendCc1(false),
    _cc1Sum(0),
    _activeVoices(0)
{
    bendHoriz=false;
    bendVertTop=0;
    bendVertBot=0;

    _baseOct = 6;
    _topOct = 7;
    _rootNote = 0;
    for(int i=0;i<SCALE_SIZE;i++) {
        _scale.append(false);
    }

    for(int i=0;i<EVENT_STACK_SIZE;i++) {
        eventStack[i].active=0;
    }

    for(int r=0;r<MAX_ROWS;r++) {
        for(int c=0;c<MAX_COLS;c++) {
            _fields[r][c].setF1rootNote(c%(SCALE_SIZE+1));
            _fields[r][c].setF2rootNote(c%(SCALE_SIZE+1));
            _fields[r][c].setPressed(0);
        }
    }

    config();
}

PlayArea::~PlayArea()
{
}

void PlayArea::config()
{
    /*
     *  |.|.||.|.|.| |
     */

    int currentCols=cols;
    int currentRows=rows;

    cols=0;
    for(int oct=_baseOct;oct<_topOct;oct++) {
        setColumn(cols,_rootNote+oct*12,_rootNote);
        if(bendHoriz) {
            cols+=2;
        } else {
            cols++;
        }
        for(int note=0;note<SCALE_SIZE;note++) {
            //qDebug() << "MWPlayArea::config " << note;
            if(_scale[note]) {
                setColumn(cols,_rootNote+oct*12+note+1,(_rootNote+note+1)%(SCALE_SIZE+1));
                //qDebug() << "set column ";
                if(bendHoriz) {
                    cols+=2;
                } else {
                    cols++;
                }
            }
        }
    }
    int topnote=_rootNote+(_topOct)*12;
    //qDebug() << "rootNote: " << Scale.rootNote << "topnote: " << topnote;
    setColumn(cols,topnote,_rootNote);
    cols++;

    if(cols!=currentCols || rows!=currentRows) {
        calcGeo();
        emit playRowsChanged();
    }

    // update touch points by "moving" them
    for(int i=0;i<EVENT_STACK_SIZE;i++) {
        EventStackElement * es = &eventStack[i];
        if(es->active>0) {
            misuTouchEvent e;
            e.state=Qt::TouchPointMoved;
            e.touchId=es->touchId;
            e.x=es->x;
            e.y=es->y;
            processTouchEvent(e);
        }
    }
}

void PlayArea::setColumn(int col, int midinote, int pitch) {
    //qDebug() << "setColumn " << col << " midinote " << midinote << " rootNote " << rootNote;
    rows=0;

    float huePerNote = 1.0f/12.0f;
    if(bendVertTop!=0) {
        _fields[rows][col].setType(BEND_VERT);
        _fields[rows][col].setF1midiNote(midinote,pitch);

        float hue = _fields[rows][col].getF1Hue()+huePerNote*(float)bendVertTop;
        _fields[rows][col].setHue1Bent(hue);
        //_fields[rows][col].setPressed(0);

        if(col>1 && bendHoriz) {
            _fields[rows][col-1].setType(BEND_VERT_HORIZ);
            _fields[rows][col-1].setF1midiNote(_fields[rows][col-2].getF1midiNote(),_fields[rows][col-2].getF1pitch());

            float hue = _fields[rows][col-1].getF1Hue()+huePerNote*(float)bendVertTop;
            _fields[rows][col-1].setHue1Bent(hue);

            _fields[rows][col-1].setF2midiNote(midinote,pitch);

            hue = _fields[rows][col-1].getF2Hue()+huePerNote*(float)bendVertTop;
            _fields[rows][col-1].setHue2Bent(hue);
            //_fields[rows][col-1].setPressed(0);
        }
        rows++;
    }
    _fields[rows][col].setType(NORMAL);
    _fields[rows][col].setF1midiNote(midinote,pitch);
    //_fields[rows][col].setPressed(0);
    if(col>1 && bendHoriz) {
        _fields[rows][col-1].setType(BEND_HORIZ);
        _fields[rows][col-1].setF1midiNote(_fields[rows][col-2].getF1midiNote(),_fields[rows][col-2].getF1pitch());
        _fields[rows][col-1].setF2midiNote(midinote,pitch);
        //_fields[rows][col-1].setPressed(0);
    }
    rows++;
    if(bendVertBot!=0) {
        _fields[rows][col].setType(BEND_VERT);
        _fields[rows][col].setF1midiNote(midinote,pitch);

        float hue = _fields[rows][col].getF1Hue()+huePerNote*(float)bendVertBot;
        _fields[rows][col].setHue1Bent(hue);
        //_fields[rows][col].setPressed(0);

        if(col>1 && bendHoriz) {
            _fields[rows][col-1].setType(BEND_VERT_HORIZ);
            _fields[rows][col-1].setF1midiNote(_fields[rows][col-2].getF1midiNote(),_fields[rows][col-2].getF1pitch());

            float hue = _fields[rows][col-1].getF1Hue()+huePerNote*(float)bendVertBot;
            _fields[rows][col-1].setHue1Bent(hue);

            _fields[rows][col-1].setF2midiNote(midinote,pitch);

            hue = _fields[rows][col-1].getF2Hue()+huePerNote*(float)bendVertBot;
            _fields[rows][col-1].setHue2Bent(hue);
            //_fields[rows][col-1].setPressed(0);
        }
        rows++;
    }
}

void PlayArea::calcGeo()
{
    _playFieldWidth=_playAreaWidth/cols;
    _playFieldHeight=_playAreaHeight/rows;
    for(int i=0;i<cols;i++) {
        colwidth[i]=_playFieldWidth;
    }
    for(int i=0;i<rows;i++) {
        rowheight[i]=_playFieldHeight;
    }
}

int PlayArea::getMidinoteAtField(int i)
{
    return _fields[0][i].getF1midiNote();
}

int PlayArea::getColumnCount()
{
    return cols;
}

int PlayArea::getPlayAreaWidth()
{
    return _playAreaWidth;
}

int PlayArea::getPlayAreaHeight()
{
    return _playAreaHeight;
}

int PlayArea::getPlayFieldWidth()
{
    return _playFieldWidth;
}

int PlayArea::getPlayFieldHeight()
{
    return _playFieldHeight;
}

void PlayArea::processTouchEvent(misuTouchEvent &touchEvent)
{
    //qDebug() << "MWPlayArea::processPoint " << e.id << " x " << e.x << " y " << e.y << " state " << e.state;

    if(touchEvent.touchId<0) {
        //qDebug() << "ignoring touch event with negative id " << e.id;
        return;
    }

    float hue=0;
    int eventHash=touchEvent.touchId%EVENT_STACK_SIZE;
    EventStackElement * eventStackElement = &eventStack[eventHash];

    // clip
    if(touchEvent.x<0) touchEvent.x=0;
    if(touchEvent.x>=_playAreaWidth) touchEvent.x=_playAreaWidth-1;
    if(touchEvent.y<0) touchEvent.y=0;
    if(touchEvent.y>=_playAreaHeight) touchEvent.y=_playAreaHeight-1;

    int row=0;

    if(_playAreaHeight>0) row = touchEvent.y*rows/_playAreaHeight;
    int col=touchEvent.x*cols/_playAreaWidth;

    if(eventStackElement->active > 0 ) {
        _cc1Sum -= eventStackElement->cc;
    }

    eventStackElement->yrel=(float)(touchEvent.y-row*rowheight[row])/(float)rowheight[row];
    eventStackElement->xrel=(float)(touchEvent.x-col*colwidth[col])/(float)colwidth[col];

    eventStackElement->x=touchEvent.x;
    eventStackElement->y=touchEvent.y;

    Playfield * pf = &_fields[row][col];
    //qDebug() << "row " << row << " col " << col << " eventHash " << eventHash << " " << pf;

    if(pf->getType() != BEND_VERT && pf->getType() != BEND_VERT_HORIZ ) {
        eventStackElement->cc = 1.0f - eventStackElement->yrel;
    } else {
        eventStackElement->cc = eventStackElement->yrel;
    }
    _cc1Sum += eventStackElement->cc;

    float freq;
    int midinote;
    int pitch=0;
    int velocity=127;

    float freqdiff;
    int pitchdiff;
    int huediff;

    switch(pf->getType()) {
    case BEND_HORIZ:
        freqdiff=pf->getF2freq()-pf->getF1freq();
        freqdiff*=eventStackElement->xrel;
        freq=pf->getF1freq()+freqdiff;

        huediff = pf->getF2Hue()-pf->getF1Hue();
        huediff*=eventStackElement->xrel;
        hue = pf->getF1Hue()+huediff;
        if(hue>1) hue-=1;
        if(hue<0) hue+=1;

        pitchdiff=pf->getF2midiNote()*100-pf->getF1midiNote()*100;
        pitchdiff+=pf->getF2pitch();
        pitchdiff-=pf->getF1midiNote();
        pitchdiff*=eventStackElement->xrel;
        pitchdiff+=pf->getF1midiNote()*100;
        midinote=round(static_cast<float>(pitchdiff)/100.0f);
        pitch=pitchdiff-midinote*100;
        //qDebug() << "BEND_HORIZ midinote " << midinote << " pitch " << pitch << " freq " << freq  << " bendVert " << bendVertTop << " " << bendVertBot << " pitchdiff " << pitchdiff;
        break;

    case BEND_VERT_HORIZ:
        pitchdiff=pf->getF2midiNote()*100-pf->getF1midiNote()*100;
        pitchdiff+=pf->getF2pitch();
        pitchdiff-=pf->getF1midiNote();
        pitchdiff*=eventStackElement->xrel;
        pitchdiff+=pf->getF1midiNote()*100;
        if(0==row) {
            pitchdiff+=bendVertTop*100*(1-eventStackElement->yrel);
        } else {
            pitchdiff+=bendVertBot*100*eventStackElement->yrel;
        }
        midinote=round(pitchdiff/100);
        _fcalc.setMidinote(midinote,pitchdiff-midinote*100);
        freq=_fcalc.getFreq();
        hue=_fcalc.getHue();
        break;

    case BEND_VERT:
        if(0==row) {
            pitchdiff=bendVertTop*100;
        } else {
            pitchdiff=bendVertBot*100;
        }
        pitchdiff*=1-eventStackElement->yrel;
        pitchdiff+=pf->getF1midiNote()*100;
        midinote=round(pitchdiff/100);
        pitch = pitchdiff-midinote*100;
        _fcalc.setMidinote(midinote,pitch);
        freq=_fcalc.getFreq();
        hue=_fcalc.getHue();
        //qDebug() << "midinote " << midinote << " pitch " << pitch << " freq " << freq  << " bendVert " << bendVertTop << " " << bendVertBot << " pitchdiff " << pitchdiff;
        break;

    default:
        freq=pf->getF1freq();
        midinote=pf->getF1midiNote();
        pitch=pf->getF1pitch();
        hue=pf->getF1Hue();
        break;
    }

    switch(touchEvent.state) {
    case Qt::TouchPointPressed:
        eventStackElement->touchId=touchEvent.touchId;
        eventStackElement->midinote=midinote;
        eventStackElement->row=row;
        eventStackElement->col=col;
        eventStackElement->f=freq;
        eventStackElement->voiceId=_out->noteOn(freq,midinote,pitch,velocity);
        eventStackElement->active=1;
        pf->incPressed();
        _activeVoices++;
        break;

    case Qt::TouchPointMoved:
        if(row!=eventStackElement->row || col!=eventStackElement->col) {
            Playfield * ppf = &_fields[eventStackElement->row][eventStackElement->col];
            ppf->decPressed();

            eventStackElement->midinote=midinote;

            eventStackElement->row=row;
            eventStackElement->col=col;

            _out->pitch(eventStackElement->voiceId,freq,midinote,pitch);

            eventStackElement->f=freq;
            pf->incPressed();
        } else if(freq!=eventStackElement->f) {
            _out->pitch(eventStackElement->voiceId,freq,midinote,pitch);
            eventStackElement->f=freq;
        }

        break;

    case Qt::TouchPointReleased:

        if(row!=eventStackElement->row || col!=eventStackElement->col) {
            Playfield * ppf = &_fields[eventStackElement->row][eventStackElement->col];
            if(ppf->getPressed()>0) ppf->decPressed();
        } else {
            if(pf->getPressed()>0) pf->decPressed();
        }

        _cc1Sum -= eventStackElement->cc;
        _out->noteOff(eventStackElement->voiceId);

        eventStackElement->active=0;

        _activeVoices--;
        if(_activeVoices == 0) {
            _cc1Sum = 0;
        }
        break;
    }
    pf->calcColor();

    if(_sendCc1 && _activeVoices > 0) {
        _out->cc(eventStackElement->voiceId,1,1.0f-eventStackElement->yrel,_cc1Sum/_activeVoices);
    }

    if(_touchHistory) _touchHistory->insert(touchEvent.x,touchEvent.y,hue);
}

void PlayArea::onSetRootNote(int p)
{
    _rootNote=p;
    config();
}


void PlayArea::setOctConf(int bottom, int top)
{
    _baseOct=bottom;
    _topOct=top;
    config();

}

void PlayArea::setScale(int n, bool v)
{
    //qDebug() << "MWPlayArea::setScale " << n << " " << v;
    _scale[n-1]=v;
    config();
}

void PlayArea::onSetScale(int rootNote, QList<bool> scale)
{
    _rootNote=rootNote;
    _scale=scale;
    config();
}

void PlayArea::onPitchChange(int rootNote, int pitch)
{
    for(int col=0;col<MAX_COLS;col++) {
        for(int row=0;row<MAX_ROWS;row++) {
            _fields[row][col].onPitchChange(rootNote,pitch);
        }
    }
}

void PlayArea::onSendCc1(bool state)
{
    _sendCc1 = state;
}

void PlayArea::setBendHori(bool b)
{
    bendHoriz=b;
    config();
}

void PlayArea::setBendVertTop(int b)
{
    bendVertTop=b;
    config();
}

void PlayArea::setBendVertBot(int b)
{
    bendVertBot=b;
    config();
}

void PlayArea::onBwModeChange(bool state)
{
    for(int r=0;r<MAX_ROWS;r++) {
        for(int c=0;c<MAX_COLS;c++) {
            _fields[r][c].setBwMode(state);
        }
    }
}

void PlayArea::onSymbolsChange(int noteSymbols)
{
    for(int r=0;r<MAX_ROWS;r++) {
        for(int c=0;c<MAX_COLS;c++) {
            _fields[r][c].setNoteSymbols(noteSymbols);
        }
    }
}

void PlayArea::onShowFreqsChange(bool showFreqs)
{
    for(int r=0;r<MAX_ROWS;r++) {
        for(int c=0;c<MAX_COLS;c++) {
            _fields[r][c].showFreqs(showFreqs);
        }
    }
}

void PlayArea::resize(int w, int h)
{
    //qDebug() << "MWPlayArea::resize w: " << w << " h: " << h;
    _playAreaWidth=w;
    _playAreaHeight=h;
    calcGeo();
}

void PlayArea::onPressed(int id, int x, int y)
{
    //qDebug() << "MWPlayArea::onPressed id: " << id << " x: " << x << " y: " << y;
    misuTouchEvent e;
    e.touchId=id;
    e.x=x;
    e.y=y;
    e.state=Qt::TouchPointPressed;
    processTouchEvent(e);
}

void PlayArea::onUpdated(int id, int x, int y)
{
    //qDebug() << "MWPlayArea::onUpdated id: " << id << " x: " << x << " y: " << y;
    misuTouchEvent e;
    e.touchId=id;
    e.x=x;
    e.y=y;
    e.state=Qt::TouchPointMoved;
    processTouchEvent(e);
}

void PlayArea::onReleased(int id, int x, int y)
{
    //qDebug() << "MWPlayArea::onReleased id: " << id << " x: " << x << " y: " << y;
    misuTouchEvent e;
    e.touchId=id;
    e.x=x;
    e.y=y;
    e.state=Qt::TouchPointReleased;
    processTouchEvent(e);
}

QList<QObject *> PlayArea::row0()
{
    QList<QObject*> colsList;
    for(int j=0;j<cols;j++) {
        _fields[0][j].calcColor();
        colsList.append(&_fields[0][j]);
    }
    return colsList;
}

QList<QObject *> PlayArea::row1()
{
    QList<QObject*> colsList;
    for(int j=0;j<cols;j++) {
        _fields[1][j].calcColor();
        colsList.append(&_fields[1][j]);
    }
    return colsList;
}

QList<QObject *> PlayArea::row2()
{
    QList<QObject*> colsList;
    for(int j=0;j<cols;j++) {
        _fields[2][j].calcColor();
        colsList.append(&_fields[2][j]);
    }
    return colsList;
}
