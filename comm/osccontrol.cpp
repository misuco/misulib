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

#include "osccontrol.h"
#include "mastersender.h"
#include "senderosc.h"

#include <QCoreApplication>
#include <QSettings>
#include <algorithm>

OscControl::OscControl(MasterSender *masterSender, SenderOsc *sender, QObject *parent)
    : QObject(parent), m_masterSender(masterSender), m_sender(sender)
{
    QSettings s(QStringLiteral("misuco"), QStringLiteral("misuco3"));
    m_format = std::clamp(s.value(QStringLiteral("osc/format"), int(SenderOsc::MidiGeneric)).toInt(),
                          int(SenderOsc::MidiGeneric), int(SenderOsc::SuperCollider));
    // the broadcast address reaches all devices of the network
    m_host = s.value(QStringLiteral("osc/host"), QStringLiteral("255.255.255.255")).toString();
    m_port = std::clamp(s.value(QStringLiteral("osc/port"), defaultPort(m_format)).toInt(), 1, 65535);
    m_tcp = s.value(QStringLiteral("osc/tcp"), false).toBool();
    m_channel = std::clamp(s.value(QStringLiteral("osc/channel"), 1).toInt(), 1, 16);
    m_statusText = QStringLiteral("Off");

    // the sender lives in its own thread, the signals are queued
    connect(m_sender, &SenderOsc::stateChanged, this, &OscControl::onSenderStateChanged);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &OscControl::onAboutToQuit);

    // the sender is switched on again if it has been on
    if(s.value(QStringLiteral("osc/enabled"), false).toBool()) {
        setEnabled(true);
    }
}

QStringList OscControl::formatNames() const
{
    // in the order of SenderOsc::Format
    return {QStringLiteral("MIDI generic"), QStringLiteral("Reaktor"), QStringLiteral("SuperCollider")};
}

int OscControl::defaultPort(int format) const
{
    // 57110 is the port of the SuperCollider synth server
    return format == SenderOsc::SuperCollider ? 57110 : 3150;
}

void OscControl::setEnabled(bool v)
{
    if(v == m_enabled) {
        return;
    }
    if(!v) {
        switchOff();
        QMetaObject::invokeMethod(m_sender, "stop", Qt::QueuedConnection);
        saveSettings();
        return;
    }

    m_enabled = true;
    emit enabledChanged();
    saveSettings();
    m_masterSender->onToggleSender(SenderName, true);
    QMetaObject::invokeMethod(m_sender, "start", Qt::QueuedConnection,
                              Q_ARG(QString, m_host), Q_ARG(int, m_port), Q_ARG(bool, m_tcp),
                              Q_ARG(int, m_format), Q_ARG(int, m_channel));
}

void OscControl::switchOff()
{
    m_masterSender->onToggleSender(SenderName, false);
    if(m_enabled) {
        m_enabled = false;
        emit enabledChanged();
    }
}

void OscControl::setFormat(int v)
{
    v = std::clamp(v, int(SenderOsc::MidiGeneric), int(SenderOsc::SuperCollider));
    if(v == m_format) {
        return;
    }
    m_format = v;
    saveSettings();
    emit formatChanged();

    // the format can be changed while the sender is on
    QMetaObject::invokeMethod(m_sender, "setFormat", Qt::QueuedConnection, Q_ARG(int, m_format));
}

void OscControl::setHost(const QString &v)
{
    if(v == m_host || m_enabled) {
        return;
    }
    m_host = v;
    saveSettings();
    emit hostChanged();
}

void OscControl::setPort(int v)
{
    v = std::clamp(v, 1, 65535);
    if(v == m_port || m_enabled) {
        return;
    }
    m_port = v;
    saveSettings();
    emit portChanged();
}

void OscControl::setTcp(bool v)
{
    if(v == m_tcp || m_enabled) {
        return;
    }
    m_tcp = v;
    saveSettings();
    emit tcpChanged();
}

void OscControl::setChannel(int v)
{
    v = std::clamp(v, 1, 16);
    if(v == m_channel) {
        return;
    }
    m_channel = v;
    saveSettings();
    emit channelChanged();

    // the channel can be changed while the sender is on
    QMetaObject::invokeMethod(m_sender, "setChannel", Qt::QueuedConnection, Q_ARG(int, m_channel));
}

void OscControl::onSenderStateChanged(int state, QString message)
{
    // the "Off" of the stop at the begin of a start
    if(m_enabled && state == SenderOsc::Off) {
        return;
    }
    m_state = state;
    m_statusText = message;
    emit stateChanged();

    // a failed start switches the sender off, the message stays
    if(m_enabled && state == SenderOsc::Failed) {
        switchOff();
        saveSettings();
    }
}

void OscControl::onAboutToQuit()
{
    // the notes are switched off before the sender thread stops, the
    // sender stays enabled in the settings
    if(m_enabled) {
        m_masterSender->onToggleSender(SenderName, false);
        QMetaObject::invokeMethod(m_sender, "stop", Qt::BlockingQueuedConnection);
    }
}

void OscControl::saveSettings()
{
    QSettings s(QStringLiteral("misuco"), QStringLiteral("misuco3"));
    s.setValue(QStringLiteral("osc/enabled"), m_enabled);
    s.setValue(QStringLiteral("osc/format"), m_format);
    s.setValue(QStringLiteral("osc/host"), m_host);
    s.setValue(QStringLiteral("osc/port"), m_port);
    s.setValue(QStringLiteral("osc/tcp"), m_tcp);
    s.setValue(QStringLiteral("osc/channel"), m_channel);
}
