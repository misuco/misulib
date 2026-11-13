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
#include <QDebug>
#include <QtGlobal>
#include <algorithm>
#include "sendersoundfont.h"


SenderSoundFont::SenderSoundFont(QObject * parent) : QObject(parent)
{
    sy = std::make_shared<SoundFontSynth>();
    qDebug() << "soundFontSynth created";
}

SenderSoundFont::SenderSoundFont(std::shared_ptr<SoundFontSynth> soundFontSynth, QObject *parent) : QObject(parent)
{
    sy = soundFontSynth;
    qDebug() << "soundFontSynth injected";
}

SenderSoundFont::~SenderSoundFont()
{
}

bool SenderSoundFont::ccToMidi(int cc, float value, int &midiValue)
{
    // the expected values are normalized (0...1)
    switch(cc) {
    case CCModAmount:
    case CCFilterResonance:
    case CCFilterCutoff:
        midiValue = static_cast<int>( 127 * std::clamp(value, 0.0f, 1.0f) );
        return true;
    default:
        return false;
    }
}

void SenderSoundFont::cc(int voiceId, int cc, float value)
{
    int midiValue;
    if(ccToMidi(cc, value, midiValue)) {
        sy->controlChange(voiceId, cc, midiValue);
    }
}

void SenderSoundFont::ccAllVoices(int cc, float value)
{
    int midiValue;
    if(ccToMidi(cc, value, midiValue)) {
        sy->controlChangeAllVoices(cc, midiValue);
    }
}

void SenderSoundFont::pc(int value)
{
    // the presets belong to the thread of the synth
    SoundFontSynth * synth = sy.get();
    QMetaObject::invokeMethod(synth, [synth, value]() {
        synth->setPresetIndex(value);
    }, Qt::QueuedConnection);
}

void SenderSoundFont::noteOn(int voiceId, float f, int, int, int v)
{
    sy->noteOn(voiceId, f, v);
}

void SenderSoundFont::noteOff(int voiceId)
{
    sy->noteOff(voiceId);
}

void SenderSoundFont::pitch(int voiceId, float f, int, int)
{
    sy->pitch(voiceId, f);
}
