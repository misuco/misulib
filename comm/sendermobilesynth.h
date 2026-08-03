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
#include <QtQmlIntegration/qqmlintegration.h>

class SenderMobileSynth : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot instantiate SenderMobileSynth")

public:
    explicit SenderMobileSynth(QObject * parent = nullptr);
    explicit SenderMobileSynth(std::shared_ptr<MobileSynth> mobileSynth, QObject * parent = nullptr);
    ~SenderMobileSynth();

    enum CcNum {
        CCModAmount = 1,
        CCArpeggioEnabled = 20,
        CCArpeggioSamples = 21,
        CCArpeggioOctaves = 22,
        CCArpeggioStep = 23,
        CCOsc1Level = 24,
        CCOsc1WaveType = 25,
        CCOsc1Octave = 26,
        CCOsc2Level = 27,
        CCOsc2WaveType = 28,
        CCOsc2Octave = 29,
        CCOsc2Shift = 30,
        CCOscSync = 31,
        CCFilterResonance = 71,
        CCFilterCutoff = 74,
        CCModSource = 85,
        CCModDest = 86,
        CCModFreq = 87,
        CCAmpEnvAttack = 102,
        CCAmpEnvDecay = 103,
        CCAmpEnvSustain = 104,
        CCAmpEnvRelease = 105,
        CCFilEnvAttack = 106,
        CCFilEnvDecay = 107,
        CCFilEnvSustain = 108,
        CCFilEnvRelease = 109,
        CCGlideSamples = 110
    };
    Q_ENUM(CcNum)

public:
    std::shared_ptr<MobileSynth> getSynthController() {
        return sy;
    }

public slots:
    void cc(int voiceId, int cc, float value);
    void ccAllVoices(int cc, float value);
    void pc(int);
    void noteOn(int voiceId, float f, int midinote, int pitch, int v);
    void noteOff(int voiceId);
    void pitch(int voiceId, float f, int, int);
    
private:
    std::shared_ptr<MobileSynth> sy;
    void ccToSynth(int voiceId, int cc, float value, bool allVoices);
};

#endif // SENDERMOBILESYNTH_H
