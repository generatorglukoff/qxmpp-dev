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


#ifndef QXMPPVCARDMANAGER_H
#define QXMPPVCARDMANAGER_H

#include <QObject>

#include "QXmppVCard.h"

class QXmppStream;

/// \brief The QXmppVCardManager gets/sets XMPP vCards. It is an
/// implentation of <B>XEP-0054: vcard-temp</B>.
///
/// \note It's object should not be created using it's constructor. Instead
/// QXmppClient::vCardManager() should be used to get the reference of instantiated
/// object this class.
///
/// Getting vCards of entries in Roster:
/// It doesn't store vCards of the JIDs in the roster of connected user. Instead
/// client has to request for a particular vCard using requestVCard(). And connect to
/// the signal vCardReceived() to get the requested vCard.
///
/// Getting vCard of the connected client:
/// For getting the vCard of the connected user itself. Client can call requestClientVCard()
/// and on the signal clientVCardReceived() it can get its vCard using clientVCard().
///
/// Settting vCard of the client:
/// Using setClientVCard() client can set its vCard.
///
/// \note Client can't set set/change vCards of roster entries.
///
/// \ingroup Managers

class QXmppVCardManager : public QObject
{
    Q_OBJECT

public:
    QXmppVCardManager(QXmppStream* stream, QObject *parent = 0);
    void requestVCard(const QString& bareJid = "");

    const QXmppVCard& clientVCard() const;
    void setClientVCard(const QXmppVCard&);
    void requestClientVCard();
    bool isClientVCardReceived();

signals:
    void vCardReceived(const QXmppVCard&);
    void clientVCardReceived();

private slots:
    void vCardIqReceived(const QXmppVCard&);

private:
    // reference to the xmpp stream (no ownership)
    QXmppStream* m_stream;

    QXmppVCard m_clientVCard;  ///< Stores the vCard of the connected client
    bool m_isClientVCardReceived;
};

#endif // QXMPPVCARDMANAGER_H
