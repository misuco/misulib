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

#include "senderrtpmidi.h"

#include <QUdpSocket>
#include <QTimer>
#include <QHostInfo>
#include <QDataStream>
#include <QNetworkDatagram>
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>

namespace {

// AppleMIDI session protocol
const quint16 Signature = 0xFFFF;
const quint16 CmdInvitation = 0x494E;   // IN
const quint16 CmdAccept = 0x4F4B;       // OK
const quint16 CmdReject = 0x4E4F;       // NO
const quint16 CmdBye = 0x4259;          // BY
const quint16 CmdSync = 0x434B;         // CK
const quint32 ProtocolVersion = 2;

// RTP
const quint8 RtpVersion = 0x80;         // V=2, no padding, no extension, no CSRC
const quint8 RtpPayloadType = 0x61;     // 97, dynamic payload type used by AppleMIDI

const int InviteInterval = 1000;        // ms
const int InviteMaxAttempts = 12;
const int SyncIntervalStart = 1500;     // ms, the first syncs
const int SyncCountStart = 6;
const int SyncInterval = 10000;         // ms, afterwards

}

SenderRtpMidi::SenderRtpMidi(QObject *parent) : QObject(parent)
{
    std::fill(std::begin(voiceNote), std::end(voiceNote), -1);
    std::fill(std::begin(noteCount), std::end(noteCount), 0);
}

SenderRtpMidi::~SenderRtpMidi()
{
}

//////////////////////////////////////////////////////////////////////////////
// session

void SenderRtpMidi::startSession(QString host, int port, QString name, int c, int id)
{
    stopSession();
    sessionId = id;

    setChannel(c);
    sessionName = name.isEmpty() ? QStringLiteral("MISUCO3") : name;
    responder = false;
    remoteControlPort = static_cast<quint16>(std::clamp(port, 1, 65534));
    remoteDataPort = remoteControlPort + 1;
    remoteName.clear();
    createTimers();

    host = host.trimmed();
    if(host.isEmpty()) {
        setState(Failed, QStringLiteral("No host"));
        return;
    }

    QHostAddress address;
    if(address.setAddress(host)) {
        QHostInfo info;
        info.setAddresses({address});
        onHostLookedUp(info);
    } else {
        setState(Resolving, QStringLiteral("Looking up ") + host);
        lookupId = QHostInfo::lookupHost(host, this, &SenderRtpMidi::onHostLookedUp);
    }
}

void SenderRtpMidi::startListening(int port, QString name, int c, int id)
{
    stopSession();
    sessionId = id;

    setChannel(c);
    sessionName = name.isEmpty() ? QStringLiteral("MISUCO3") : name;
    responder = true;
    remoteName.clear();
    createTimers();

    listenPort = static_cast<quint16>(std::clamp(port, 1, 65534));
    if(!openSockets(listenPort)) {
        setState(Failed, QStringLiteral("Port %1 or %2 is in use").arg(listenPort).arg(listenPort + 1));
        return;
    }
    ssrc = QRandomGenerator::global()->generate();
    sequenceNumber = static_cast<quint16>(QRandomGenerator::global()->generate());
    clock.start();
    listenAgain(QString());
}

void SenderRtpMidi::listenAgain(const QString &message)
{
    syncTimer->stop();
    remoteAddress.clear();
    remoteName.clear();
    inviteStep = InviteControl;
    setState(Listening, (message.isEmpty() ? QString() : message + QStringLiteral(" / "))
             + QStringLiteral("Waiting as %1 on port %2").arg(sessionName).arg(listenPort));
}

void SenderRtpMidi::stopSession()
{
    if(lookupId >= 0) {
        QHostInfo::abortHostLookup(lookupId);
        lookupId = -1;
    }
    if(state == Connected) {
        allNotesOff();
    }
    if(state == Connected || (state == Inviting && !responder)) {
        sendBye();
    }
    closeSockets();
    if(state != Off) {
        setState(Off);
    }
}

