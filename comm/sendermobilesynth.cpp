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
#include <QDebug>
#include <QtGlobal>
#include "sendermobilesynth.h"


SenderMobileSynth::SenderMobileSynth(QObject * parent) : QObject(parent)
{
    sy= new MobileSynth();
    qDebug() << "+++++++++++++++++++++++";
    qDebug() << "mobileSynthQt68 created";
}

SenderMobileSynth::~SenderMobileSynth()
{
    delete(sy);
}

void SenderMobileSynth::cc(int voiceId, int cc, float v1, float)
{
    if(cc==1) {
        sy->set_modulation_amount(v1);
    } else if(cc==102) {
        sy->set_osc1_wave_type(v1);
    } else if(cc==103) {
        sy->set_volume_attack(100*pow(10,v1/250.0f));
    } else if(cc==104) {
        sy->set_volume_decay(100*pow(10,v1/250.0f));
    } else if(cc==105) {
        sy->set_volume_sustain(v1/1000.0f);
    } else if(cc==106) {
        sy->set_volume_release(100*pow(10,v1/250.0f));
    } else if(cc==107) {
        sy->set_filter_cutoff(v1/1000.0f);
    } else if(cc==108) {
        sy->set_filter_resonance(v1/500.0f);
    } else if(cc==109) {
        sy->set_modulation_frequency(v1/50.0f);
        sy->set_modulation_source(1);
        sy->set_modulation_destination(1);
    } else if(cc==110) {
        sy->set_modulation_amount(v1/1000.0f);
        sy->set_modulation_source(1);
        sy->set_modulation_destination(0);
    } else if(cc==111) {
        sy->set_osc1_level(v1/1000.0f);
    }
}

void SenderMobileSynth::pc(int) {}

void SenderMobileSynth::noteOn(int voiceId, float f, int, int, int)
{
    qDebug() << "------------------------------";
    qDebug() << "SenderMobileSynth::noteOn voiceId: " << voiceId << " f: " << f;
    sy->noteOn(voiceId, (float)f);
}

void SenderMobileSynth::noteOff(int voiceId)
{
    qDebug() << "SenderMobileSynth::noteOff voiceId: " << voiceId;
    sy->noteOff(voiceId);
}

void SenderMobileSynth::pitch(int voiceId, float f, int, int) {
    qDebug() << "SenderMobileSynth::pitch voiceId: " << voiceId << " f: " << f;
    sy->pitch(voiceId, (float)f);
}
