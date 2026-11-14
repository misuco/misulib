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

#include "rtpmidicontrol.h"
#include "mastersender.h"
#include "senderrtpmidi.h"
#include "mdnsbrowser.h"
#include "bonjourannouncer.h"

#include <QCoreApplication>
#include <QNetworkInterface>
#include <QSettings>
#include <algorithm>

RtpMidiControl::RtpMidiControl(MasterSender *masterSender, SenderRtpMidi *sender, QObject *parent)
    : QObject(parent), m_masterSender(masterSender), m_sender(sender)
{
    QSettings s(QStringLiteral("misuco"), QStringLiteral("misuco3"));
    m_mode = std::clamp(s.value(QStringLiteral("rtpmidi/mode"), int(ConnectTo)).toInt(), 0, 1);
    m_host = s.value(QStringLiteral("rtpmidi/host"), QString()).toString();
    m_port = s.value(QStringLiteral("rtpmidi/port"), 5004).toInt();
    m_name = s.value(QStringLiteral("rtpmidi/name"), QStringLiteral("MISUCO3")).toString();
    m_channel = std::clamp(s.value(QStringLiteral("rtpmidi/channel"), 1).toInt(), 1, 16);
    m_statusText = QStringLiteral("Off");

    m_browser = new MdnsBrowser(this);
    connect(m_browser, &MdnsBrowser::servicesChanged, this, &RtpMidiControl::onServicesChanged);
    connect(m_browser, &MdnsBrowser::finished, this, &RtpMidiControl::searchingChanged);

    m_announcer = new BonjourAnnouncer(this);

    // the sender lives in its own thread, the signals are queued
    connect(m_sender, &SenderRtpMidi::stateChanged, this, &RtpMidiControl::onSenderStateChanged);
    connect(m_sender, &SenderRtpMidi::latencyChanged, this, &RtpMidiControl::onSenderLatencyChanged);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &RtpMidiControl::onAboutToQuit);

    updateLocalAddresses();
}

RtpMidiControl::~RtpMidiControl()
{
}

void RtpMidiControl::setEnabled(bool v)
{
    if(v == m_enabled) {
        return;
    }
    if(!v) {
        switchOff();
        QMetaObject::invokeMethod(m_sender, "stopSession", Qt::QueuedConnection);
        return;
    }

    m_enabled = true;
    emit enabledChanged();
    m_latency = -1;
    emit latencyChanged();
    updateLocalAddresses();
    m_sessionId++;
    m_masterSender->onToggleSender(SenderName, true);

    if(m_mode == WaitFor) {
        // the session is announced when the ports are open
        QMetaObject::invokeMethod(m_sender, "startListening", Qt::QueuedConnection,
                                  Q_ARG(int, m_port), Q_ARG(QString, m_name), Q_ARG(int, m_channel),
                                  Q_ARG(int, m_sessionId));
    } else {
        QMetaObject::invokeMethod(m_sender, "startSession", Qt::QueuedConnection,
                                  Q_ARG(QString, m_host), Q_ARG(int, m_port),
                                  Q_ARG(QString, m_name), Q_ARG(int, m_channel), Q_ARG(int, m_sessionId));
    }
}

void RtpMidiControl::switchOff()
{
    m_masterSender->onToggleSender(SenderName, false);
    if(m_announcer->announced()) {
        m_announcer->withdraw();
        emit announcedChanged();
    }
    if(m_enabled) {
        m_enabled = false;
        emit enabledChanged();
    }
}

void RtpMidiControl::setMode(int v)
{
    v = std::clamp(v, 0, 1);
    if(v == m_mode || m_enabled) {
        return;
    }
    m_mode = v;
    saveSettings();
    emit modeChanged();
}

void RtpMidiControl::setHost(const QString &v)
{
    if(v == m_host) {
        return;
    }
    m_host = v;
    saveSettings();
    emit hostChanged();
}

void RtpMidiControl::setPort(int v)
{
    v = std::clamp(v, 1, 65534);
    if(v == m_port) {
        return;
    }
    m_port = v;
    saveSettings();
    emit portChanged();
}

