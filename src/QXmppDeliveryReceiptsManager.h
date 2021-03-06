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

#ifndef QXMPPDELIVERYRECEIPTSMANAGER_H
#define QXMPPDELIVERYRECEIPTSMANAGER_H

#include "QXmppClientExtension.h"

/// \brief The QXmppDeliveryReceiptsManager class makes it possible to
/// get message delivery receipts (as well as send them) as defined in
/// XEP-0184: Message Delivery Receipts.
///
/// \ingroup Managers

class QXmppDeliveryReceiptsManager : public QXmppClientExtension
{
    Q_OBJECT
public:
    QXmppDeliveryReceiptsManager();

    bool autoReceipt() const;
    void setAutoReceipt(bool);

    /// \cond
    virtual QStringList discoveryFeatures() const;
    virtual bool handleStanza(const QDomElement &stanza);
    /// \endcond

public slots:
    void sendReceipt(const QString &jid, const QString &id);

signals:
    /// This signal is emitted when receipt for the message with the
    /// given id is received. The id could be previously obtained by
    /// calling QXmppMessage::id().

    void messageDelivered(const QString &id);

private:
    bool m_autoReceipt;

};

#endif // QXMPPDELIVERYRECEIPTSMANAGER_H
