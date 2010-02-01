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

#include "QXmppConstants.h"
#include "QXmppPingIq.h"
#include "QXmppUtils.h"

#include <QDomElement>

QXmppPingIq::QXmppPingIq() : QXmppIq(QXmppIq::Get)
{
}

bool QXmppPingIq::isPingIq( QDomElement &element )
{
    QDomElement pingElement = element.firstChildElement("ping");
    return (pingElement.namespaceURI() == ns_ping);
}

void QXmppPingIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("ping");
    helperToXmlAddAttribute(writer, "xmlns", ns_ping);
    writer->writeEndElement();
}

