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

#include "bonjourannouncer.h"
#include <QDebug>

#if defined(Q_OS_ANDROID)

#include <QJniObject>
#include <QCoreApplication>

struct BonjourAnnouncer::Platform {};

bool BonjourAnnouncer::announce(const QString &name, const QString &serviceType, quint16 port)
{
    withdraw();
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    m_announced = QJniObject::callStaticMethod<jboolean>(
        "org/misuco/misuco3/NsdAnnouncer", "register",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;I)Z",
        context.object(),
        QJniObject::fromString(name).object<jstring>(),
        QJniObject::fromString(serviceType).object<jstring>(),
        jint(port));
    return m_announced;
}

void BonjourAnnouncer::withdraw()
{
    if(m_announced) {
        QJniObject::callStaticMethod<void>("org/misuco/misuco3/NsdAnnouncer", "unregister", "()V");
        m_announced = false;
    }
}

#elif defined(Q_OS_WIN)

#include <QHostInfo>
#include <QNetworkInterface>
#include <QSysInfo>
#include <windows.h>
#include <windns.h>
#include <atomic>

// The functions are loaded at run time, they are missing before
// Windows 10 1809.
namespace {
using ConstructInstanceFn = PDNS_SERVICE_INSTANCE (WINAPI *)(PCWSTR, PCWSTR, PIP4_ADDRESS, PIP6_ADDRESS,
                                                             WORD, WORD, WORD, DWORD, PCWSTR *, PCWSTR *);
using FreeInstanceFn = VOID (WINAPI *)(PDNS_SERVICE_INSTANCE);
using RegisterFn = DWORD (WINAPI *)(PDNS_SERVICE_REGISTER_REQUEST, PDNS_SERVICE_CANCEL);

struct DnsApi {
    ConstructInstanceFn constructInstance{nullptr};
    FreeInstanceFn freeInstance{nullptr};
    RegisterFn registerService{nullptr};
    RegisterFn deregisterService{nullptr};

    DnsApi() {
        HMODULE lib = LoadLibraryW(L"dnsapi.dll");
        if(lib) {
            constructInstance = reinterpret_cast<ConstructInstanceFn>(
                reinterpret_cast<void *>(GetProcAddress(lib, "DnsServiceConstructInstance")));
            freeInstance = reinterpret_cast<FreeInstanceFn>(
                reinterpret_cast<void *>(GetProcAddress(lib, "DnsServiceFreeInstance")));
            registerService = reinterpret_cast<RegisterFn>(
                reinterpret_cast<void *>(GetProcAddress(lib, "DnsServiceRegister")));
            deregisterService = reinterpret_cast<RegisterFn>(
                reinterpret_cast<void *>(GetProcAddress(lib, "DnsServiceDeRegister")));
        }
    }
    bool ok() const {
        return constructInstance && freeInstance && registerService && deregisterService;
    }
};

DnsApi & dnsApi()
{
    static DnsApi api;
    return api;
}

}

// The request must stay valid until the asynchronous register and
// deregister calls have completed, the last callback deletes it.
struct BonjourAnnouncer::Platform {
    PDNS_SERVICE_INSTANCE instance{nullptr};
    DNS_SERVICE_REGISTER_REQUEST request{};
    std::atomic<int> pendingCallbacks{0};
    std::atomic<bool> released{false};

    void callbackDone() {
        if(pendingCallbacks.fetch_sub(1) == 1 && released.load()) {
            dispose();
        }
    }
    void release() {
        released = true;
        if(pendingCallbacks.load() == 0) {
            dispose();
        }
    }
    void dispose() {
        bool expected = true;
        if(released.compare_exchange_strong(expected, false)) {
            dnsApi().freeInstance(instance);
            delete this;
        }
    }
};

namespace {
VOID WINAPI registerComplete(DWORD status, PVOID context, PDNS_SERVICE_INSTANCE instance)
{
    qDebug() << "BonjourAnnouncer register / deregister complete, status" << status;
    if(instance) {
        dnsApi().freeInstance(instance);
    }
    if(context) {
        static_cast<BonjourAnnouncer::Platform *>(context)->callbackDone();
    }
}
}

