/*
 * Copyright (C) 2008-2010 QXmpp Developers
 *
 * Authors:
 *	Ian Reinhart Geiser
 *  Jeremy Lainé
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

#include <QBuffer>
#include <QDebug>

#include "QXmppMessage.h"
#include "QXmppUtils.h"

#include "xmppClient.h"

xmppClient::xmppClient(QObject *parent)
    : QXmppClient(parent)
{
    bool check = connect(this, SIGNAL(presenceReceived(QXmppPresence)),
                         this, SLOT(slotPresenceReceived(QXmppPresence)));
    Q_ASSERT(check);

    check = connect(&callManager(), SIGNAL(callReceived(QXmppCall*)),
                    this, SLOT(slotCallReceived(QXmppCall*)));
    Q_ASSERT(check);
}

/// A call was received.

void xmppClient::slotCallReceived(QXmppCall *call)
{
    qDebug() << "Got call from:" << call->jid();

    bool check = connect(call, SIGNAL(connected()), this, SLOT(slotConnected()));
    Q_ASSERT(check);

    check = connect(call, SIGNAL(finished()), this, SLOT(slotFinished()));
    Q_ASSERT(check);
}

/// A call connected.

void xmppClient::slotConnected()
{
    qDebug() << "Call connected";

    // FIXME : show how to use QAudioInput and QAudioOutput
}

/// A call finished.

void xmppClient::slotFinished()
{
    qDebug() << "Call finished";
}

/// A presence was received.

void xmppClient::slotPresenceReceived(const QXmppPresence &presence)
{
    const QLatin1String recipient("qxmpp.test2@gmail.com");

    // if we are the recipient, or if the presence is not from the recipient,
    // do nothing
    if (getConfiguration().jidBare() == recipient ||
        jidToBareJid(presence.from()) != recipient ||
        presence.type() != QXmppPresence::Available)
        return;

    // start the call and connect to the its signals
    QXmppCall *call = callManager().call(recipient);

    bool check = connect(job, SIGNAL(connected()), this, SLOT(slotConnected()));
    Q_ASSERT(check);

    check = connect(job, SIGNAL(finished()), this, SLOT(slotFinished()));
    Q_ASSERT(check);
}

