/*
 
 Copyright (C) 2019 by Claudio Zopfi, Zurich, Suisse, z@x21.ch
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */
#include <QDebug>
#include "senderqmidi.h"


SenderQMidi::SenderQMidi(QObject * parent) : QObject(parent),
    _midiOut(new QMidiOut())
{    
    QMap<QString, QString> vals = QMidiOut::devices();
    for (QString key : vals.keys()) {
        QString value = vals.value(key);
        qDebug() << key << " " << value;
    }
    _midiOut->connect(vals.keys().first());

    for(int i=0;i<128;i++) {
        _noteOnCount[i]=0;
    }
}

SenderQMidi::~SenderQMidi()
{
    _midiOut->disconnect();
    delete(_midiOut);
}

void SenderQMidi::cc(int voiceId, int cc, float v1, float)
{
}

void SenderQMidi::pc(int) {}

void SenderQMidi::noteOn(int voiceId, float f, int midinote, int pitch, int vel)
{    
    _noteOnCount[midinote]++;
    _voiceId2Midinote[voiceId%64]=midinote;
    _midiOut->noteOn(midinote+2,0,vel);
    _midiOut->pitchWheel(0,pitch);
}

void SenderQMidi::noteOff(int voiceId)
{
    int midinote = _voiceId2Midinote[voiceId%64];
    if(midinote < 128) {
        _noteOnCount[midinote]--;
        if(_noteOnCount[midinote]<1) {
            _midiOut->noteOff(midinote+2,0);
        }
    }
}

void SenderQMidi::pitch(int voiceId, float f, int midinote, int pitch) {
}

