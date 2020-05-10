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

#include "mastersender.h"
#include "senderthread.h"
#include "sendermobilesynth.h"

MasterSender::MasterSender(QObject *parent) : QObject(parent)
{
    nextVoiceId=1;

}

MasterSender::~MasterSender()
{
}

void MasterSender::cc(int voiceId, int cc, float v1, float v1avg)
{
    //qDebug() << "SenderMulti::cc(" << chan << "," << voiceId << "," << cc << "," << v1 << "," << v1avg << ")";
    emit sigCc(voiceId,cc,v1,v1avg);
}

void MasterSender::pc(int v1)
{
    emit sigPc(v1);
}

int MasterSender::noteOn(float f, int midinote, int pitch, int v)
{
    int vid=nextVoiceId;
    nextVoiceId++;
    if(nextVoiceId>1023)  nextVoiceId=1;
    emit sigNoteOn(vid,f,midinote,pitch,v);
    return vid;
}

void MasterSender::noteOff(int voiceId)
{
    emit sigNoteOff(voiceId);
}

void MasterSender::pitch(int voiceId, float f, int midinote, int pitch)
{
    //qDebug() << "SenderMulti::pitch(" << chan << "," << voiceId << "," << f << "," << midinote << "," << pitch << ")";
    emit sigPitch(voiceId,f,midinote,pitch);
}

void MasterSender::addSenderThread(QObject *s, QString name)
{
    QThread * t = new SenderThread();
    t->setObjectName(name);
    t->start(QThread::TimeCriticalPriority);
    s->moveToThread(t);
    connectSender(s);
    mSenders.insert(name,s);
}

void MasterSender::connectSender(QObject * s) {
    connect(this,SIGNAL(sigNoteOn(int,float,int,int,int)),s,SLOT(noteOn(int,float,int,int,int)));
    connect(this,SIGNAL(sigNoteOff(int)),s,SLOT(noteOff(int)));
    connect(this,SIGNAL(sigPitch(int,float,int,int)),s,SLOT(pitch(int,float,int,int)));
    connect(this,SIGNAL(sigCc(int,int,float,float)),s,SLOT(cc(int,int,float,float)));
    connect(this,SIGNAL(sigPc(int)),s,SLOT(pc(int)));
}

void MasterSender::disconnectSender(QObject *s)
{
    disconnect(this,SIGNAL(sigNoteOn(int,float,int,int,int)),s,SLOT(noteOn(int,float,int,int,int)));
    disconnect(this,SIGNAL(sigNoteOff(int)),s,SLOT(noteOff(int)));
    disconnect(this,SIGNAL(sigPitch(int,float,int,int)),s,SLOT(pitch(int,float,int,int)));
    disconnect(this,SIGNAL(sigCc(int,int,float,float)),s,SLOT(cc(int,int,float,float)));
    disconnect(this,SIGNAL(sigPc(int)),s,SLOT(pc(int)));
}

void MasterSender::onToggleSender(QString id, bool value)
{
    QObject * sender = mSenders.value(id,nullptr);

    if(sender) {
        if(value == true) {
            connectSender(sender);
        } else {
            disconnectSender(sender);
        }
    }
}
