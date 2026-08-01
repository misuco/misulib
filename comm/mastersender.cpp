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
#include <QDebug>

MasterSender::~MasterSender()
{
}

void MasterSender::cc(int voiceId, int cc, float value)
{
    qDebug() << "MasterSender::cc( voiceId: " << voiceId << ", cc: " << cc << ", value: " << value << ")";
    emit sigCc(voiceId,cc,value);
}

void MasterSender::ccAllVoices(int cc, float value)
{
    qDebug() << "MasterSender::ccAllVoices( cc: " << cc << ", value: " << value << ")";
    emit sigCcAllVoices(cc,value);
}

void MasterSender::pc(int value)
{
    emit sigPc(value);
}

int MasterSender::noteOn(float f, int midinote, int pitch, int v)
{
    qDebug() << "MasterSender::noteOn f: " << f << ", midinote: " << midinote << ", pitch: " << pitch << ", v: " << v << ")";
    int voiceId=nextVoiceId;
    nextVoiceId++;
    if(nextVoiceId>87)  nextVoiceId=1;
    emit sigNoteOn(voiceId,f,midinote,pitch,v);
    return voiceId;
}

void MasterSender::noteOff(int voiceId)
{
    qDebug() << "MasterSender::noteOff voiceId: " << voiceId << ")";
    emit sigNoteOff(voiceId);
}

void MasterSender::pitch(int voiceId, float f, int midinote, int pitch)
{
    qDebug() << "MasterSender::pitch( voideId: " << voiceId << ", f: " << f << ", midinote: " << midinote << ", pitch: " << pitch << ")";
    emit sigPitch(voiceId,f,midinote,pitch);
}

void MasterSender::addSenderThread(QObject *s, QString name)
{
    qDebug() << "MasterSender::addSenderThread(" << name << "," << s << ")";
    QThread * t = new SenderThread();
    t->setObjectName(name);
    t->start(QThread::TimeCriticalPriority);
    s->moveToThread(t);
    mSenders.insert(name,s);
}

void MasterSender::connectSender(QObject * s) {
    qDebug() << "MasterSender::connectSender " << s;
    connect(this,SIGNAL(sigNoteOn(int,float,int,int,int)),s,SLOT(noteOn(int,float,int,int,int)));
    connect(this,SIGNAL(sigNoteOff(int)),s,SLOT(noteOff(int)));
    connect(this,SIGNAL(sigPitch(int,float,int,int)),s,SLOT(pitch(int,float,int,int)));
    connect(this,SIGNAL(sigCc(int,int,float)),s,SLOT(cc(int,int,float)));
    connect(this,SIGNAL(sigCcAllVoices(int,float)),s,SLOT(ccAllVoices(int,float)));
    connect(this,SIGNAL(sigPc(int)),s,SLOT(pc(int)));
}

void MasterSender::disconnectSender(QObject *s)
{
    qDebug() << "MasterSender::disconnectSender " << s;
    this->disconnect(s);
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
