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

#ifndef SENDEROSC_H
#define SENDEROSC_H

// Sends the notes as OSC messages over UDP or TCP, in one of three
// formats:
//
// MidiGeneric    /note/<ch> note velocity ch, /pitch/<ch> bend,
//                /cc/<ch>/<cc> value cc ch, /pc/<ch> program ch
// Reaktor        /note/<ch> note velocity, /pitch/<ch> bend,
//                /cc/<ch>/<cc> value, /pc/<ch> program
// SuperCollider  /s_new and /n_set for the synth server, one node for
//                each voice with the frequency of the note
//
// It lives in a sender thread of the MasterSender. The session is
// controlled by the OscControl in the gui thread with queued calls of
// the slots.

#include <QObject>
#include <QString>
#include <QVariant>

class QOscClientInterface;
class QOscTcpClient;
class QTimer;

class SenderOsc : public QObject
{
    Q_OBJECT

public:
    enum Format {
        MidiGeneric = 0,
        Reaktor,
        SuperCollider
    };

    enum State {
        Off = 0,
        Sending,        // udp, or tcp connected
        Connecting,     // tcp, not connected yet
        Failed
    };

    explicit SenderOsc(QObject * parent = nullptr);
    ~SenderOsc();

public slots:
    // the session
    void start(QString host, int port, bool tcp, int format, int channel);
    void stop();
    void setFormat(int format);
    void setChannel(int channel);

    // from the MasterSender
    void cc(int voiceId, int cc, float value);
    void ccAllVoices(int cc, float value);
    void pc(int v1);
    void noteOn(int voiceId, float f, int midinote, int pitch, int v);
    void noteOff(int voiceId);
    void pitch(int voiceId, float f, int midinote, int pitch);

signals:
    void stateChanged(int state, QString message);

private slots:
    void checkConnection();

private:
    static const int Voices = 1024;

    QOscClientInterface * client{nullptr};
    QOscTcpClient * tcpClient{nullptr};     // the client, if it is a tcp client
    QObject * clientObject{nullptr};
    QTimer * checkTimer{nullptr};

    QString destination;
    int state{Off};
    int format{MidiGeneric};
    int channel{1};
    QString synthName{QStringLiteral("m0")};    // the SuperCollider synth

    int voiceNote[Voices];      // the note of a voice, -1 if it does not sound
    int voiceIds[Voices];       // the id of the voice, the SuperCollider node
    int voiceChannel[Voices];   // the channel the note has been sent on
    int voiceVelocity[Voices];
    int ccState[128];

    void send(const QString & path, const QVariantList & values);
    void sendBend(int pitch);
    void allNotesOff();
    void setState(int s, const QString & message);
};

#endif // SENDEROSC_H
