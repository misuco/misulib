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

#ifndef BONJOURANNOUNCER_H
#define BONJOURANNOUNCER_H

// Announces a service with DNS-SD (Bonjour), e.g. an "_apple-midi._udp"
// session, so that it shows up in the directory of other devices.
// The service of the operating system is used:
//   Android:     NsdManager (android/src/org/misuco/misuco3/NsdAnnouncer.java)
//   Windows:     DnsServiceRegister (Windows 10 1809 and later)
//   macOS / iOS: dns_sd
// On the other platforms announce() returns false.

#include <QObject>
#include <QString>

class BonjourAnnouncer : public QObject
{
    Q_OBJECT

public:
    explicit BonjourAnnouncer(QObject * parent = nullptr);
    ~BonjourAnnouncer();

    // serviceType like "_apple-midi._udp"
    bool announce(const QString & name, const QString & serviceType, quint16 port);
    void withdraw();
    bool announced() const { return m_announced; }

    // platform specific data
    struct Platform;

private:
    bool m_announced{false};
    Platform * d{nullptr};
};

#endif // BONJOURANNOUNCER_H
