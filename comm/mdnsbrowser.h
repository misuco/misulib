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

#ifndef MDNSBROWSER_H
#define MDNSBROWSER_H

// A minimal mDNS / DNS-SD browser (RFC 6762, RFC 6763) that finds the
// instances of a service type, e.g. "_apple-midi._udp".
//
// The queries are sent from an ephemeral port to the mDNS multicast group.
// Such "legacy unicast" queries are answered by unicast to the asking
// socket (macOS, Bonjour on Windows). Other responders (the mDNS service
// of Windows 10/11, many devices) answer by multicast only, those answers
// are received on port 5353, shared with the system mDNS service.
// On Android a multicast lock is held during the search.

#include <QObject>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

class QUdpSocket;
class QTimer;

class MdnsBrowser : public QObject
{
    Q_OBJECT

public:
    struct Service {
        QString name;           // instance name, e.g. "MacBook Session"
        QHostAddress address;
        quint16 port{0};
    };

    explicit MdnsBrowser(QObject * parent = nullptr);
    ~MdnsBrowser();

    // browse for a service type like "_apple-midi._udp"
    void start(const QString & serviceType, int durationMs = 2500);
    void stop();
    bool running() const;
    QList<Service> services() const { return m_services; }

signals:
    void servicesChanged();
    void finished();

private slots:
    void onReadyRead();
    void onRepeat();

private:
    struct SrvRecord {
        quint16 port{0};
        QString target;
    };

    QUdpSocket * socket{nullptr};       // legacy unicast queries and answers
    QUdpSocket * mcastSocket{nullptr};  // multicast answers on port 5353
    QTimer * repeatTimer{nullptr};
    QTimer * stopTimer{nullptr};
    quint16 queryId{0};
    QString serviceName;    // "_apple-midi._udp.local"

    QSet<QString> instances;                // "MacBook._apple-midi._udp.local"
    QMap<QString, SrvRecord> srv;           // instance -> port, host
    QMap<QString, QHostAddress> hosts;      // host -> ipv4 address
    QMap<QString, QHostAddress> senders;    // instance -> address of the answer
    QSet<QString> resolving;                // instances that were asked directly
    QList<Service> m_services;

    static QList<QNetworkInterface> multicastInterfaces();
    void sendQuery(const QString & name, quint16 type);
    void parse(const QByteArray & packet, const QHostAddress & sender);
    void update();
};

#endif // MDNSBROWSER_H