void SenderRtpMidi::setChannel(int c)
{
    // the channel is given as 1...16
    int newChannel = std::clamp(c, 1, 16) - 1;
    if(newChannel != channel && state == Connected) {
        allNotesOff();
    }
    channel = newChannel;
}

void SenderRtpMidi::createTimers()
{
    if(!inviteTimer) {
        inviteTimer = new QTimer(this);
        inviteTimer->setInterval(InviteInterval);
        connect(inviteTimer, &QTimer::timeout, this, &SenderRtpMidi::onInviteTimer);
        syncTimer = new QTimer(this);
        connect(syncTimer, &QTimer::timeout, this, &SenderRtpMidi::onSyncTimer);
    }
}

void SenderRtpMidi::onHostLookedUp(const QHostInfo &info)
{
    lookupId = -1;

    // the sockets are bound to ipv4
    const QList<QHostAddress> addresses = info.addresses();
    auto it = std::find_if(addresses.begin(), addresses.end(), [](const QHostAddress & a) {
        return a.protocol() == QAbstractSocket::IPv4Protocol;
    });
    if(it == addresses.end()) {
        setState(Failed, QStringLiteral("Host not found"));
        return;
    }
    remoteAddress = *it;

    if(!openSockets()) {
        setState(Failed, QStringLiteral("Cannot open the udp ports"));
        return;
    }

    initiatorToken = QRandomGenerator::global()->generate();
    ssrc = QRandomGenerator::global()->generate();
    sequenceNumber = static_cast<quint16>(QRandomGenerator::global()->generate());
    clock.start();

    inviteStep = InviteControl;
    inviteAttempts = 0;
    setState(Inviting, QStringLiteral("Inviting %1:%2").arg(remoteAddress.toString()).arg(remoteControlPort));
    sendInvitation();
    inviteTimer->start();
}

bool SenderRtpMidi::openSockets(quint16 port)
{
    closeSockets();

    // AppleMIDI expects the data port to be control port + 1,
    // port 0 takes any free pair of ports
    for(int attempt = 0; attempt < (port ? 1 : 10); attempt++) {
        auto control = new QUdpSocket(this);
        auto data = new QUdpSocket(this);
        if(control->bind(QHostAddress::AnyIPv4, port)
           && control->localPort() < 65535
           && data->bind(QHostAddress::AnyIPv4, control->localPort() + 1)) {
            controlSocket = control;
            dataSocket = data;
            connect(controlSocket, &QUdpSocket::readyRead, this, &SenderRtpMidi::onControlReadyRead);
            connect(dataSocket, &QUdpSocket::readyRead, this, &SenderRtpMidi::onDataReadyRead);
            qDebug() << "SenderRtpMidi local ports" << controlSocket->localPort() << dataSocket->localPort();
            return true;
        }
        delete control;
        delete data;
    }
    return false;
}

void SenderRtpMidi::closeSockets()
{
    if(inviteTimer) {
        inviteTimer->stop();
        syncTimer->stop();
    }
    delete controlSocket;
    controlSocket = nullptr;
    delete dataSocket;
    dataSocket = nullptr;
}

void SenderRtpMidi::setState(State s, const QString &message)
{
    state = s;
    QString text = message;
    if(text.isEmpty()) {
        switch(s) {
        case Off:        text = QStringLiteral("Off"); break;
        case Resolving:  text = QStringLiteral("Looking up host"); break;
        case Inviting:   text = QStringLiteral("Inviting"); break;
        case Connected:  text = QStringLiteral("Connected"); break;
        case Failed:     text = QStringLiteral("Failed"); break;
        case Listening:  text = QStringLiteral("Waiting for a connection"); break;
        }
    }
    qDebug() << "SenderRtpMidi state" << s << text;
    emit stateChanged(s, text, remoteName, sessionId);
}

qint64 SenderRtpMidi::now() const
{
    // AppleMIDI timestamps count in 100 microseconds
    return clock.isValid() ? clock.nsecsElapsed() / 100000 : 0;
}

