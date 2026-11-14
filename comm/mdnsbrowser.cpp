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

#include "mdnsbrowser.h"

#include <QUdpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QDataStream>
#include <QVariant>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QCoreApplication>
#endif
#include <QDebug>
#include <algorithm>

namespace {

const quint16 MdnsPort = 5353;
const quint16 TypeA = 1;
const quint16 TypePtr = 12;
const quint16 TypeSrv = 33;
const quint16 TypeAny = 255;
const quint16 ClassIn = 1;
const int RepeatInterval = 800;     // ms, queries are repeated against packet loss

QByteArray encodeName(const QString & name)
{
    QByteArray out;
    const QStringList labels = name.split(QChar('.'), Qt::SkipEmptyParts);
    for(const QString & label : labels) {
        QByteArray l = label.toUtf8().left(63);
        out.append(char(l.size()));
        out.append(l);
    }
    out.append(char(0));
    return out;
}

// reads a possibly compressed name at offset, offset is moved behind it
bool readName(const QByteArray & p, int & offset, QString & name)
{
    QStringList labels;
    int pos = offset;
    bool jumped = false;
    for(int guard = 0; guard < 128; guard++) {
        if(pos >= p.size()) {
            return false;
        }
        quint8 len = static_cast<quint8>(p[pos]);
        if(len == 0) {
            if(!jumped) {
                offset = pos + 1;
            }
            name = labels.join(QChar('.'));
            return true;
        }
        if((len & 0xC0) == 0xC0) {
            if(pos + 1 >= p.size()) {
                return false;
            }
            int target = ((len & 0x3F) << 8) | static_cast<quint8>(p[pos + 1]);
            if(!jumped) {
                offset = pos + 2;
            }
            jumped = true;
            pos = target;
            continue;
        }
        if(pos + 1 + len > p.size()) {
            return false;
        }
        labels.append(QString::fromUtf8(p.constData() + pos + 1, len));
        pos += 1 + len;
    }
    return false;
}

quint16 read16(const QByteArray & p, int o)
{
    return static_cast<quint16>((static_cast<quint8>(p[o]) << 8) | static_cast<quint8>(p[o + 1]));
}

}

MdnsBrowser::MdnsBrowser(QObject *parent) : QObject(parent)
{
    repeatTimer = new QTimer(this);
    repeatTimer->setInterval(RepeatInterval);
    connect(repeatTimer, &QTimer::timeout, this, &MdnsBrowser::onRepeat);

    stopTimer = new QTimer(this);
    stopTimer->setSingleShot(true);
    connect(stopTimer, &QTimer::timeout, this, [this]() {
        stop();
        emit finished();
    });
}

MdnsBrowser::~MdnsBrowser()
{
    stop();
}

bool MdnsBrowser::running() const
{
    return socket != nullptr;
}

void MdnsBrowser::start(const QString &serviceType, int durationMs)
{
    stop();

    serviceName = serviceType + QStringLiteral(".local");
    instances.clear();
    srv.clear();
    hosts.clear();
    senders.clear();
    resolving.clear();
    m_services.clear();
    emit servicesChanged();

    socket = new QUdpSocket(this);
    if(!socket->bind(QHostAddress::AnyIPv4, 0)) {
        qWarning() << "MdnsBrowser cannot bind" << socket->errorString();
        stop();
        emit finished();
        return;
    }
    socket->setSocketOption(QAbstractSocket::MulticastTtlOption, QVariant(255));
    connect(socket, &QUdpSocket::readyRead, this, &MdnsBrowser::onReadyRead);

#if defined(Q_OS_ANDROID)
    // android drops multicast packets unless a multicast lock is held
    QJniObject::callStaticMethod<void>("org/misuco/misuco3/NsdAnnouncer", "acquireMulticastLock",
                                       "(Landroid/content/Context;)V",
                                       QNativeInterface::QAndroidApplication::context().object());
#endif

    // the multicast answers, port 5353 is shared with the system mDNS service
    mcastSocket = new QUdpSocket(this);
    if(mcastSocket->bind(QHostAddress::AnyIPv4, MdnsPort,
                         QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        mcastSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, QVariant(255));
        const QHostAddress group(QStringLiteral("224.0.0.251"));
        for(const QNetworkInterface & i : multicastInterfaces()) {
            mcastSocket->joinMulticastGroup(group, i);
        }
        connect(mcastSocket, &QUdpSocket::readyRead, this, &MdnsBrowser::onReadyRead);
    } else {
        qWarning() << "MdnsBrowser cannot bind port 5353" << mcastSocket->errorString();
        delete mcastSocket;
        mcastSocket = nullptr;
    }

    // a legacy unicast query must have a non zero id
    queryId = static_cast<quint16>(QRandomGenerator::global()->bounded(1, 65535));

    onRepeat();
    repeatTimer->start();
    stopTimer->start(durationMs);
}

void MdnsBrowser::stop()
{
    repeatTimer->stop();
    stopTimer->stop();
    delete mcastSocket;
    mcastSocket = nullptr;
    if(socket) {
        delete socket;
        socket = nullptr;
#if defined(Q_OS_ANDROID)
        QJniObject::callStaticMethod<void>("org/misuco/misuco3/NsdAnnouncer", "releaseMulticastLock", "()V");
#endif
    }
}

