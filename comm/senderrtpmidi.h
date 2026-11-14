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

#ifndef SENDERRTPMIDI_H
#define SENDERRTPMIDI_H

// MIDI over ethernet (RTP-MIDI, RFC 6295) with the AppleMIDI session
// protocol. Two modes:
// - initiator: the sender invites a session that is open on the remote
//   side, the session listens on a control port (default 5004) and on
//   the data port (control port + 1).
//     macOS:   Audio MIDI Setup / MIDI Studio / Network
//     Windows: rtpMIDI (Tobias Erichsen)
// - responder: the sender waits on a port pair for an invitation, the
//   session is announced with Bonjour by RtpMidiControl and shows up in
//   the directory of the remote side.
//
// The sender lives in its own thread (see MasterSender::addSenderThread),
// the session slots must be invoked queued.

#include <QObject>
#include <QHostAddress>
#include <QElapsedTimer>
#include <QByteArray>
#include <QString>

class QUdpSocket;
class QTimer;
class QHostInfo;

class SenderRtpMidi : public QObject
{
    Q_OBJECT

public:
    enum State {
        Off = 0,
        Resolving,
        Inviting,
        Connected,
        Failed,
        Listening       // waiting for an invitation
    };
    Q_ENUM(State)

    explicit SenderRtpMidi(QObject * parent = nullptr);
    ~SenderRtpMidi();

public slots:
    // MasterSender interface
    void cc(int voiceId, int cc, float value);
    void ccAllVoices(int cc, float value);
    void pc(int value);
    void noteOn(int voiceId, float f, int midinote, int pitch, int v);
    void noteOff(int voiceId);
    void pitch(int voiceId, float f, int midinote, int pitch);

    // session
    // initiator: invite the session open on host:port
    void startSession(QString host, int port, QString name, int channel, int sessionId);
    // responder: wait on port for an invitation
    void startListening(int port, QString name, int channel, int sessionId);
    void stopSession();
    void setChannel(int channel);

signals:
    // sessionId is the id given at the start of the session, state changes
    // of a previous session can be told apart
    void stateChanged(int state, QString message, QString remoteName, int sessionId);
    void latencyChanged(double ms);

private slots:
    void onHostLookedUp(const QHostInfo & info);
    void onControlReadyRead();
    void onDataReadyRead();
    void onInviteTimer();
    void onSyncTimer();

private:
    enum InviteStep { InviteControl, InviteData };

    QUdpSocket * controlSocket{nullptr};
    QUdpSocket * dataSocket{nullptr};
    QTimer * inviteTimer{nullptr};
    QTimer * syncTimer{nullptr};
    QElapsedTimer clock;

    State state{Off};
    InviteStep inviteStep{InviteControl};
    int inviteAttempts{0};
    int syncCount{0};
    int lookupId{-1};

    QHostAddress remoteAddress;
    quint16 remoteControlPort{5004};
    quint16 remoteDataPort{5005};
    int sessionId{0};
    bool responder{false};          // the remote side invited
    quint16 listenPort{0};          // responder: the local control port
    QString sessionName;
    QString remoteName;
    quint32 initiatorToken{0};
    quint32 ssrc{0};
    quint16 sequenceNumber{0};
    int channel{0};     // 0...15

    int voiceNote[128];     // voiceId -> midi note, -1 if off
    int noteCount[128];     // number of voices that play a midi note
    int lastBend{-1};       // last sent pitch bend, -1 if none

    void setState(State s, const QString & message = QString());
    bool openSockets(quint16 port = 0);
    void closeSockets();
    void sendInvitation();
    void sendAccept(QUdpSocket * socket, const QHostAddress & to, quint16 port, quint32 token);
    void sendReject(QUdpSocket * socket, const QHostAddress & to, quint16 port, quint32 token);
    void listenAgain(const QString & message);
    void sendBye();
    void sendSync(quint8 count, qint64 ts1, qint64 ts2, qint64 ts3);
    void handleSessionPacket(const QByteArray & packet, bool dataPort, const QHostAddress & from, quint16 fromPort);
    void createTimers();
    qint64 now() const;

    void sendMidi(const QList<QByteArray> & commands);
    QByteArray bendCommand(int pitch);
    void allNotesOff();
};

#endif // SENDERRTPMIDI_H