bool BonjourAnnouncer::announce(const QString &name, const QString &serviceType, quint16 port)
{
    withdraw();
    DnsApi & api = dnsApi();
    if(!api.ok()) {
        qWarning() << "BonjourAnnouncer: DnsServiceRegister is not available";
        return false;
    }

    // the address of the host record, the first ipv4 address
    IP4_ADDRESS ip4 = 0;
    const auto addresses = QNetworkInterface::allAddresses();
    for(const QHostAddress & a : addresses) {
        if(a.protocol() == QAbstractSocket::IPv4Protocol && !a.isLoopback() && !a.isLinkLocal()) {
            ip4 = htonl(a.toIPv4Address());
            break;
        }
    }

    std::wstring instanceName = (name + QChar('.') + serviceType + QStringLiteral(".local")).toStdWString();
    std::wstring hostName = (QHostInfo::localHostName() + QStringLiteral(".local")).toStdWString();

    d = new Platform;
    d->instance = api.constructInstance(instanceName.c_str(), hostName.c_str(), ip4 ? &ip4 : nullptr,
                                        nullptr, port, 0, 0, 0, nullptr, nullptr);
    if(!d->instance) {
        delete d;
        d = nullptr;
        return false;
    }
    d->request.Version = DNS_QUERY_REQUEST_VERSION1;
    d->request.InterfaceIndex = 0;
    d->request.pServiceInstance = d->instance;
    d->request.pRegisterCompletionCallback = registerComplete;
    d->request.pQueryContext = d;
    d->request.hCredentials = nullptr;
    d->request.unicastEnabled = FALSE;

    d->pendingCallbacks = 1;
    DWORD status = api.registerService(&d->request, nullptr);
    if(status != DNS_REQUEST_PENDING) {
        qWarning() << "BonjourAnnouncer: DnsServiceRegister failed" << status;
        d->pendingCallbacks = 0;
        d->release();
        d = nullptr;
        return false;
    }
    m_announced = true;
    return true;
}

void BonjourAnnouncer::withdraw()
{
    if(!d) {
        return;
    }
    if(m_announced) {
        d->pendingCallbacks++;
        if(dnsApi().deregisterService(&d->request, nullptr) != DNS_REQUEST_PENDING) {
            d->pendingCallbacks--;
        }
    }
    d->release();
    d = nullptr;
    m_announced = false;
}

#elif defined(Q_OS_DARWIN)

#include <dns_sd.h>
#include <arpa/inet.h>

struct BonjourAnnouncer::Platform {
    DNSServiceRef ref{nullptr};
};

bool BonjourAnnouncer::announce(const QString &name, const QString &serviceType, quint16 port)
{
    withdraw();
    d = new Platform;
    DNSServiceErrorType err = DNSServiceRegister(&d->ref, 0, 0, name.toUtf8().constData(),
                                                 serviceType.toUtf8().constData(), nullptr, nullptr,
                                                 htons(port), 0, nullptr, nullptr, nullptr);
    if(err != kDNSServiceErr_NoError) {
        qWarning() << "BonjourAnnouncer: DNSServiceRegister failed" << err;
        delete d;
        d = nullptr;
        return false;
    }
    m_announced = true;
    return true;
}

void BonjourAnnouncer::withdraw()
{
    if(d) {
        DNSServiceRefDeallocate(d->ref);
        delete d;
        d = nullptr;
    }
    m_announced = false;
}

#else

struct BonjourAnnouncer::Platform {};

bool BonjourAnnouncer::announce(const QString &, const QString &, quint16)
{
    qWarning() << "BonjourAnnouncer: not supported on this platform";
    return false;
}

void BonjourAnnouncer::withdraw()
{
}

#endif

BonjourAnnouncer::BonjourAnnouncer(QObject *parent) : QObject(parent)
{
}

BonjourAnnouncer::~BonjourAnnouncer()
{
    withdraw();
}