void RtpMidiControl::setName(const QString &v)
{
    if(v == m_name) {
        return;
    }
    m_name = v;
    saveSettings();
    emit nameChanged();
}

void RtpMidiControl::setChannel(int v)
{
    v = std::clamp(v, 1, 16);
    if(v == m_channel) {
        return;
    }
    m_channel = v;
    saveSettings();
    emit channelChanged();

    // the channel can be changed while the session is running
    QMetaObject::invokeMethod(m_sender, "setChannel", Qt::QueuedConnection, Q_ARG(int, m_channel));
}

bool RtpMidiControl::searching() const
{
    return m_browser->running();
}

bool RtpMidiControl::announced() const
{
    return m_announcer->announced();
}

void RtpMidiControl::search()
{
    m_browser->start(QString::fromLatin1(ServiceType));
    emit searchingChanged();
}

void RtpMidiControl::onServicesChanged()
{
    QStringList list;
    const auto services = m_browser->services();
    for(const MdnsBrowser::Service & s : services) {
        list.append(QStringLiteral("%1 (%2:%3)").arg(s.name, s.address.toString()).arg(s.port));
    }
    if(list != m_sessions) {
        m_sessions = list;
        emit sessionsChanged();
    }
}

void RtpMidiControl::selectSession(int index)
{
    const auto services = m_browser->services();
    if(index < 0 || index >= services.size()) {
        return;
    }
    setHost(services[index].address.toString());
    setPort(services[index].port);
}

void RtpMidiControl::updateLocalAddresses()
{
    QStringList list;
    const auto addresses = QNetworkInterface::allAddresses();
    for(const QHostAddress & a : addresses) {
        if(a.protocol() == QAbstractSocket::IPv4Protocol && !a.isLoopback() && !a.isLinkLocal()) {
            list.append(a.toString());
        }
    }
    QString text = list.isEmpty() ? QStringLiteral("no network") : list.join(QStringLiteral(", "));
    if(text != m_localAddresses) {
        m_localAddresses = text;
        emit localAddressesChanged();
    }
}

void RtpMidiControl::onSenderStateChanged(int state, QString message, QString remoteName, int sessionId)
{
    // the state of a session that has been replaced by a new one
    if(sessionId != m_sessionId) {
        return;
    }

    m_state = state;
    m_statusText = message;
    m_remoteName = remoteName;

    if(state == SenderRtpMidi::Listening && m_enabled && !m_announcer->announced()) {
        // the ports are open, the session can be found now
        QString name = m_name.isEmpty() ? QStringLiteral("MISUCO3") : m_name;
        if(m_announcer->announce(name, QString::fromLatin1(ServiceType), static_cast<quint16>(m_port))) {
            emit announcedChanged();
        } else {
            m_statusText += QStringLiteral(" (not announced, connect to ") + m_localAddresses + QChar(')');
        }
    }
    emit stateChanged();

    // a failed or closed session switches the sender off, the message stays
    if(m_enabled && (state == SenderRtpMidi::Failed || state == SenderRtpMidi::Off)) {
        switchOff();
    }
}

void RtpMidiControl::onSenderLatencyChanged(double ms)
{
    m_latency = ms;
    emit latencyChanged();
}

void RtpMidiControl::onAboutToQuit()
{
    // say goodbye to the remote side before the sender thread stops
    m_browser->stop();
    if(m_enabled) {
        switchOff();
        QMetaObject::invokeMethod(m_sender, "stopSession", Qt::BlockingQueuedConnection);
    }
}

void RtpMidiControl::saveSettings()
{
    QSettings s(QStringLiteral("misuco"), QStringLiteral("misuco3"));
    s.setValue(QStringLiteral("rtpmidi/mode"), m_mode);
    s.setValue(QStringLiteral("rtpmidi/host"), m_host);
    s.setValue(QStringLiteral("rtpmidi/port"), m_port);
    s.setValue(QStringLiteral("rtpmidi/name"), m_name);
    s.setValue(QStringLiteral("rtpmidi/channel"), m_channel);
}
