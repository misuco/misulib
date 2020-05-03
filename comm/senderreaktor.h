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

#ifndef SENDERREAKTOR_H
#define SENDERREAKTOR_H

#include <QObject>
#include "lib/misulib/comm/libofqf/qoscclientinterface.h"

class SenderReaktor : public QObject
{
    Q_OBJECT

public:
    explicit SenderReaktor(QObject * parent = nullptr);
    ~SenderReaktor();

    void setDestination(char * a, int p);
    void reconnect();
    int getPort() {return port;}
    char* getAddress() {return adr;}

    void setChannel(int c);

public slots:
    void cc(int voiceId, int cc, float v1, float v1avg);
    void pc(int v1);
    void noteOn(int voiceId, float f, int midinote, int pitch, int v);
    void noteOff(int voiceId);
    void pitch(int voiceId, float f, int, int);

private:
    QOscClientInterface* oscout;
    char * adr;
    quint16 port;
    int _channel;

    int prog;              // current program
    int * notestate;   // currently played notes
    int * notechan;    // currently played notes chan
    int * notevel;    // currently played notes velocity
    int * ccstate;       // current ccval;

    void sendOsc(QString path, QVariant list);
};

#endif // SENDERREAKTOR_H