void MdnsBrowser::onRepeat()
{
    sendQuery(serviceName, TypePtr);
}

QList<QNetworkInterface> MdnsBrowser::multicastInterfaces()
{
    QList<QNetworkInterface> list;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for(const QNetworkInterface & i : interfaces) {
        const auto flags = i.flags();
        if(!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)
           || !(flags & QNetworkInterface::CanMulticast) || (flags & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        const auto entries = i.addressEntries();
        bool ipv4 = std::any_of(entries.begin(), entries.end(), [](const QNetworkAddressEntry & e) {
            return e.ip().protocol() == QAbstractSocket::IPv4Protocol;
        });
        if(ipv4) {
            list.append(i);
        }
    }
    return list;
}

void MdnsBrowser::sendQuery(const QString &name, quint16 type)
{
    if(!socket) {
        return;
    }

    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    // id, flags (standard query), 1 question, no answers
    s << queryId << quint16(0) << quint16(1) << quint16(0) << quint16(0) << quint16(0);
    QByteArray n = encodeName(name);
    s.writeRawData(n.constData(), n.size());
    s << type << ClassIn;

    const QHostAddress group(QStringLiteral("224.0.0.251"));

    // the query goes out on every network that can multicast, from the
    // ephemeral port (legacy unicast answer) and from port 5353
    // (multicast answer)
    const QList<QNetworkInterface> interfaces = multicastInterfaces();
    for(QUdpSocket * out : {socket, mcastSocket}) {
        if(!out) {
            continue;
        }
        bool sent = false;
        for(const QNetworkInterface & i : interfaces) {
            out->setMulticastInterface(i);
            if(out->writeDatagram(packet, group, MdnsPort) >= 0) {
                sent = true;
            }
        }
        if(!sent) {
            out->writeDatagram(packet, group, MdnsPort);
        }
    }
}

void MdnsBrowser::onReadyRead()
{
    for(QUdpSocket * in : {socket, mcastSocket}) {
        while(in && in->hasPendingDatagrams()) {
            QNetworkDatagram d = in->receiveDatagram();
            parse(d.data(), d.senderAddress());
        }
    }
    update();
}

void MdnsBrowser::parse(const QByteArray &p, const QHostAddress &sender)
{
    if(p.size() < 12) {
        return;
    }
    quint16 flags = read16(p, 2);
    if(!(flags & 0x8000)) {
        return;     // not a response
    }
    int questions = read16(p, 4);
    int records = read16(p, 6) + read16(p, 8) + read16(p, 10);

    int o = 12;
    QString name;
    for(int i = 0; i < questions; i++) {
        if(!readName(p, o, name) || o + 4 > p.size()) {
            return;
        }
        o += 4;
    }

    QHostAddress senderV4(sender.toIPv4Address());

    for(int i = 0; i < records; i++) {
        if(!readName(p, o, name) || o + 10 > p.size()) {
            return;
        }
        quint16 type = read16(p, o);
        int length = read16(p, o + 8);
        int data = o + 10;
        o = data + length;
        if(o > p.size()) {
            return;
        }

        if(type == TypePtr && name.compare(serviceName, Qt::CaseInsensitive) == 0) {
            int d = data;
            QString instance;
            if(readName(p, d, instance)) {
                instances.insert(instance);
                senders.insert(instance, senderV4);
            }
        } else if(type == TypeSrv && length >= 7) {
            int d = data + 6;
            SrvRecord r;
            r.port = read16(p, data + 4);
            if(readName(p, d, r.target)) {
                srv.insert(name, r);
                senders.insert(name, senderV4);
            }
        } else if(type == TypeA && length == 4) {
            quint32 ip = (static_cast<quint8>(p[data]) << 24) | (static_cast<quint8>(p[data + 1]) << 16)
                       | (static_cast<quint8>(p[data + 2]) << 8) | static_cast<quint8>(p[data + 3]);
            hosts.insert(name.toLower(), QHostAddress(ip));
        }
    }
}

void MdnsBrowser::update()
{
    QList<Service> list;
    for(const QString & instance : std::as_const(instances)) {
        auto it = srv.constFind(instance);
        if(it == srv.constEnd()) {
            // the answer had no service record, ask the instance directly
            if(!resolving.contains(instance)) {
                resolving.insert(instance);
                sendQuery(instance, TypeAny);
            }
            continue;
        }
        Service s;
        // the instance label is the part before the service type
        s.name = instance.left(instance.size() - serviceName.size() - 1);
        s.port = it->port;
        // without an address record the device that answered is taken
        s.address = hosts.value(it->target.toLower(), senders.value(instance));
        if(s.address.isNull()) {
            continue;
        }
        list.append(s);
    }
    std::sort(list.begin(), list.end(), [](const Service & a, const Service & b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });

    bool changed = list.size() != m_services.size();
    for(int i = 0; !changed && i < list.size(); i++) {
        changed = list[i].name != m_services[i].name || list[i].address != m_services[i].address
                  || list[i].port != m_services[i].port;
    }
    if(changed) {
        m_services = list;
        emit servicesChanged();
    }
}
