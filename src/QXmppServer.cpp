/*
 * Copyright (C) 2008-2010 The QXmpp developers
 *
 * Author:
 *  Jeremy Lainé
 *
 * Source:
 *  http://code.google.com/p/qxmpp
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

#include <QDomElement>
#include <QFileInfo>
#include <QSslSocket>

#include "QXmppDialback.h"
#include "QXmppIq.h"
#include "QXmppIncomingClient.h"
#include "QXmppIncomingServer.h"
#include "QXmppOutgoingServer.h"
#include "QXmppPingIq.h"
#include "QXmppServer.h"
#include "QXmppUtils.h"

class QXmppServerPrivate
{
public:
    QXmppServerPrivate();

    QString domain;
    QMap<QString, QStringList> subscribers;
    QXmppLogger *logger;
    QXmppPasswordChecker *passwordChecker;

    // client-to-server
    QXmppSslServer *serverForClients;
    QList<QXmppIncomingClient*> incomingClients;

    // server-to-server
    QList<QXmppOutgoingServer*> outgoingServers;
    QXmppSslServer *serverForServers;
};

QXmppServerPrivate::QXmppServerPrivate()
    : logger(0),
    passwordChecker(0)
{
}

/// Constructs a new XMPP server instance.
///
/// \param parent

QXmppServer::QXmppServer(QObject *parent)
    : QObject(parent),
    d(new QXmppServerPrivate)
{
    d->serverForClients = new QXmppSslServer(this);
    bool check = connect(d->serverForClients, SIGNAL(newConnection(QSslSocket*)),
                         this, SLOT(slotClientConnection(QSslSocket*)));
    Q_ASSERT(check);

    d->serverForServers = new QXmppSslServer(this);
    check = connect(d->serverForServers, SIGNAL(newConnection(QSslSocket*)),
                    this, SLOT(slotServerConnection(QSslSocket*)));
    Q_ASSERT(check);
}

/// Destroys an XMPP server instance.
///

QXmppServer::~QXmppServer()
{
    delete d;
}

/// Returns the server's domain.
///

QString QXmppServer::domain() const
{
    return d->domain;
}

/// Sets the server's domain.
///
/// \param domain

void QXmppServer::setDomain(const QString &domain)
{
    d->domain = domain;
}

/// Returns the QXmppLogger associated with the server.

QXmppLogger *QXmppServer::logger()
{
    return d->logger;
}

/// Sets the QXmppLogger associated with the server.

void QXmppServer::setLogger(QXmppLogger *logger)
{
    d->logger = logger;
}

/// Returns the password checker used to verify client credentials.
///
/// \param checker
///

QXmppPasswordChecker *QXmppServer::passwordChecker()
{
    return d->passwordChecker;
}

/// Sets the password checker used to verify client credentials.
///
/// \param checker
///

void QXmppServer::setPasswordChecker(QXmppPasswordChecker *checker)
{
    d->passwordChecker = checker;
}

/// Sets the path for additional SSL CA certificates.
///
/// \param path

void QXmppServer::addCaCertificates(const QString &path)
{
    if (!path.isEmpty() && !QFileInfo(path).isReadable())
        qWarning() << "SSL CA certificates are not readable" << path;
    d->serverForClients->addCaCertificates(path);
    d->serverForServers->addCaCertificates(path);
}

/// Sets the path for the local SSL certificate.
///
/// \param path

void QXmppServer::setLocalCertificate(const QString &path)
{
    if (!path.isEmpty() && !QFileInfo(path).isReadable())
        qWarning() << "SSL certificate is not readable" << path;
    d->serverForClients->setLocalCertificate(path);
    d->serverForServers->setLocalCertificate(path);
}

/// Sets the path for the local SSL private key.
///
/// \param path

void QXmppServer::setPrivateKey(const QString &path)
{
    if (!path.isEmpty() && !QFileInfo(path).isReadable())
        qWarning() << "SSL key is not readable" << path;
    d->serverForClients->setPrivateKey(path);
    d->serverForServers->setPrivateKey(path);
}

/// Listen for incoming XMPP client connections.
///
/// \param address
/// \param port

bool QXmppServer::listenForClients(const QHostAddress &address, quint16 port)
{
    if (!d->serverForClients->listen(address, port))
    {
        if (logger())
            logger()->log(QXmppLogger::WarningMessage,
                QString("Could not start listening for C2S on port %1").arg(QString::number(port)));
        return false;
    }
    return true;
}

/// Listen for incoming XMPP server connections.
///
/// \param address
/// \param port

bool QXmppServer::listenForServers(const QHostAddress &address, quint16 port)
{
    if (!d->serverForServers->listen(address, port))
    {
        if (logger())
            logger()->log(QXmppLogger::WarningMessage,
                QString("Could not start listening for S2S on port %1").arg(QString::number(port)));
        return false;
    }
    return true;
}

QXmppOutgoingServer* QXmppServer::connectToDomain(const QString &domain)
{
    // initialise outgoing server-to-server
    QXmppOutgoingServer *stream = new QXmppOutgoingServer(d->domain, this);
    stream->setObjectName("S2S-out-" + domain);
    stream->setLocalStreamKey(generateStanzaHash().toAscii());
    stream->setLogger(d->logger);
    stream->configuration().setDomain(domain);
    stream->configuration().setHost(domain);
    stream->configuration().setPort(5269);
    bool check = connect(stream, SIGNAL(elementReceived(QDomElement, bool&)),
                         this, SLOT(slotElementReceived(QDomElement, bool&)));
    Q_ASSERT(check);
    Q_UNUSED(check);

    d->outgoingServers.append(stream);

    // connect to remote server
    stream->connectToHost();
    return stream;
}

/// Returns the XMPP streams for the given recipient.
///
/// \param to
///

QList<QXmppStream*> QXmppServer::getStreams(const QString &to)
{
    QList<QXmppStream*> found;
    if (to.isEmpty())
        return found;
    const QString toDomain = jidToDomain(to);
    if (toDomain == d->domain)
    {
        // look for a client connection
        foreach (QXmppIncomingClient *conn, d->incomingClients)
        {
            if (conn->jid() == to || jidToBareJid(conn->jid()) == to)
                found << conn;
        }
    } else {
        // look for an outgoing S2S connection
        foreach (QXmppOutgoingServer *conn, d->outgoingServers)
        {
            if (conn->configuration().domain() == toDomain && conn->isConnected())
            {
                found << conn;
                break;
            }
        }

        // if we did not find an outgoing server,
        // we need to establish the S2S connection
        if (found.isEmpty())
        {
            connectToDomain(toDomain);

            // FIXME : the current packet will not be delivered
        }
    }
    return found;
}

/// Handles an incoming XML element.
///
/// \param stream
/// \param element

void QXmppServer::handleStanza(QXmppStream *stream, const QDomElement &element)
{
    const QString to = element.attribute("to");

    if (to == d->domain)
    {
        if (element.tagName() == "presence")
        {
            // presence to the local domain, broadcast it to subscribers
            if (element.attribute("type").isEmpty() || element.attribute("type") == "unavailable")
            {
                const QString from = element.attribute("from");
                foreach (QString subscriber, subscribers(from))
                {
                    QDomElement changed(element);
                    changed.setAttribute("to", subscriber);
                    sendElement(changed);
                }
            }
        }
        else if (element.tagName() == "iq")
        {
            // XEP-0199: XMPP Ping
            if (QXmppPingIq::isPingIq(element))
            {
                QXmppPingIq request;
                request.parse(element);

                QXmppIq response(QXmppIq::Result);
                response.setId(element.attribute("id"));
                response.setFrom(d->domain);
                response.setTo(request.from());
                stream->sendPacket(response);
            }
            // Other IQs
            else
            {
                QXmppIq request;
                request.parse(element);

                if (request.type() != QXmppIq::Error && request.type() != QXmppIq::Result)
                {
                    QXmppIq response(QXmppIq::Error);
                    response.setId(request.id());
                    response.setFrom(domain());
                    response.setTo(request.from());
                    QXmppStanza::Error error(QXmppStanza::Error::Cancel,
                        QXmppStanza::Error::FeatureNotImplemented);
                    response.setError(error);
                    stream->sendPacket(response);
                }
            }
        }

    } else {

        if (element.tagName() == "presence")
        {
            // directed presence, update subscribers
            QXmppPresence presence;
            presence.parse(element);

            const QString from = presence.from();
            QStringList subscribers = d->subscribers.value(from);
            if (presence.type() == QXmppPresence::Available)
            {
                subscribers.append(to);
                d->subscribers[from] = subscribers;
            } else if (presence.type() == QXmppPresence::Unavailable) {
                subscribers.removeAll(to);
                d->subscribers[from] = subscribers;
            }
        }

        // route element or reply on behalf of missing peer
        if (!sendElement(element) && element.tagName() == "iq")
        {
            QXmppIq request;
            request.parse(element);

            QXmppIq response(QXmppIq::Error);
            response.setId(request.id());
            response.setFrom(request.to());
            response.setTo(request.from());
            QXmppStanza::Error error(QXmppStanza::Error::Cancel,
                QXmppStanza::Error::ServiceUnavailable);
            response.setError(error);
            stream->sendPacket(response);
        }
    }
}

QStringList QXmppServer::subscribers(const QString &jid)
{
    return d->subscribers.value(jid);
}

/// Route an XMPP stanza.
///
/// \param element

bool QXmppServer::sendElement(const QDomElement &element)
{
    bool sent = false;
    const QString to = element.attribute("to");
    foreach (QXmppStream *conn, getStreams(to))
    {
        if (conn->sendElement(element))
            sent = true;
    }
    return sent;
}

/// Route an XMPP packet.
///
/// \param packet

bool QXmppServer::sendPacket(const QXmppStanza &packet)
{
    bool sent = false;
    foreach (QXmppStream *conn, getStreams(packet.to()))
    {
        if (conn->sendPacket(packet))
            sent = true;
    }
    return sent;
}

/// Handle a new incoming TCP connection from a client.
///
/// \param socket

void QXmppServer::slotClientConnection(QSslSocket *socket)
{
    QXmppIncomingClient *stream = new QXmppIncomingClient(socket, d->domain, this);
    socket->setParent(stream);
    stream->setLogger(d->logger);
    stream->setPasswordChecker(d->passwordChecker);

    bool check = connect(stream, SIGNAL(connected()),
                         this, SLOT(slotClientConnected()));
    Q_ASSERT(check);

    check = connect(stream, SIGNAL(disconnected()),
                    this, SLOT(slotClientDisconnected()));
    Q_ASSERT(check);

    check = connect(stream, SIGNAL(elementReceived(QDomElement, bool&)),
                    this, SLOT(slotElementReceived(QDomElement, bool&)));
    Q_ASSERT(check);

    d->incomingClients.append(stream);
}

/// Handle a successful client authentication.
///

void QXmppServer::slotClientConnected()
{
    QXmppIncomingClient *stream = qobject_cast<QXmppIncomingClient *>(sender());
    if (!stream || !d->incomingClients.contains(stream))
        return;

    // check whether the connection conflicts with another one
    foreach (QXmppIncomingClient *conn, d->incomingClients)
    {
        if (conn != stream && conn->jid() == stream->jid())
        {
            conn->sendData("<stream:error><conflict xmlns='urn:ietf:params:xml:ns:xmpp-streams'/><text xmlns='urn:ietf:params:xml:ns:xmpp-streams'>Replaced by new connection</text></stream:error>");
            conn->disconnectFromHost();
        }
    }

    // update statistics
    updateStatistics();
}

/// Handle a disconnection from a client.
///

void QXmppServer::slotClientDisconnected()
{
    QXmppIncomingClient *stream = qobject_cast<QXmppIncomingClient *>(sender());
    if (!stream || !d->incomingClients.contains(stream))
        return;

    // notify subscribed peers of disconnection
    if (!stream->jid().isEmpty())
    {
        foreach (QString subscriber, d->subscribers.value(stream->jid()))
        {
            QXmppPresence presence;
            presence.setFrom(stream->jid());
            presence.setTo(subscriber);
            presence.setType(QXmppPresence::Unavailable);
            sendPacket(presence);
        }
    }
    d->incomingClients.removeAll(stream);
    stream->deleteLater();

    // update statistics
    updateStatistics();
}

void QXmppServer::slotDialbackRequestReceived(const QXmppDialback &dialback)
{
    QXmppIncomingServer *stream = qobject_cast<QXmppIncomingServer *>(sender());
    if (!stream)
        return;

    if (dialback.command() == QXmppDialback::Verify)
    {
        // handle a verify request
        foreach (QXmppOutgoingServer *out, d->outgoingServers)
        {
            if (out->configuration().domain() != dialback.from())
                continue;

            bool isValid = dialback.key() == out->localStreamKey();
            QXmppDialback verify;
            verify.setCommand(QXmppDialback::Verify);
            verify.setId(dialback.id());
            verify.setTo(dialback.from());
            verify.setFrom(d->domain);
            verify.setType(isValid ? "valid" : "invalid");
            stream->sendPacket(verify);
            return;
        }
    }
}

/// Handle an incoming XML element.

void QXmppServer::slotElementReceived(const QDomElement &element, bool &handled)
{
    QXmppStream *incoming = qobject_cast<QXmppStream *>(sender());
    if (!incoming)
        return;
    handleStanza(incoming, element);
}

/// Handle a new incoming TCP connection from a server.
///
/// \param socket

void QXmppServer::slotServerConnection(QSslSocket *socket)
{
    QXmppIncomingServer *stream = new QXmppIncomingServer(socket, d->domain, this);
    socket->setParent(stream);
    stream->setLogger(d->logger);

    bool check = connect(stream, SIGNAL(disconnected()),
                         stream, SLOT(deleteLater()));
    Q_ASSERT(check);

    check = connect(stream, SIGNAL(dialbackRequestReceived(QXmppDialback)),
                    this, SLOT(slotDialbackRequestReceived(QXmppDialback)));
    Q_ASSERT(check);

    check = connect(stream, SIGNAL(elementReceived(QDomElement, bool&)),
                    this, SLOT(slotElementReceived(QDomElement, bool&)));
    Q_ASSERT(check);
}

void QXmppServer::updateStatistics()
{
}

class QXmppSslServerPrivate
{
public:
    QString caCertificates;
    QString localCertificate;
    QString privateKey;
};

/// Constructs a new SSL server instance.
///
/// \param parent

QXmppSslServer::QXmppSslServer(QObject *parent)
    : QTcpServer(parent),
    d(new QXmppSslServerPrivate)
{
}

/// Destroys an SSL server instance.
///

QXmppSslServer::~QXmppSslServer()
{
    delete d;
}

void QXmppSslServer::incomingConnection(int socketDescriptor)
{
    QSslSocket *socket = new QSslSocket;
    socket->setSocketDescriptor(socketDescriptor);
    if (!d->localCertificate.isEmpty() && !d->privateKey.isEmpty())
    {
        socket->setProtocol(QSsl::AnyProtocol);
        socket->addCaCertificates(d->caCertificates);
        socket->setLocalCertificate(d->localCertificate);
        socket->setPrivateKey(d->privateKey);
    }
    emit newConnection(socket);
}

void QXmppSslServer::addCaCertificates(const QString &caCertificates)
{
    d->caCertificates = caCertificates;
}

void QXmppSslServer::setLocalCertificate(const QString &localCertificate)
{
    d->localCertificate = localCertificate;
}

void QXmppSslServer::setPrivateKey(const QString &privateKey)
{
    d->privateKey = privateKey;
}

