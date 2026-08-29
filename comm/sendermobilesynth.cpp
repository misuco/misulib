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
    sy = std::make_shared<MobileSynth>();
    qDebug() << "+++++++++++++++++++++++";
    qDebug() << "mobileSynthQt68 created";
}

SenderMobileSynth::SenderMobileSynth(std::shared_ptr<MobileSynth> mobileSynth, QObject *parent)
{
    sy = mobileSynth;
    qDebug() << "+++++++++++++++++++++++";
    qDebug() << "mobileSynthQt68 injected";
}

SenderMobileSynth::~SenderMobileSynth()
{
}

void SenderMobileSynth::cc(int voiceId, int cc, float value)
{
    ccToSynth(voiceId,cc,value,false);
}

void SenderMobileSynth::ccAllVoices(int cc, float value)
{
    ccToSynth(0,cc,value,true);
}

void SenderMobileSynth::ccToSynth(int voiceId, int cc, float value, bool allVoices)
{
    // the expected values for v1 are normalized (0...1)
    // and scaled here trought constant values

    const int osc_min_wave_type = 0;
    const int osc_max_wave_type = 4;
    const int osc_min_octave = 0;
    const int osc_max_octave = 4;
    const long attack_min = 10;
    const long attack_max = 400000;
    const long decay_min = 10;
    const long decay_max = 400000;
    const long release_min = 10;
    const long release_max = 400000;

    if(cc==CCArpeggioEnabled) {

        bool v = value>0;

        sy->set_arpeggio_enabled( v );

    } else if(cc==CCArpeggioSamples) {

        const int arp_min_size = 10;
        const int arp_max_increase = 1000;
        float v = arp_min_size + arp_max_increase * std::clamp(value, 0.0f, 1.0f);

        sy->set_arpeggio_samples( v );

    } else if(cc==CCArpeggioOctaves) {

        const int arp_min_octaves = 1;
        const int arp_max_octaves = 4;
        int v = std::clamp(static_cast<int>(value), arp_min_octaves, arp_max_octaves);

        sy->set_arpeggio_octaves( v );

    } else if(cc==CCArpeggioStep) {

        const int arp_min_step = 0;
        const int arp_max_step = 3;
        int v = std::clamp(static_cast<int>(value), arp_min_step, arp_max_step);

        sy->set_arpeggio_step( v );

    } else if(cc==CCGlideSamples) {

        const int glide_smp_max = 400000;
        int v = static_cast<int>( glide_smp_max * std::clamp(value, 0.0f, 1.0f) );

        sy->set_glide_samples( v );

    } else if(cc==CCFilterCutoff) {

        const float filter_min_cutoff = 1.0f;
        const float filter_max_cutoff_increase = 1000.0f;
        float v = filter_min_cutoff + filter_max_cutoff_increase * std::clamp(value, 0.0f, 1.0f);

        sy->set_filter_cutoff( v );

    } else if(cc==CCFilterResonance) {

        const float filter_max_reso = 10.0f;
        float v = static_cast<float>( filter_max_reso * std::clamp(value, 0.0f, 1.0f) );

        sy->set_filter_resonance( v );

    } else if(cc==CCOsc1Level) {

        float v = std::clamp(value, 0.0f, 1.0f);

        sy->set_osc1_level( v );

    } else if(cc==CCOsc1WaveType) {

        int v = std::clamp(static_cast<int>(value), osc_min_wave_type, osc_max_wave_type);

        sy->set_osc1_wave_type( v );

    } else if(cc==CCOsc1Octave) {

        int v = std::clamp(static_cast<int>(value), osc_min_octave, osc_max_octave);

        sy->set_osc1_octave( v );

    } else if(cc==CCOsc2Level) {

        float v = std::clamp(value, 0.0f, 1.0f);

        sy->set_osc2_level( v );

    } else if(cc==CCOsc2WaveType) {

        int v = std::clamp(static_cast<int>(value), osc_min_wave_type, osc_max_wave_type);

        sy->set_osc2_wave_type( v );

    } else if(cc==CCOsc2Octave) {

        int v = std::clamp(static_cast<int>(value), osc_min_octave, osc_max_octave);

        sy->set_osc2_octave( v );

    } else if(cc==CCOsc2Shift) {

        const float osc2_max_shift = 1200;
        int v = static_cast<int>( osc2_max_shift * std::clamp(value, 0.0f, 1.0f) );

        sy->set_osc2_shift( v );

    } else if(cc==CCOscSync) {

            bool v = value>0;

            sy->set_osc_sync( v );

    } else if(cc==CCModSource) {

        const int mod_min_source = 0;
        const int mod_max_source = 3;
        int v = std::clamp(static_cast<int>(value), mod_min_source, mod_max_source);

        sy->set_modulation_source( v );

    } else if(cc==CCModDest) {

        const int mod_min_dest = 0;
        const int mod_max_dest = 3;
        int v = std::clamp(static_cast<int>(value), mod_min_dest, mod_max_dest);

        sy->set_modulation_destination( v );

    } else if(cc==CCModAmount) {

        float v = std::clamp(value, 0.0f, 1.0f);

        if(allVoices) {
            sy->set_modulation_amount( v );
        } else {
            qDebug() << "ccToSynth: sy->set_modulation_amount " << voiceId << " " << v;
            sy->set_modulation_amount( voiceId, v );
        }

    } else if(cc==CCModFreq) {

        const float mod_f_min = 0.1f;
        const float mod_f_max_inc = 200.0f;
        float v = mod_f_min + mod_f_max_inc * std::clamp(value, 0.0f, 1.0f);

        sy->set_modulation_frequency(v);

    } else if(cc==CCAmpEnvAttack) {

        float v = attack_min + attack_max * std::clamp(value, 0.0f, 1.0f);

        sy->set_volume_attack(v);

    } else if(cc==CCAmpEnvDecay) {

        float v = decay_min + decay_max * std::clamp(value, 0.0f, 1.0f);

        sy->set_volume_decay(v);

    } else if(cc==CCAmpEnvSustain) {

        float v = std::clamp(value, 0.0f, 1.0f);

        sy->set_volume_sustain(v);

    } else if(cc==CCAmpEnvRelease) {

        float v = release_min + release_max * std::clamp(value, 0.0f, 1.0f);

        sy->set_volume_release(v);

    } else if(cc==CCFilEnvAttack) {

        float v = attack_min + attack_max * std::clamp(value, 0.0f, 1.0f);

        sy->set_filter_attack(v);

    } else if(cc==CCFilEnvDecay) {

        float v = decay_min + decay_max * std::clamp(value, 0.0f, 1.0f);

        sy->set_filter_decay(v);

    } else if(cc==CCFilEnvSustain) {

        float v = std::clamp(value, 0.0f, 1.0f);

        sy->set_filter_sustain(v);

    } else if(cc==CCFilEnvRelease) {

        float v = release_min + release_max * std::clamp(value, 0.0f, 1.0f);

        sy->set_filter_release(v);
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
