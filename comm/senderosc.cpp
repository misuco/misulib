/*

 Copyright (C) 2013-2026 by Claudio Zopfi, Zurich, Suisse, z@x21.ch

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

#include "senderosc.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QTimer>
#include <algorithm>

#include "qoscclient.h"
#include "qosctcpclient.h"

SenderOsc::SenderOsc(QObject *parent) : QObject(parent)
{
    std::fill(std::begin(voiceNote), std::end(voiceNote), -1);
    std::fill(std::begin(voiceChannel), std::end(voiceChannel), 1);
    std::fill(std::begin(voiceIds), std::end(voiceIds), 0);
    std::fill(std::begin(voiceVelocity), std::end(voiceVelocity), 127);
    std::fill(std::begin(ccState), std::end(ccState), -1);
}

SenderOsc::~SenderOsc()
{
}

void SenderOsc::start(QString host, int port, bool tcp, int f, int c)
{
    stop();

    // the clients take addresses only, no names
    QHostAddress address(host.trimmed());
    if(address.isNull()) {
        setState(Failed, QStringLiteral("\"%1\" is not an IP address").arg(host.trimmed()));
        return;
    }
    QByteArray adr = address.toString().toLatin1();

    format = std::clamp(f, int(MidiGeneric), int(SuperCollider));
    channel = std::clamp(c, 1, 16);
    destination = QStringLiteral("%1:%2").arg(address.toString()).arg(port);

    // the client is created here, to live in the sender thread
    if(tcp) {
        tcpClient = new QOscTcpClient(this);
        tcpClient->setAddress(adr.data(), static_cast<quint16>(port));
        client = tcpClient;
        clientObject = tcpClient;

        // the tcp client connects again by itself when something is sent
        if(!checkTimer) {
            checkTimer = new QTimer(this);
            checkTimer->setInterval(500);
            connect(checkTimer, &QTimer::timeout, this, &SenderOsc::checkConnection);
        }
        checkTimer->start();
        setState(Connecting, QStringLiteral("Connecting to %1 (TCP)").arg(destination));
    } else {
        QOscClient * udp = new QOscClient(this);
        udp->setAddress(adr.data(), static_cast<quint16>(port));
        client = udp;
        clientObject = udp;
        setState(Sending, QStringLiteral("Sending to %1 (UDP)").arg(destination));
    }
}

void SenderOsc::stop()
{
    if(client) {
        allNotesOff();
    }
    if(checkTimer) {
        checkTimer->stop();
    }
    delete clientObject;
    clientObject = nullptr;
    client = nullptr;
    tcpClient = nullptr;
    std::fill(std::begin(voiceNote), std::end(voiceNote), -1);
    std::fill(std::begin(ccState), std::end(ccState), -1);
    if(state != Off) {
        setState(Off, QStringLiteral("Off"));
    }
}

void SenderOsc::checkConnection()
{
    if(!tcpClient) {
        return;
    }
    if(tcpClient->state() == QAbstractSocket::ConnectedState) {
        if(state != Sending) {
            setState(Sending, QStringLiteral("Connected to %1 (TCP)").arg(destination));
        }
    } else {
        if(state != Connecting) {
            setState(Connecting, QStringLiteral("Connecting to %1 (TCP)").arg(destination));
        }
        // try again
        if(tcpClient->state() == QAbstractSocket::UnconnectedState) {
            QByteArray adr = destination.left(destination.lastIndexOf(QChar(':'))).toLatin1();
            int port = destination.mid(destination.lastIndexOf(QChar(':')) + 1).toInt();
            tcpClient->setAddress(adr.data(), static_cast<quint16>(port));
        }
    }
}

void SenderOsc::setFormat(int f)
{
    f = std::clamp(f, int(MidiGeneric), int(SuperCollider));
    if(f == format) {
        return;
    }
    // the notes are switched off in the format they have been switched on
    if(client) {
        allNotesOff();
    }
    format = f;
}

void SenderOsc::setChannel(int c)
{
    // the notes that sound are switched off on the channel they have been sent on
    channel = std::clamp(c, 1, 16);
}

void SenderOsc::setState(int s, const QString &message)
{
    state = s;
    emit stateChanged(s, message);
}

void SenderOsc::send(const QString &path, const QVariantList &values)
{
    if(client) {
        client->sendData(path, values);
    }
}

void SenderOsc::sendBend(int pitch)
{
    // misuco pitch is in cent (100 per semitone).
    // MidiGeneric: 4096 per semitone, the 14 bit pitch bend with a
    // range of +-2 semitones.
    // Reaktor: 2048 per semitone.
    int bend = format == Reaktor ? pitch * 2048 / 100 : pitch * 4096 / 100;
    send(QStringLiteral("/pitch/%1").arg(channel), {bend});
}

void SenderOsc::noteOn(int voiceId, float f, int midinote, int pitch, int v)
{
    if(!client) {
        return;
    }
    int voice = voiceId & (Voices - 1);

    switch(format) {
    case SuperCollider:
        send(QStringLiteral("/s_new"), {synthName, voiceId, 0, 0,
                                        QStringLiteral("freq"), f, QStringLiteral("amp"), v});
        break;
    case Reaktor:
        send(QStringLiteral("/note/%1").arg(channel), {midinote, v});
        sendBend(pitch);
        break;
    default:
        sendBend(pitch);
        send(QStringLiteral("/note/%1").arg(channel), {midinote, v, channel});
        break;
    }

    voiceNote[voice] = midinote;
    voiceIds[voice] = voiceId;
    voiceChannel[voice] = channel;
    voiceVelocity[voice] = v;
}

void SenderOsc::noteOff(int voiceId)
{
    if(!client) {
        return;
    }
    int voice = voiceId & (Voices - 1);
    if(voiceNote[voice] < 0) {
        return;
    }

    switch(format) {
    case SuperCollider:
        send(QStringLiteral("/n_set"), {voiceId, QStringLiteral("amp"), 0});
        break;
    case Reaktor:
        send(QStringLiteral("/note/%1").arg(voiceChannel[voice]), {voiceNote[voice], 0});
        break;
    default:
        send(QStringLiteral("/note/%1").arg(voiceChannel[voice]), {voiceNote[voice], 0, voiceChannel[voice]});
        break;
    }
    voiceNote[voice] = -1;
}

void SenderOsc::pitch(int voiceId, float f, int midinote, int pitch)
{
    if(!client) {
        return;
    }
    int voice = voiceId & (Voices - 1);
    if(voiceNote[voice] < 0) {
        return;
    }

    if(format == SuperCollider) {
        send(QStringLiteral("/n_set"), {voiceId, QStringLiteral("freq"), f});
        voiceNote[voice] = midinote;
    } else if(voiceNote[voice] != midinote) {
        int velocity = voiceVelocity[voice];
        noteOff(voiceId);
        noteOn(voiceId, f, midinote, pitch, velocity);
    } else {
        sendBend(pitch);
    }
}

void SenderOsc::cc(int voiceId, int cc, float value)
{
    if(!client) {
        return;
    }
    if(format == SuperCollider) {
        send(QStringLiteral("/n_set"), {voiceId, QStringLiteral("par%1").arg(cc), value});
    } else {
        // the midi formats have no controller for a single voice
        ccAllVoices(cc, value);
    }
}

void SenderOsc::ccAllVoices(int cc, float value)
{
    if(!client) {
        return;
    }
    if(format == SuperCollider) {
        for(int voice = 0; voice < Voices; voice++) {
            if(voiceNote[voice] >= 0) {
                send(QStringLiteral("/n_set"), {voiceIds[voice], QStringLiteral("par%1").arg(cc), value});
            }
        }
        return;
    }

    if(cc < 0 || cc > 127) {
        return;
    }
    int midiValue = std::clamp(qRound(127.0f * value), 0, 127);
    if(midiValue == ccState[cc]) {
        return;
    }
    ccState[cc] = midiValue;
    const QString path = QStringLiteral("/cc/%1/%2").arg(channel).arg(cc);
    if(format == Reaktor) {
        send(path, {midiValue});
    } else {
        send(path, {midiValue, cc, channel});
    }
}

void SenderOsc::pc(int v1)
{
    if(!client) {
        return;
    }
    switch(format) {
    case SuperCollider:
        // the program is the synth of the notes that follow
        synthName = QStringLiteral("m%1").arg(v1);
        break;
    case Reaktor:
        send(QStringLiteral("/pc/%1").arg(channel), {v1});
        break;
    default:
        send(QStringLiteral("/pc/%1").arg(channel), {v1, channel});
        break;
    }
}

void SenderOsc::allNotesOff()
{
    for(int voice = 0; voice < Voices; voice++) {
        if(voiceNote[voice] >= 0) {
            noteOff(voiceIds[voice]);
        }
    }
}
