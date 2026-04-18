/*
 
 Copyright (C) 2013 by Claudio Zopfi, Zurich, Suisse, z@x21.ch
 
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

#ifndef SENDERMOBILESYNTH_H
#define SENDERMOBILESYNTH_H

#include <QObject>
#include <QtGlobal>
#include "polymobilesynth/qt6/mobilesynth.h"

class SenderMobileSynth : public QObject
{
    Q_OBJECT

public:
    explicit SenderMobileSynth(QObject * parent = nullptr);
    ~SenderMobileSynth();

public:
    MobileSynth * getSynthController() {
        return sy;
    }

public slots:
    void cc(int voiceId, int cc, float v1, float);
    void pc(int);
    void noteOn(int voiceId, float f, int midinote, int pitch, int v);
    void noteOff(int voiceId);
    void pitch(int voiceId, float f, int, int);
    
private:
    MobileSynth * sy;
};

#endif // SENDERMOBILESYNTH_H
