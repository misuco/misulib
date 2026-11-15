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

#ifndef OSCCONTROL_H
#define OSCCONTROL_H

// The QML side of the OSC sender. It lives in the gui thread, keeps the
// settings and passes the commands to the SenderOsc in its sender thread.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

class MasterSender;
class SenderOsc;

class OscControl : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot instantiate OscControl")

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int format READ format WRITE setFormat NOTIFY formatChanged)
    Q_PROPERTY(QStringList formatNames READ formatNames CONSTANT)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool tcp READ tcp WRITE setTcp NOTIFY tcpChanged)
    Q_PROPERTY(int channel READ channel WRITE setChannel NOTIFY channelChanged)
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)

public:
    // the name of the sender in the MasterSender
    static constexpr const char * SenderName = "Osc";

    explicit OscControl(MasterSender * masterSender, SenderOsc * sender, QObject * parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v);
    // SenderOsc::Format
    int format() const { return m_format; }
    void setFormat(int v);
    QStringList formatNames() const;
    QString host() const { return m_host; }
    void setHost(const QString & v);
    int port() const { return m_port; }
    void setPort(int v);
    bool tcp() const { return m_tcp; }
    void setTcp(bool v);
    int channel() const { return m_channel; }
    void setChannel(int v);
    int state() const { return m_state; }
    QString statusText() const { return m_statusText; }

    // the port the receiver of a format usually listens on
    Q_INVOKABLE int defaultPort(int format) const;

signals:
    void enabledChanged();
    void formatChanged();
    void hostChanged();
    void portChanged();
    void tcpChanged();
    void channelChanged();
    void stateChanged();

private slots:
    void onSenderStateChanged(int state, QString message);
    void onAboutToQuit();

private:
    MasterSender * m_masterSender;
    SenderOsc * m_sender;

    bool m_enabled{false};
    int m_format{0};
    QString m_host;
    int m_port{3150};
    bool m_tcp{false};
    int m_channel{1};
    int m_state{0};
    QString m_statusText;

    void switchOff();
    void saveSettings();
};

#endif // OSCCONTROL_H
