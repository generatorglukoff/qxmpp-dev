/*
 * Copyright (C) 2008-2011 The QXmpp developers
 *
 * Author:
 *  Georg Rudoy
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

#include "QXmppDeliveryReceiptsManager.h"

#include <QDomElement>

#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppClient.h"

QXmppDeliveryReceiptsManager::QXmppDeliveryReceiptsManager() : QXmppClientExtension(),
    m_autoReceipt(true)
{
}

bool QXmppDeliveryReceiptsManager::autoReceipt() const
{
    return m_autoReceipt;
}

void QXmppDeliveryReceiptsManager::setAutoReceipt(bool autoReceipt)
{
    m_autoReceipt = autoReceipt;
}

void QXmppDeliveryReceiptsManager::sendReceipt(const QString &jid, const QString &id)
{
    QXmppMessage msg;
    msg.setTo(jid);

    QXmppElement elem;
    elem.setTagName("received");
    elem.setAttribute("xlmns", ns_message_receipts);
    elem.setAttribute("id", id);

    msg.setExtensions(QXmppElementList(elem));

    client()->sendPacket(msg);
}

QStringList QXmppDeliveryReceiptsManager::discoveryFeatures() const
{
    return QStringList(ns_message_receipts);
}

bool QXmppDeliveryReceiptsManager::handleStanza(const QDomElement &stanza)
{
    if (stanza.tagName() != "message")
        return false;

    // Case 1: incoming receipt
    // This way we handle the receipt and cancel any further processing.
    const QDomElement &received = stanza.firstChildElement("received");
    if (received.namespaceURI() == ns_message_receipts)
    {
        QString id = received.attribute("id");

        // check if it's old-style XEP
        if (id.isEmpty())
            id = stanza.attribute("id");

        emit messageDelivered(id);
        return true;
    }

    // Case 2: incoming message requesting receipt
    // If autoreceipt is enabled, we queue sending back receipt, otherwise
    // we just ignore the message. In either case, we don't cancel any
    // further processing.
    if (m_autoReceipt && stanza.firstChildElement("request").namespaceURI() == ns_message_receipts)
    {
        const QString &jid = stanza.attribute("from");
        const QString &id = stanza.attribute("id");

        // Send out receipt only if jid and id is not empty, otherwise fail
        // silently.
        if (!jid.isEmpty() && !id.isEmpty())
            QMetaObject::invokeMethod(this,
                    "sendReceipt",
                    Q_ARG(QString, jid),
                    Q_ARG(QString, id));
    }

    return false;
}
