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

#ifndef RTPMIDICONTROL_H
#define RTPMIDICONTROL_H

// The QML side of the network midi sender. It lives in the gui thread,
// keeps the settings, searches for sessions, announces the own session
// and passes the session commands to the SenderRtpMidi in its sender
// thread.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

class MasterSender;
class SenderRtpMidi;
class MdnsBrowser;
class BonjourAnnouncer;

class RtpMidiControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot instantiate RtpMidiControl")

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int channel READ channel WRITE setChannel NOTIFY channelChanged)
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString remoteName READ remoteName NOTIFY stateChanged)
    Q_PROPERTY(double latency READ latency NOTIFY latencyChanged)
    Q_PROPERTY(QString localAddresses READ localAddresses NOTIFY localAddressesChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QStringList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(bool announced READ announced NOTIFY announcedChanged)

public:
    enum Mode {
        ConnectTo = 0,      // invite a session on host:port
        WaitFor = 1         // announce the own session and wait
    };
    Q_ENUM(Mode)

    // the name of the sender in the MasterSender
    static constexpr const char * SenderName = "RtpMidi";
    static constexpr const char * ServiceType = "_apple-midi._udp";

    explicit RtpMidiControl(MasterSender * masterSender, SenderRtpMidi * sender, QObject * parent = nullptr);
    ~RtpMidiControl();

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v);
    int mode() const { return m_mode; }
    void setMode(int v);
    QString host() const { return m_host; }
    void setHost(const QString & v);
    int port() const { return m_port; }
    void setPort(int v);
    QString name() const { return m_name; }
    void setName(const QString & v);
    int channel() const { return m_channel; }
    void setChannel(int v);
    int state() const { return m_state; }
    QString statusText() const { return m_statusText; }
    QString remoteName() const { return m_remoteName; }
    double latency() const { return m_latency; }
    QString localAddresses() const { return m_localAddresses; }
    bool searching() const;
    QStringList sessions() const { return m_sessions; }
    bool announced() const;

    Q_INVOKABLE void updateLocalAddresses();
    // search the network for open sessions
    Q_INVOKABLE void search();
    // take host and port of a found session
    Q_INVOKABLE void selectSession(int index);

signals:
    void enabledChanged();
    void modeChanged();
    void hostChanged();
    void portChanged();
    void nameChanged();
    void channelChanged();
    void stateChanged();
    void latencyChanged();
    void localAddressesChanged();
    void searchingChanged();
    void sessionsChanged();
    void announcedChanged();

private slots:
    void onSenderStateChanged(int state, QString message, QString remoteName, int sessionId);
    void onSenderLatencyChanged(double ms);
    void onServicesChanged();
    void onAboutToQuit();

private:
    MasterSender * m_masterSender;
    SenderRtpMidi * m_sender;
    MdnsBrowser * m_browser;
    BonjourAnnouncer * m_announcer;

    bool m_enabled{false};
    int m_sessionId{0};     // counts the started sessions
    int m_mode{ConnectTo};
    QString m_host;
    int m_port{5004};
    QString m_name;
    int m_channel{1};
    int m_state{0};
    QString m_statusText;
    QString m_remoteName;
    double m_latency{-1};
    QString m_localAddresses;
    QStringList m_sessions;

    void switchOff();
    void saveSettings();
};

#endif // RTPMIDICONTROL_H
