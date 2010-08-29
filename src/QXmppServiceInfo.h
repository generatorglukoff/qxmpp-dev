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

#ifndef QXMPPSERVICEINFO_H
#define QXMPPSERVICEINFO_H

#include <QList>
#include <QString>

/// \brief The QXmppServiceRecord class represents a DNS SRV record.
///

class QXmppServiceRecord
{
public:
    QXmppServiceRecord();

    QString hostName() const;
    void setHostName(const QString &hostName);

    quint16 port() const;
    void setPort(quint16 port);

private:
    QString host_name;
    quint16 host_port;
};

/// \brief The QXmppServiceInfo class provides static methods for DNS SRV lookups.
///

class QXmppServiceInfo
{
public:
    QString errorString() const;
    QList<QXmppServiceRecord> records() const;

    static QXmppServiceInfo fromName(const QString &dname);

private:
    QString m_errorString;
    QList<QXmppServiceRecord> m_records;
};

#endif
