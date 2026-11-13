/*

 Copyright (C) 2026 by Claudio Zopfi, Zurich, Suisse, z@x21.ch

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

#ifndef SENDERSOUNDFONT_H
#define SENDERSOUNDFONT_H

#include <QObject>
#include <QtGlobal>
#include "polysoundfont/qt6/soundfontsynth.h"
#include <QtQmlIntegration/qqmlintegration.h>

#include <memory>

class SenderSoundFont : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot instantiate SenderSoundFont")

public:
    explicit SenderSoundFont(QObject * parent = nullptr);
    explicit SenderSoundFont(std::shared_ptr<SoundFontSynth> soundFontSynth, QObject * parent = nullptr);
    ~SenderSoundFont();

    // The controller numbers of misuco that have a meaning for a
    // soundfont player. They are passed on as midi controllers, all
    // other controllers are ignored.
    enum CcNum {
        CCModAmount = 1,
        CCFilterResonance = 71,
        CCFilterCutoff = 74
    };
    Q_ENUM(CcNum)

public:
    std::shared_ptr<SoundFontSynth> getSynthController() {
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
    std::shared_ptr<SoundFontSynth> sy;
    static bool ccToMidi(int cc, float value, int &midiValue);
};

#endif // SENDERSOUNDFONT_H