void SenderRtpMidi::sendInvitation()
{
    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s << Signature << CmdInvitation << ProtocolVersion << initiatorToken << ssrc;
    QByteArray name = sessionName.toUtf8();
    s.writeRawData(name.constData(), name.size());
    s << quint8(0);

    if(inviteStep == InviteControl) {
        controlSocket->writeDatagram(packet, remoteAddress, remoteControlPort);
    } else {
        dataSocket->writeDatagram(packet, remoteAddress, remoteDataPort);
    }
}

void SenderRtpMidi::sendAccept(QUdpSocket *socket, const QHostAddress &to, quint16 port, quint32 token)
{
    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s << Signature << CmdAccept << ProtocolVersion << token << ssrc;
    QByteArray name = sessionName.toUtf8();
    s.writeRawData(name.constData(), name.size());
    s << quint8(0);
    socket->writeDatagram(packet, to, port);
}

void SenderRtpMidi::sendReject(QUdpSocket *socket, const QHostAddress &to, quint16 port, quint32 token)
{
    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s << Signature << CmdReject << ProtocolVersion << token << ssrc;
    socket->writeDatagram(packet, to, port);
}

void SenderRtpMidi::sendBye()
{
    if(!controlSocket || remoteAddress.isNull()) {
        return;
    }
    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s << Signature << CmdBye << ProtocolVersion << initiatorToken << ssrc;
    controlSocket->writeDatagram(packet, remoteAddress, remoteControlPort);
    controlSocket->flush();
}

void SenderRtpMidi::sendSync(quint8 count, qint64 ts1, qint64 ts2, qint64 ts3)
{
    if(!dataSocket) {
        return;
    }
    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s << Signature << CmdSync << ssrc << count << quint8(0) << quint8(0) << quint8(0)
      << quint64(ts1) << quint64(ts2) << quint64(ts3);
    dataSocket->writeDatagram(packet, remoteAddress, remoteDataPort);
}

void SenderRtpMidi::onInviteTimer()
{
    inviteAttempts++;
    if(inviteAttempts >= InviteMaxAttempts) {
        closeSockets();
        setState(Failed, QStringLiteral("No answer from %1:%2").arg(remoteAddress.toString()).arg(remoteControlPort));
        return;
    }
    sendInvitation();
}

void SenderRtpMidi::onSyncTimer()
{
    sendSync(0, now(), 0, 0);
    syncCount++;
    if(syncCount == SyncCountStart) {
        syncTimer->setInterval(SyncInterval);
    }
}

void SenderRtpMidi::onControlReadyRead()
{
    while(controlSocket && controlSocket->hasPendingDatagrams()) {
        QNetworkDatagram d = controlSocket->receiveDatagram();
        handleSessionPacket(d.data(), false, d.senderAddress(), static_cast<quint16>(d.senderPort()));
    }
}

void SenderRtpMidi::onDataReadyRead()
{
    while(dataSocket && dataSocket->hasPendingDatagrams()) {
        QNetworkDatagram d = dataSocket->receiveDatagram();
        // midi sent by the remote side is not used
        handleSessionPacket(d.data(), true, d.senderAddress(), static_cast<quint16>(d.senderPort()));
    }
}

