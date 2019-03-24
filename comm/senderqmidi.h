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

#ifndef SENDERQMIDI_H
#define SENDERQMIDI_H

#include <QObject>
#include "lib/qmidi/src/QMidiOut.h"

class SenderQMidi : public QObject
{
    Q_OBJECT

public:
    explicit SenderQMidi(QObject * parent = nullptr);
    ~SenderQMidi();

public slots:
    void cc(int voiceId, int cc, float v1, float);
    void pc(int);
    void noteOn(int voiceId, float f, int midinote, int pitch, int vel);
    void noteOff(int voiceId);
    void pitch(int voiceId, float f, int midinote, int pitch);

private:
    QMidiOut * _midiOut;
    int     _voiceId2Midinote[64];
    int     _noteOnCount[128];
};

#endif // SENDERQMIDI_H
