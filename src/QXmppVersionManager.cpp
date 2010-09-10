/*
 * Copyright (C) 2008-2010 The QXmpp developers
 *
 * Author:
 *  Manjeet Dahiya
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

#include <QCoreApplication>
#include <QDomElement>

#include "QXmppVersionManager.h"
#include "QXmppOutgoingClient.h"
#include "QXmppVersionIq.h"
#include "QXmppGlobal.h"

bool QXmppVersionManager::handleStanza(QXmppStream *stream, const QDomElement &element)
{
    if (element.tagName() == "iq" && QXmppVersionIq::isVersionIq(element))
    {
        QXmppVersionIq versionIq;
        versionIq.parse(element);

        if(versionIq.type() == QXmppIq::Get)
        {
            // respond to query
            QXmppVersionIq responseIq;
            responseIq.setType(QXmppIq::Result);
            responseIq.setId(versionIq.id());
            responseIq.setTo(versionIq.from());

            QString name = qApp->applicationName();
            if(name.isEmpty())
                name = "Based on QXmpp";
            responseIq.setName(name);

            QString version = qApp->applicationVersion();
            if(version.isEmpty())
                version = QXmppVersion();
            responseIq.setVersion(version);

            // TODO set OS aswell
            stream->sendPacket(responseIq);
        }

        emit versionReceived(versionIq);
        return true;
    }

    return false;
}

void QXmppVersionManager::requestVersion(const QString& jid)
{
    QXmppVersionIq request;
    request.setType(QXmppIq::Get);
    request.setFrom(client()->configuration().jid());
    request.setTo(jid);
    client()->sendPacket(request);
}