void SenderRtpMidi::handleSessionPacket(const QByteArray &packet, bool dataPort,
                                        const QHostAddress &fromAddress, quint16 fromPort)
{
    if(packet.size() < 4) {
        return;
    }
    QDataStream s(packet);
    quint16 signature, command;
    s >> signature >> command;
    if(signature != Signature) {
        return;
    }
    QHostAddress from(fromAddress.toIPv4Address());
    QString remoteLabel = remoteName.isEmpty() ? remoteAddress.toString() : remoteName;

    switch(command) {
    case CmdInvitation: {
        // responder: the remote side invites, first on the control port,
        // then on the data port
        quint32 version, token, remoteSsrc;
        s >> version >> token >> remoteSsrc;
        if(s.status() != QDataStream::Ok || !responder) {
            return;
        }
        QUdpSocket * socket = dataPort ? dataSocket : controlSocket;
        bool samePeer = from == remoteAddress && token == initiatorToken;

        if(!dataPort && (state == Listening || samePeer)) {
            remoteAddress = from;
            remoteControlPort = fromPort;
            initiatorToken = token;
            remoteName = QString::fromUtf8(packet.mid(16)).section(QChar(0), 0, 0);
            if(state != Connected) {
                inviteStep = InviteData;
            }
            sendAccept(socket, from, fromPort, token);
        } else if(dataPort && samePeer && (inviteStep == InviteData || state == Connected)) {
            remoteDataPort = fromPort;
            sendAccept(socket, from, fromPort, token);
            if(state != Connected) {
                lastBend = -1;
                syncCount = 0;
                setState(Connected, QStringLiteral("Connected to ") +
                         (remoteName.isEmpty() ? remoteAddress.toString() : remoteName));
            }
        } else {
            // one session at a time
            sendReject(socket, from, fromPort, token);
        }
        break;
    }

    case CmdAccept:
    case CmdReject: {
        quint32 version, token, remoteSsrc;
        s >> version >> token >> remoteSsrc;
        if(s.status() != QDataStream::Ok || token != initiatorToken || state != Inviting || responder) {
            return;
        }
        if(command == CmdReject) {
            closeSockets();
            setState(Failed, QStringLiteral("Rejected by %1").arg(remoteAddress.toString()));
            return;
        }
        if(!dataPort && inviteStep == InviteControl) {
            inviteStep = InviteData;
            inviteAttempts = 0;
            sendInvitation();
            inviteTimer->start();
        } else if(dataPort && inviteStep == InviteData) {
            inviteTimer->stop();
            remoteName = QString::fromUtf8(packet.mid(16)).section(QChar(0), 0, 0);
            lastBend = -1;
            setState(Connected, QStringLiteral("Connected to ") +
                     (remoteName.isEmpty() ? remoteAddress.toString() : remoteName));
            syncCount = 0;
            syncTimer->setInterval(SyncIntervalStart);
            syncTimer->start();
            onSyncTimer();
        }
        break;
    }

    case CmdBye:
        if(from != remoteAddress) {
            return;
        }
        if(responder) {
            if(state == Connected || inviteStep == InviteData) {
                // the notes can not be sent any more
                std::fill(std::begin(voiceNote), std::end(voiceNote), -1);
                std::fill(std::begin(noteCount), std::end(noteCount), 0);
                listenAgain(QStringLiteral("Closed by ") + remoteLabel);
            }
        } else if(state == Connected || state == Inviting) {
            closeSockets();
            setState(Failed, QStringLiteral("Session closed by ") + remoteLabel);
        }
        break;

    case CmdSync: {
        if(!dataPort || state != Connected || from != remoteAddress) {
            return;
        }
        quint32 remoteSsrc;
        quint8 count, pad;
        quint64 ts1, ts2, ts3;
        s >> remoteSsrc >> count >> pad >> pad >> pad >> ts1 >> ts2 >> ts3;
        if(s.status() != QDataStream::Ok) {
            return;
        }
        if(count == 0) {
            // the remote side starts a sync
            sendSync(1, ts1, now(), 0);
        } else if(count == 1) {
            qint64 t = now();
            sendSync(2, ts1, ts2, t);
            emit latencyChanged((t - static_cast<qint64>(ts1)) / 20.0);   // half round trip in ms
        } else if(count == 2) {
            // the round trip measured by the remote side, in its clock
            qint64 roundTrip = static_cast<qint64>(ts3) - static_cast<qint64>(ts1);
            if(roundTrip >= 0) {
                emit latencyChanged(roundTrip / 20.0);
            }
        }
        break;
    }

    default:
        // RS (receiver feedback) is not needed
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////
// midi

void SenderRtpMidi::sendMidi(const QList<QByteArray> &commands)
{
    if(state != Connected || !dataSocket || commands.isEmpty()) {
        return;
    }

    // midi list: the first command has no delta time, the following
    // commands are preceded by a delta time of 0
    QByteArray list;
    for(int i = 0; i < commands.size(); i++) {
        if(i > 0) {
            list.append(char(0));
        }
        list.append(commands[i]);
    }

    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s << RtpVersion << RtpPayloadType << sequenceNumber++
      << static_cast<quint32>(now()) << ssrc;

    // midi command section header without journal
    if(list.size() <= 15) {
        s << quint8(list.size());
    } else {
        s << quint8(0x80 | ((list.size() >> 8) & 0x0F)) << quint8(list.size() & 0xFF);
    }
    s.writeRawData(list.constData(), list.size());

    dataSocket->writeDatagram(packet, remoteAddress, remoteDataPort);
}

QByteArray SenderRtpMidi::bendCommand(int pitch)
{
    // misuco pitch is in cent (100 per semitone), the 14 bit pitch bend
    // covers +-2 semitones (general midi default), 4096 per semitone
    int bend = std::clamp(8192 + pitch * 4096 / 100, 0, 16383);
    if(bend == lastBend) {
        return QByteArray();
    }
    lastBend = bend;
    QByteArray c;
    c.append(char(0xE0 | channel));
    c.append(char(bend & 0x7F));
    c.append(char((bend >> 7) & 0x7F));
    return c;
}

void SenderRtpMidi::noteOn(int voiceId, float, int midinote, int pitch, int v)
{
    if(midinote < 0 || midinote > 127) {
        return;
    }
    int voice = voiceId & 127;

    QList<QByteArray> commands;
    QByteArray bend = bendCommand(pitch);
    if(!bend.isEmpty()) {
        commands.append(bend);
    }

    QByteArray c;
    c.append(char(0x90 | channel));
    c.append(char(midinote));
    c.append(char(std::clamp(v, 1, 127)));
    commands.append(c);
    sendMidi(commands);

    voiceNote[voice] = midinote;
    noteCount[midinote]++;
}

void SenderRtpMidi::noteOff(int voiceId)
{
    int voice = voiceId & 127;
    int note = voiceNote[voice];
    if(note < 0) {
        return;
    }
    voiceNote[voice] = -1;

    // a note that is played by several voices is released by the last one
    noteCount[note] = std::max(noteCount[note] - 1, 0);
    if(noteCount[note] > 0) {
        return;
    }

    QByteArray c;
    c.append(char(0x80 | channel));
    c.append(char(note));
    c.append(char(0));
    sendMidi({c});
}

void SenderRtpMidi::pitch(int voiceId, float f, int midinote, int pitch)
{
    int voice = voiceId & 127;
    if(voiceNote[voice] < 0) {
        return;
    }
    if(voiceNote[voice] != midinote) {
        noteOff(voiceId);
        noteOn(voiceId, f, midinote, pitch, 127);
    } else {
        QByteArray bend = bendCommand(pitch);
        if(!bend.isEmpty()) {
            sendMidi({bend});
        }
    }
}

void SenderRtpMidi::cc(int, int cc, float value)
{
    // midi controllers are per channel
    ccAllVoices(cc, value);
}

void SenderRtpMidi::ccAllVoices(int cc, float value)
{
    if(cc < 0 || cc > 119) {
        return;
    }
    QByteArray c;
    c.append(char(0xB0 | channel));
    c.append(char(cc));
    c.append(char(static_cast<int>(127 * std::clamp(value, 0.0f, 1.0f))));
    sendMidi({c});
}

void SenderRtpMidi::pc(int value)
{
    QByteArray c;
    c.append(char(0xC0 | channel));
    c.append(char(std::clamp(value, 0, 127)));
    sendMidi({c});
}

void SenderRtpMidi::allNotesOff()
{
    QList<QByteArray> commands;
    QByteArray c;
    c.append(char(0xB0 | channel));
    c.append(char(123));    // all notes off
    c.append(char(0));
    commands.append(c);
    lastBend = -1;
    commands.append(bendCommand(0));
    sendMidi(commands);

    std::fill(std::begin(voiceNote), std::end(voiceNote), -1);
    std::fill(std::begin(noteCount), std::end(noteCount), 0);
}
