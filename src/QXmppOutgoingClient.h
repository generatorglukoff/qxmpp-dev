/*
 * Copyright (C) 2008-2010 The QXmpp developers
 *
 * Authors:
 *  Manjeet Dahiya
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


#ifndef QXMPPOUTGOINGCLIENT_H
#define QXMPPOUTGOINGCLIENT_H

#include "QXmppClient.h"
#include "QXmppStanza.h"
#include "QXmppStream.h"

class QDomElement;
class QSslError;

class QXmppClient;
class QXmppConfiguration;
class QXmppPacket;
class QXmppPresence;
class QXmppIq;
class QXmppBind;
class QXmppRosterIq;
class QXmppVCardIq;
class QXmppMessage;
class QXmppRpcResponseIq;
class QXmppRpcErrorIq;
class QXmppArchiveChatIq;
class QXmppArchiveListIq;
class QXmppArchivePrefIq;
class QXmppByteStreamIq;
class QXmppDiscoveryIq;
class QXmppIbbCloseIq;
class QXmppIbbDataIq;
class QXmppIbbOpenIq;
class QXmppJingleIq;
class QXmppMucAdminIq;
class QXmppMucOwnerIq;
class QXmppStreamInitiationIq;
class QXmppVersionIq;

class QXmppOutgoingClientPrivate;

/// \brief The QXmppOutgoingClient class represents an outgoing XMPP stream
/// to an XMPP server.
///

class QXmppOutgoingClient : public QXmppStream
{
    Q_OBJECT

public:
    QXmppOutgoingClient(QObject *parent);
    ~QXmppOutgoingClient();

    void connectToHost();
    bool isConnected() const;

    QAbstractSocket::SocketError socketError();
    QXmppStanza::Error::Condition xmppStreamError();

    QXmppConfiguration& configuration();

    QXmppElementList presenceExtensions() const;

signals:
    void error(QXmppClient::Error);

    /// This signal is emitted when an element is received.
    void elementReceived(const QDomElement &element, bool &handled);
    void presenceReceived(const QXmppPresence&);
    void messageReceived(const QXmppMessage&);
    void iqReceived(const QXmppIq&);
    void rosterIqReceived(const QXmppRosterIq&);

    // XEP-0009: Jabber-RPC
    void rpcCallInvoke(const QXmppRpcInvokeIq &invoke);
    void rpcCallResponse(const QXmppRpcResponseIq& result);
    void rpcCallError(const QXmppRpcErrorIq &err);

    // XEP-0030: Service Discovery
    void discoveryIqReceived(const QXmppDiscoveryIq&);

    // XEP-0047 In-Band Bytestreams
    void ibbCloseIqReceived(const QXmppIbbCloseIq&);
    void ibbDataIqReceived(const QXmppIbbDataIq&);
    void ibbOpenIqReceived(const QXmppIbbOpenIq&);

    // XEP-0045: Multi-User Chat
    void mucAdminIqReceived(const QXmppMucAdminIq&);
    void mucOwnerIqReceived(const QXmppMucOwnerIq&);

    // XEP-0054: vcard-temp
    void vCardIqReceived(const QXmppVCardIq&);

    // XEP-0065: SOCKS5 Bytestreams
    void byteStreamIqReceived(const QXmppByteStreamIq&);

    // XEP-0095: Stream Initiation
    void streamInitiationIqReceived(const QXmppStreamInitiationIq&);

    // XEP-0136: Message Archiving
    void archiveChatIqReceived(const QXmppArchiveChatIq&);
    void archiveListIqReceived(const QXmppArchiveListIq&);
    void archivePrefIqReceived(const QXmppArchivePrefIq&);

    // XEP-0166: Jingle
    void jingleIqReceived(const QXmppJingleIq&);

protected:
    /// \cond
    // Overridable methods
    virtual void handleStart();
    virtual void handleStanza(const QDomElement &element);
    virtual void handleStream(const QDomElement &element);
    /// \endcond

private slots:
    void socketError(QAbstractSocket::SocketError);
    void socketSslErrors(const QList<QSslError>&);

    void pingStart();
    void pingStop();
    void pingSend();
    void pingTimeout();

private:
    QXmppDiscoveryIq capabilities() const;
    void sendAuthDigestMD5ResponseStep1(const QString& challenge);
    void sendAuthDigestMD5ResponseStep2(const QString& challenge);
    void sendNonSASLAuth(bool plaintext);
    void sendNonSASLAuthQuery();

    QXmppOutgoingClientPrivate * const d;
};

#endif // QXMPPOUTGOINGCLIENT_H
