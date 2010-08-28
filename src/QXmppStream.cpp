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


#include "QXmppConstants.h"
#include "QXmppLogger.h"
#include "QXmppPacket.h"
#include "QXmppStream.h"
#include "QXmppUtils.h"

#include <QBuffer>
#include <QDomDocument>
#include <QHostAddress>
#include <QRegExp>
#include <QSslSocket>
#include <QStringList>
#include <QTime>
#include <QXmlStreamWriter>

static bool randomSeeded = false;
static const QString capabilitiesNode = "http://code.google.com/p/qxmpp";
static const QByteArray streamRootElementEnd = "</stream:stream>";

class QXmppStreamPrivate
{
public:
    QXmppStreamPrivate();

    QByteArray dataBuffer;
    QXmppLogger* logger;
    QSslSocket* socket;

    // stream state
    QByteArray streamStart;
};

QXmppStreamPrivate::QXmppStreamPrivate()
    : logger(0),
    socket(0)
{
}

/// Constructs a base XMPP stream.
///
/// \param parent

QXmppStream::QXmppStream(QObject *parent)
    : QObject(parent),
    d(new QXmppStreamPrivate)
{
    // Make sure the random number generator is seeded
    if (!randomSeeded)
    {
        qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
        randomSeeded = true;
    }
}

/// Destroys a base XMPP stream.

QXmppStream::~QXmppStream()
{
    delete d;
}

/// Disconnects from the remote host.
///

void QXmppStream::disconnectFromHost()
{
    sendData(streamRootElementEnd);
    if (d->socket)
    {
        d->socket->flush();
        d->socket->disconnectFromHost();
    }
}

void QXmppStream::handleStart()
{
}

/// Returns the QXmppLogger associated with the current stream.

QXmppLogger *QXmppStream::logger()
{
    return d->logger;
}

/// Sets the QXmppLogger associated with the current stream.

void QXmppStream::setLogger(QXmppLogger *logger)
{
    if (d->logger)
        disconnect(this, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                   d->logger, SLOT(log(QXmppLogger::MessageType, QString)));
    d->logger = logger;
    if (d->logger)
        connect(this, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                d->logger, SLOT(log(QXmppLogger::MessageType, QString)));
}

void QXmppStream::debug(const QString &data)
{
    emit logMessage(QXmppLogger::DebugMessage, data);
}

void QXmppStream::info(const QString &data)
{
    emit logMessage(QXmppLogger::InformationMessage, data);
}

void QXmppStream::warning(const QString &data)
{
    emit logMessage(QXmppLogger::WarningMessage, data);
}

/// Returns true if the stream is connected.
///

bool QXmppStream::isConnected() const
{
    return d->socket &&
           d->socket->state() == QAbstractSocket::ConnectedState;
}

/// Sends raw data to the peer.
///
/// \param data

bool QXmppStream::sendData(const QByteArray &data)
{
    emit logMessage(QXmppLogger::SentMessage, QString::fromUtf8(data));
    if (!d->socket || d->socket->state() != QAbstractSocket::ConnectedState)
        return false;
    return d->socket->write(data) == data.size();
}

/// Sends an XML element to the peer.
///
/// \param element

bool QXmppStream::sendElement(const QDomElement &element)
{
    // prepare packet
    QByteArray data;
    QXmlStreamWriter xmlStream(&data);
    const QStringList omitNamespaces = QStringList() << ns_client << ns_server;
    helperToXmlAddDomElement(&xmlStream, element, omitNamespaces);

    // send packet
    return sendData(data);
}

/// Sends an XMPP packet to the peer.
///
/// \param packet

bool QXmppStream::sendPacket(const QXmppPacket &packet)
{
    // prepare packet
    QByteArray data;
    QXmlStreamWriter xmlStream(&data);
    packet.toXml(&xmlStream);

    // send packet
    return sendData(data);
}

/// Returns the QSslSocket used for this stream.
///

QSslSocket *QXmppStream::socket()
{
    return d->socket;
}

/// Sets the QSslSocket used for this stream.
///

void QXmppStream::setSocket(QSslSocket *socket)
{
    d->socket = socket;

    // socket events
    bool check = connect(socket, SIGNAL(connected()),
                    this, SLOT(socketConnected()));
    Q_ASSERT(check);

    check = connect(socket, SIGNAL(disconnected()),
                    this, SLOT(socketDisconnected()));
    Q_ASSERT(check);

    check = connect(socket, SIGNAL(encrypted()),
                    this, SLOT(socketEncrypted()));
    Q_ASSERT(check);

    check = connect(socket, SIGNAL(readyRead()),
                    this, SLOT(socketReadyRead()));
    Q_ASSERT(check);

    // relay signals
    check = connect(socket, SIGNAL(disconnected()),
                    this, SIGNAL(disconnected()));
    Q_ASSERT(check);
}

void QXmppStream::socketConnected()
{
    info(QString("Socket connected to %1 %2").arg(
        socket()->peerAddress().toString(),
        QString::number(socket()->peerPort())));
    d->dataBuffer.clear();
    handleStart();
}

void QXmppStream::socketDisconnected()
{
    info("Socket disconnected");
    d->dataBuffer.clear();
}

void QXmppStream::socketEncrypted()
{
    debug("Socket encrypted");
    d->dataBuffer.clear();
    handleStart();
}

void QXmppStream::socketReadyRead()
{
    const QByteArray data = socket()->readAll();
    //debug("SERVER [COULD BE PARTIAL DATA]:" + data.left(20));

    d->dataBuffer.append(data);

    // FIXME : maybe these QRegExps could be static?
    QRegExp startStreamRegex("^(<\\?xml.*\\?>)?\\s*<stream:stream.*>");
    startStreamRegex.setMinimal(true);
    QRegExp endStreamRegex("</stream:stream>$");
    endStreamRegex.setMinimal(true);

    // check whether we need to add stream start / end elements
    QByteArray completeXml = d->dataBuffer;
    const QString strData = QString::fromUtf8(d->dataBuffer);
    bool streamStart = false;
    if(strData.contains(startStreamRegex))
    {
        streamStart = true;
        d->streamStart = startStreamRegex.cap(0).toUtf8();
    }
    else
        completeXml.prepend(d->streamStart);
    if(!strData.contains(endStreamRegex))
        completeXml.append(streamRootElementEnd);

    // check whether we have a valid XML document
    QDomDocument doc;
    if(!doc.setContent(completeXml, true))
        return;

    // remove data from buffer
    emit logMessage(QXmppLogger::ReceivedMessage, strData);
    d->dataBuffer.clear();

    // process stream start
    QDomElement nodeRecv = doc.documentElement().firstChildElement();
    if (streamStart)
    {
        QDomElement streamElement = doc.documentElement();
        handleStream(streamElement);
    }

    // process stanzas
    while(!nodeRecv.isNull())
    {
        handleStanza(nodeRecv);
        nodeRecv = nodeRecv.nextSiblingElement();
    }
}


