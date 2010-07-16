/*
 * Copyright (C) 2010 Bolloré telecom
 *
 * Author:
 *	Jeremy Lainé
 *
 * Source:
 *	http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#define QXMPP_DEBUG_STUN

#include <QCryptographicHash>
#include <QDebug>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QTimer>

#include "QXmppStun.h"
#include "QXmppUtils.h"

static const quint32 STUN_MAGIC = 0x2112A442;
static const quint16 STUN_HEADER = 20;
static const quint8 STUN_IPV4 = 0x01;
static const quint8 STUN_IPV6 = 0x02;

enum MessageType {
    BindingRequest       = 0x0001,
    BindingIndication    = 0x0011,
    BindingResponse      = 0x0101,
    BindingError         = 0x0111,
    SharedSecretRequest  = 0x0002,
    SharedSecretResponse = 0x0102,
    SharedSecretError    = 0x0112,
};

enum AttributeType {
    Username         = 0x0006,
    MessageIntegrity = 0x0008,
    ErrorCode        = 0x0009,
    XorMappedAddress = 0x0020,
    Priority         = 0x0024,
    UseCandidate     = 0x0025,
    Fingerprint      = 0x8028,
    IceControlled    = 0x8029,
    IceControlling   = 0x802a,
};

static QByteArray randomByteArray(int length)
{
    QByteArray result(length, 0);
    for (int i = 0; i < length; ++i)
        result[i] = quint8(qrand() % 255);
    return result;
}

class QXmppStunMessage
{
public:
    QXmppStunMessage();

    QByteArray encode(const QString &password = QString()) const;
    bool decode(const QByteArray &buffer, const QString &password = QString(), QStringList *errors = 0);
    QString toString() const;
    static quint16 peekType(const QByteArray &buffer);

    quint16 type;
    QByteArray id;

    // attributes
    int errorCode;
    QString errorPhrase;
    quint32 priority;
    QByteArray iceControlling;
    QByteArray iceControlled;
    QHostAddress mappedHost;
    quint16 mappedPort;
    QString username;
    bool useCandidate;

private:
    void setBodyLength(QByteArray &buffer, qint16 length) const;
};

/// Constructs a new QXmppStunMessage.

QXmppStunMessage::QXmppStunMessage()
    : errorCode(0), priority(0), mappedPort(0), useCandidate(false)
{
}

/// Decodes a QXmppStunMessage and checks its integrity using the given
/// password.
///
/// \param buffer
/// \param password

bool QXmppStunMessage::decode(const QByteArray &buffer, const QString &password, QStringList *errors)
{
    QStringList silent;
    if (!errors)
        errors = &silent;

    if (buffer.size() < STUN_HEADER)
    {
        *errors << QLatin1String("Received a truncated STUN packet");
        return false;
    }

    // parse STUN header
    QDataStream stream(buffer);
    quint16 length;
    quint32 cookie;
    stream >> type;
    stream >> length;
    stream >> cookie;
    id.resize(12);
    stream.readRawData(id.data(), id.size());

    if (cookie != STUN_MAGIC || length != buffer.size() - STUN_HEADER)
    {
        *errors << QLatin1String("Received an invalid STUN packet");
        return false;
    }

    // parse STUN attributes
    int done = 0;
    bool after_integrity = false;
    while (done < length)
    {
        quint16 a_type, a_length;
        stream >> a_type;
        stream >> a_length;
        const int pad_length = 4 * ((a_length + 3) / 4) - a_length;

        // only FINGERPRINT is allowed after MESSAGE-INTEGRITY
        if (after_integrity && a_type != Fingerprint)
        {
            *errors << QString("Skipping attribute %1 after MESSAGE-INTEGRITY").arg(QString::number(a_type));
            stream.skipRawData(a_length + pad_length);
            done += 4 + a_length + pad_length;
            continue;
        }

        if (a_type == Priority)
        {
            // PRIORITY
            if (a_length != sizeof(priority))
                return false;
            stream >> priority;

        } else if (a_type == ErrorCode) {

            // ERROR-CODE
            if (a_length < 4)
                return false;
            quint16 reserved;
            quint8 errorCodeHigh, errorCodeLow;
            stream >> reserved;
            stream >> errorCodeHigh;
            stream >> errorCodeLow;
            errorCode = errorCodeHigh * 100 + errorCodeLow;
            QByteArray phrase(a_length - 4, 0);
            stream.readRawData(phrase.data(), phrase.size());
            errorPhrase = QString::fromUtf8(phrase);

        } else if (a_type == UseCandidate) {

            // USE-CANDIDATE
            if (a_length != 0)
                return false;
            useCandidate = true;

        } else if (a_type == XorMappedAddress) {

            // XOR-MAPPED-ADDRESS
            if (a_length < 4)
                return false;
            quint8 reserved, protocol;
            quint16 xport;
            stream >> reserved;
            stream >> protocol;
            stream >> xport;
            mappedPort = xport ^ (STUN_MAGIC >> 16);
            if (protocol == STUN_IPV4)
            {
                if (a_length != 8)
                    return false;
                quint32 xaddr;
                stream >> xaddr;
                mappedHost = QHostAddress(xaddr ^ STUN_MAGIC);
            } else if (protocol == STUN_IPV6) {
                if (a_length != 20)
                    return false;
                QByteArray xaddr(16, 0);
                stream.readRawData(xaddr.data(), xaddr.size());
                QByteArray xpad;
                QDataStream(&xpad, QIODevice::WriteOnly) << STUN_MAGIC;
                xpad += id;
                Q_IPV6ADDR addr;
                for (int i = 0; i < 16; i++)
                    addr[i] = xaddr[i] ^ xpad[i];
                mappedHost = QHostAddress(addr);
            } else {
                *errors << QString("Bad protocol %1").arg(QString::number(protocol));
                return false;
            }

        } else if (a_type == MessageIntegrity) {

            // MESSAGE-INTEGRITY
            if (a_length != 20)
                return false;
            QByteArray integrity(20, 0);
            stream.readRawData(integrity.data(), integrity.size());

            // check HMAC-SHA1
            if (!password.isEmpty())
            {
                const QByteArray key = password.toUtf8();
                QByteArray copy = buffer.left(STUN_HEADER + done);
                setBodyLength(copy, done + 24);
                if (integrity != generateHmacSha1(key, copy))
                {
                    *errors << QLatin1String("Bad message integrity");
                    return false;
                }
            }

            // from here onwards, only FINGERPRINT is allowed
            after_integrity = true;

        } else if (a_type == Fingerprint) {

            // FINGERPRINT
            if (a_length != 4)
                return false;
            quint32 fingerprint;
            stream >> fingerprint;

            // check CRC32
            QByteArray copy = buffer.left(STUN_HEADER + done);
            setBodyLength(copy, done + 8);
            const quint32 expected = generateCrc32(copy) ^ 0x5354554eL;
            if (fingerprint != expected)
            {
                *errors << QLatin1String("Bad fingerprint");
                return false;
            }

            // stop parsing, no more attributes are allowed
            return true;

        } else if (a_type == IceControlling) {

            /// ICE-CONTROLLING
            if (a_length != 8)
                return false;
            iceControlling.resize(8);
            stream.readRawData(iceControlling.data(), iceControlling.size());

         } else if (a_type == IceControlled) {

            /// ICE-CONTROLLED
            if (a_length != 8)
                return false;
            iceControlled.resize(8);
            stream.readRawData(iceControlled.data(), iceControlled.size());

        } else if (a_type == Username) {

            // USERNAME
            QByteArray utf8Username(a_length, 0);
            stream.readRawData(utf8Username.data(), utf8Username.size());
            username = QString::fromUtf8(utf8Username);

        } else {

            // Unknown attribute
            stream.skipRawData(a_length);
            *errors << QString("Skipping unknown attribute %1").arg(QString::number(a_type));

        }
        stream.skipRawData(pad_length);
        done += 4 + a_length + pad_length;
    }
    return true;
}

/// Encodes the current QXmppStunMessage, optionally calculating the
/// message integrity attribute using the given password.
/// 
/// \param password

QByteArray QXmppStunMessage::encode(const QString &password) const
{
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);

    // encode STUN header
    quint16 length = 0;
    stream << type;
    stream << length;
    stream << STUN_MAGIC;
    stream.writeRawData(id.data(), id.size());

    // XOR-MAPPED-ADDRESS
    if (mappedPort && !mappedHost.isNull() &&
        (mappedHost.protocol() == QAbstractSocket::IPv4Protocol ||
         mappedHost.protocol() == QAbstractSocket::IPv6Protocol))
    {
        stream << quint16(XorMappedAddress);
        stream << quint16(8);
        stream << quint8(0);
        if (mappedHost.protocol() == QAbstractSocket::IPv4Protocol)
        {
            stream << quint8(STUN_IPV4);
            stream << quint16(mappedPort ^ (STUN_MAGIC >> 16));
            stream << quint32(mappedHost.toIPv4Address() ^ STUN_MAGIC);
        } else {
            stream << quint8(STUN_IPV6);
            stream << quint16(mappedPort ^ (STUN_MAGIC >> 16));
            Q_IPV6ADDR addr = mappedHost.toIPv6Address();
            QByteArray xaddr;
            QDataStream(&xaddr, QIODevice::WriteOnly) << STUN_MAGIC;
            xaddr += id;
            for (int i = 0; i < 16; i++)
                xaddr[i] = xaddr[i] ^ addr[i];
            stream.writeRawData(xaddr.data(), xaddr.size());
        }
    }

    // ERROR-CODE
    if (errorCode)
    {
        const quint16 reserved = 0;
        const quint8 errorCodeHigh = errorCode / 100;
        const quint8 errorCodeLow = errorCode % 100;
        const QByteArray phrase = errorPhrase.toUtf8();
        stream << quint16(ErrorCode);
        stream << quint16(phrase.size() + 4);
        stream << reserved;
        stream << errorCodeHigh;
        stream << errorCodeLow;
        stream.writeRawData(phrase.data(), phrase.size());
        if (phrase.size() % 4)
        {
            const QByteArray padding(4 - (phrase.size() %  4), 0);
            stream.writeRawData(padding.data(), padding.size());
        }
    }

    // PRIORITY
    if (priority)
    {
        stream << quint16(Priority);
        stream << quint16(sizeof(priority));
        stream << priority;
    }

    // USE-CANDIDATE
    if (useCandidate)
    {
        stream << quint16(UseCandidate);
        stream << quint16(0);
    }

    // ICE-CONTROLLING or ICE-CONTROLLED
    if (!iceControlling.isEmpty())
    {
        stream << quint16(IceControlling);
        stream << quint16(iceControlling.size());
        stream.writeRawData(iceControlling.data(), iceControlling.size());
    } else if (!iceControlled.isEmpty()) {
        stream << quint16(IceControlled);
        stream << quint16(iceControlled.size());
        stream.writeRawData(iceControlled.data(), iceControlled.size());
    }

    // USERNAME
    if (!username.isEmpty())
    {
        const QByteArray user = username.toUtf8();
        stream << quint16(Username);
        stream << quint16(user.size());
        stream.writeRawData(user.data(), user.size());
        if (user.size() % 4)
        {
            const QByteArray padding(4 - (user.size() % 4), 0);
            stream.writeRawData(padding.data(), padding.size());
        }
    }

    // set body length
    setBodyLength(buffer, buffer.size() - STUN_HEADER);

    // MESSAGE-INTEGRITY
    if (!password.isEmpty())
    {
        const QByteArray key = password.toUtf8();
        setBodyLength(buffer, buffer.size() - STUN_HEADER + 24);
        QByteArray integrity = generateHmacSha1(key, buffer);
        stream << quint16(MessageIntegrity);
        stream << quint16(integrity.size());
        stream.writeRawData(integrity.data(), integrity.size());
    }

    // FINGERPRINT
    setBodyLength(buffer, buffer.size() - STUN_HEADER + 8);
    quint32 fingerprint = generateCrc32(buffer) ^ 0x5354554eL;
    stream << quint16(Fingerprint);
    stream << quint16(sizeof(fingerprint));
    stream << fingerprint;

    return buffer;
}

/// If the given packet looks like a STUN message, returns the message
/// type, otherwise returns 0.
///
/// \param buffer

quint16 QXmppStunMessage::peekType(const QByteArray &buffer)
{
    if (buffer.size() < STUN_HEADER)
        return 0;

    // parse STUN header
    QDataStream stream(buffer);
    quint16 type;
    quint16 length;
    quint32 cookie;
    stream >> type;
    stream >> length;
    stream >> cookie;

    if (cookie != STUN_MAGIC || length != buffer.size() - STUN_HEADER)
        return 0;

    return type;
}
 
void QXmppStunMessage::setBodyLength(QByteArray &buffer, qint16 length) const
{
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream.device()->seek(2);
    stream << length;
}

QString QXmppStunMessage::toString() const
{
    QStringList dumpLines;
    QString typeName;
    switch (type & 0x000f)
    {
        case 1: typeName = "Binding"; break;
        case 2: typeName = "Shared Secret"; break;
        default: typeName = "Unknown"; break;
    }
    switch (type & 0x0ff0)
    {
        case 0x000: typeName += " Request"; break;
        case 0x010: typeName += " Indication"; break;
        case 0x100: typeName += " Response"; break;
        case 0x110: typeName += " Error"; break;
        default: break;
    }
    dumpLines << QString(" type %1 (%2)")
        .arg(typeName)
        .arg(QString::number(type));
    dumpLines << QString(" id %1").arg(QString::fromAscii(id.toHex()));

    // attributes
    if (!username.isEmpty())
        dumpLines << QString(" * USERNAME %1").arg(username);
    if (errorCode)
        dumpLines << QString(" * ERROR-CODE %1 %2")
            .arg(QString::number(errorCode))
            .arg(errorPhrase);
    if (mappedPort)
        dumpLines << QString(" * MAPPED %1 %2")
            .arg(mappedHost.toString())
            .arg(QString::number(mappedPort));
    if (!iceControlling.isEmpty())
        dumpLines << QString(" * ICE-CONTROLLING %1")
            .arg(QString::fromAscii(iceControlling.toHex()));
    if (!iceControlled.isEmpty())
        dumpLines << QString(" * ICE-CONTROLLED %1")
            .arg(QString::fromAscii(iceControlled.toHex()));

    return dumpLines.join("\n");
}

QXmppStunSocket::Pair::Pair()
    : checked(QIODevice::NotOpen)
{
    // FIXME : calculate priority
    priority = 1862270975;
    transaction = randomByteArray(12);
}

QString QXmppStunSocket::Pair::toString() const
{
    QString str = QString("%1 %2").arg(remote.host().toString(), QString::number(remote.port()));
    if (!reflexive.host().isNull() && reflexive.port())
        str += QString(" (reflexive %1 %2)").arg(reflexive.host().toString(), QString::number(reflexive.port()));
    return str;
}

/// Constructs a new QXmppStunSocket.
///

QXmppStunSocket::QXmppStunSocket(bool iceControlling, QObject *parent)
    : QObject(parent),
    m_activePair(0),
    m_iceControlling(iceControlling)
{
    m_localUser = generateStanzaHash(4);
    m_localPassword = generateStanzaHash(22);

    m_socket = new QUdpSocket(this);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    if (!m_socket->bind())
        qWarning("QXmppStunSocket could not start listening");

    m_timer = new QTimer(this);
    m_timer->setInterval(500);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkCandidates())); 
}

QXmppStunSocket::~QXmppStunSocket()
{
    foreach (Pair *pair, m_pairs)
        delete pair;
}

/// Returns the component id for the current socket, e.g. 1 for RTP
/// and 2 for RTCP.

int QXmppStunSocket::component() const
{
    return m_component;
}

/// Sets the component id for the current socket, e.g. 1 for RTP
/// and 2 for RTCP.
///
/// \param component

void QXmppStunSocket::setComponent(int component)
{
    m_component = component;
}

void QXmppStunSocket::checkCandidates()
{
    debug("Checking remote candidates");
    foreach (Pair *pair, m_pairs)
    {
        // send a binding request
        QXmppStunMessage message;
        message.id = pair->transaction;
        message.type = BindingRequest;
        message.priority = pair->priority;
        message.username = QString("%1:%2").arg(m_remoteUser, m_localUser);
        if (m_iceControlling)
        {
            message.iceControlling = QByteArray(8, 0);
            message.useCandidate = true;
        } else {
            message.iceControlled = QByteArray(8, 0);
        }
        // REMOTE PWD
        writeStun(message, pair);
    }
}

/// Closes the socket.

void QXmppStunSocket::close()
{
    m_socket->close();
    m_timer->stop();
}

/// Start ICE connectivity checks.

void QXmppStunSocket::connectToHost()
{
    checkCandidates();
    m_timer->start();
}

/// Returns the QIODevice::OpenMode which represents the socket's ability
/// to read and/or write data.

QIODevice::OpenMode QXmppStunSocket::openMode() const
{
    return m_activePair ? QIODevice::ReadWrite : QIODevice::NotOpen;
}

void QXmppStunSocket::debug(const QString &message, QXmppLogger::MessageType type)
{
    emit logMessage(type, QString("STUN(%1) %2").arg(QString::number(m_component)).arg(message));
}

/// Returns the list of local HOST CANDIDATES candidates by iterating
/// over the available network interfaces.

QList<QXmppJingleCandidate> QXmppStunSocket::localCandidates() const
{
    QList<QXmppJingleCandidate> candidates;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) ||
            interface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol ||
                entry.netmask().isNull() ||
                entry.netmask() == QHostAddress::Broadcast)
                continue;

            QXmppJingleCandidate candidate;
            candidate.setComponent(m_component);
            candidate.setHost(entry.ip());
            candidate.setId(generateStanzaHash(10));
            candidate.setNetwork(interface.index());
            candidate.setPort(m_socket->localPort());
            candidate.setPriority(2130706432 - m_component);
            candidate.setProtocol("udp");
            candidate.setType("host");
            candidates << candidate;
        }
    }
    return candidates;
}

QString QXmppStunSocket::localUser() const
{
    return m_localUser;
}

void QXmppStunSocket::setLocalUser(const QString &user)
{
    m_localUser = user;
}

QString QXmppStunSocket::localPassword() const
{
    return m_localPassword;
}

void QXmppStunSocket::setLocalPassword(const QString &password)
{
    m_localPassword = password;
}

/// Adds a remote STUN candidate.

bool QXmppStunSocket::addRemoteCandidate(const QXmppJingleCandidate &candidate)
{
    if (candidate.component() != m_component ||
        candidate.type() != "host" ||
        candidate.protocol() != "udp")
        return false;

    foreach (Pair *pair, m_pairs)
        if (pair->remote.host() == candidate.host() &&
            pair->remote.port() == candidate.port())
            return false;

    Pair *pair = new Pair;
    pair->remote = candidate;
    m_pairs << pair;
    return true;
}

/// Adds a discovered STUN candidate.

QXmppStunSocket::Pair *QXmppStunSocket::addRemoteCandidate(const QHostAddress &host, quint16 port)
{
    foreach (Pair *pair, m_pairs)
        if (pair->remote.host() == host &&
            pair->remote.port() == port)
            return pair;

    QXmppJingleCandidate candidate;
    candidate.setHost(host);
    candidate.setPort(port);
    candidate.setProtocol("udp");
    candidate.setComponent(m_component);

    Pair *pair = new Pair;
    pair->remote = candidate;
    m_pairs << pair;

    debug(QString("Added candidate %1").arg(pair->toString()));
    return pair;
}

void QXmppStunSocket::setRemoteUser(const QString &user)
{
    m_remoteUser = user;
}

void QXmppStunSocket::setRemotePassword(const QString &password)
{
    m_remotePassword = password;
}

void QXmppStunSocket::readyRead()
{
    const qint64 size = m_socket->pendingDatagramSize();
    QHostAddress remoteHost;
    quint16 remotePort;
    QByteArray buffer(size, 0);
    m_socket->readDatagram(buffer.data(), buffer.size(), &remoteHost, &remotePort);

    // if this is not a STUN message, emit it
    quint16 messageType = QXmppStunMessage::peekType(buffer);
    if (!messageType)
    {
        emit datagramReceived(buffer, remoteHost, remotePort);
        return;
    }

    // parse STUN message
    const QString messagePassword = (messageType & 0xFF00) ? m_remotePassword : m_localPassword;
    if (messagePassword.isEmpty())
        return;
    QXmppStunMessage message;
    QStringList errors;
    if (!message.decode(buffer, messagePassword, &errors))
    {
        foreach (const QString &error, errors)
            debug(error, QXmppLogger::WarningMessage);
        return;
    }
#ifdef QXMPP_DEBUG_STUN
    debug(QString("Received from %1 port %2\n%3")
        .arg(remoteHost.toString())
        .arg(QString::number(remotePort))
        .arg(message.toString()),
        QXmppLogger::ReceivedMessage);
#endif

    if (m_activePair)
        return;

    // process message
    Pair *pair = 0;
    if (message.type == BindingRequest)
    {
        // add remote candidate
        pair = addRemoteCandidate(remoteHost, remotePort);

        // send a binding response
        QXmppStunMessage response;
        response.id = message.id;
        response.type = BindingResponse;
        response.username = message.username;
        response.mappedHost = pair->remote.host();
        response.mappedPort = pair->remote.port();
        writeStun(response, pair);

        // update state
        if (m_iceControlling || message.useCandidate)
        {
            debug(QString("ICE reverse check %1").arg(pair->toString()));
            pair->checked |= QIODevice::ReadOnly;
        }

        if (!m_iceControlling)
        {
            // send a triggered connectivity test
            QXmppStunMessage message;
            message.id = pair->transaction;
            message.type = BindingRequest;
            message.priority = pair->priority;
            message.username = QString("%1:%2").arg(m_remoteUser, m_localUser);
            message.iceControlled = QByteArray(8, 0);
            writeStun(message, pair);
        }

    } else if (message.type == BindingResponse) {

        // find the pair for this transaction
        foreach (Pair *ptr, m_pairs)
        {
            if (ptr->transaction == message.id)
            {
                pair = ptr;
                break;
            }
        }
        if (!pair)
        {
            debug(QString("Unknown transaction %1").arg(QString::fromAscii(message.id.toHex())));
            return;
        }
        // store reflexive address
        pair->reflexive.setHost(message.mappedHost);
        pair->reflexive.setPort(message.mappedPort);

        // add the new remote candidate
        addRemoteCandidate(remoteHost, remotePort);

#if 0
        // send a binding indication
        QXmppStunMessage indication;
        indication.id = randomByteArray(12);
        indication.type = BindingIndication;
        m_socket->writeStun(indication, pair);
#endif
        // outgoing media can flow
        debug(QString("ICE forward check %1").arg(pair->toString()));
        pair->checked |= QIODevice::WriteOnly;
    }

    // signal completion
    if (pair && pair->checked == QIODevice::ReadWrite)
    { 
        debug(QString("ICE completed %1").arg(pair->toString()));
        m_activePair = pair;
        m_timer->stop();
        emit ready();
    }
}

/// Sends a data packet to the remote party.
///
/// \param datagram

qint64 QXmppStunSocket::writeDatagram(const QByteArray &datagram)
{
    if (!m_activePair)
        return -1;
    return m_socket->writeDatagram(datagram, m_activePair->remote.host(), m_activePair->remote.port());
}

/// Sends a STUN packet to the remote party.

qint64 QXmppStunSocket::writeStun(const QXmppStunMessage &message, QXmppStunSocket::Pair *pair)
{
    const QString messagePassword = (message.type & 0xFF00) ? m_localPassword : m_remotePassword;
#ifdef QXMPP_DEBUG_STUN
    debug(
        QString("Sent to %1\n%2").arg(pair->toString(), message.toString()),
        QXmppLogger::SentMessage);
#endif
    m_socket->writeDatagram(message.encode(messagePassword), pair->remote.host(), pair->remote.port());
}

