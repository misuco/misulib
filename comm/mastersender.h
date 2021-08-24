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

#ifndef SENDERMULTI_H
#define SENDERMULTI_H

#include <QObject>
#include <QList>
#include "lib/misulib/comm/libofqf/qoscclient.h"

class MasterSender : public QObject
{
    Q_OBJECT

public:
    explicit MasterSender(QObject * parent = nullptr);
    ~MasterSender();
    void cc(int nextVoiceId, int cc, float v1, float v1avg);
    void pc(int v1);
    int noteOn(float f, int midinote, int pitch, int v);
    void noteOff(int voiceId);
    void pitch(int voiceId, float f, int midinote, int pitch);
    void addSenderThread(QObject *s, QString name);

public slots:
    void onToggleSender(QString id, bool value);

signals:
    void sigCc(int nextVoiceId, int cc, float v1, float v1avg);
    void sigPc(int v1);
    int sigNoteOn(int vid, float f, int midinote, int pitch, int v);
    void sigNoteOff(int voiceId);
    void sigPitch(int voiceId, float f, int midinote, int pitch);

private:
    QMap<QString,QObject *> mSenders;
    int nextVoiceId;
    void connectSender(QObject *s);
    void disconnectSender(QObject *s);
};

#endif // SENDERMULTI_H
