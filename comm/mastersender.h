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

#include <QtQmlIntegration/qqmlintegration.h>

#include <QObject>
#include <QList>
#include <QMap>

class MasterSender : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot instantiate MasterSender")

public:
    explicit MasterSender() = default;
    ~MasterSender();

    Q_INVOKABLE void cc(int voiceId, int cc, float value);
    Q_INVOKABLE void ccAllVoices(int cc, float value);
    Q_INVOKABLE void pc(int value);
    Q_INVOKABLE int noteOn(float f, int midinote, int pitch, int velocity);
    Q_INVOKABLE void noteOff(int voiceId);
    Q_INVOKABLE void pitch(int voiceId, float f, int midinote, int pitch);

    void addSenderThread(QObject *s, QString name);

public slots:
    void onToggleSender(QString id, bool value);

signals:
    void sigCc(int voiceId, int cc, float value);
    void sigCcAllVoices(int cc, float value);
    void sigPc(int value);
    int sigNoteOn(int voiceId, float f, int midinote, int pitch, int velocity);
    void sigNoteOff(int voiceId);
    void sigPitch(int voiceId, float f, int midinote, int pitch);

private:
    QMap<QString,QObject *> mSenders;
    int nextVoiceId{1};

    void connectSender(QObject *s);
    void disconnectSender(QObject *s);
};

#endif // SENDERMULTI_H
