/*
 * Copyright (C) 2008-2011 The QXmpp developers
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

#ifndef QXMPPMUCMANAGER_H
#define QXMPPMUCMANAGER_H

#include <QMap>

#include "QXmppClientExtension.h"
#include "QXmppMucIq.h"
#include "QXmppPresence.h"

class QXmppDataForm;
class QXmppMessage;
class QXmppMucAdminIq;
class QXmppMucOwnerIq;

/// \brief The QXmppMucManager class makes it possible to interact with
/// multi-user chat rooms as defined by XEP-0045: Multi-User Chat.
///
/// To make use of this manager, you need to instantiate it and load it into
/// the QXmppClient instance as follows:
///
/// \code
/// QXmppMucManager *manager = new QXmppMucManager;
/// client->addExtension(manager);
/// \endcode
///
/// \ingroup Managers

class QXmppMucManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    bool joinRoom(const QString &roomJid, const QString &nickName, const QString &password = QString());
    bool leaveRoom(const QString &roomJid);

    bool requestRoomConfiguration(const QString &roomJid);
    bool setRoomConfiguration(const QString &roomJid, const QXmppDataForm &form);

    bool requestRoomPermissions(const QString &roomJid);

    bool setRoomSubject(const QString &roomJid, const QString &subject);

    bool sendInvitation(const QString &roomJid, const QString &jid, const QString &reason);
    bool sendMessage(const QString &roomJid, const QString &text);

    QMap<QString, QXmppPresence> roomParticipants(const QString& bareJid) const;

    QXmppMucAdminIq::Item::Affiliation getAffiliation(const QString &roomJid, const QString &nick) const;
    QXmppMucAdminIq::Item::Role getRole(const QString &roomJid, const QString &nick) const;
    QString getRealJid(const QString &roomJid, const QString &nick) const;

    /// \cond
    QStringList discoveryFeatures() const;
    bool handleStanza(const QDomElement &element);
    /// \endcond

signals:
    /// This signal is emitted when an invitation to a chat room is received.
    void invitationReceived(const QString &roomJid, const QString &inviter, const QString &reason);

    /// This signal is emitted when the configuration form for a chat room is received.
    void roomConfigurationReceived(const QString &roomJid, const QXmppDataForm &configuration);

    /// This signal is emitted when the permissions for a chat room are received.
    void roomPermissionsReceived(const QString &roomJid, const QList<QXmppMucAdminIq::Item> &permissions);

    /// This signal is emitted when a room participant's presence changed.
    ///
    /// \sa roomParticipants()
    void roomParticipantChanged(const QString &roomJid, const QString &nickName);

    /// This signal is emitted when a room participant changes his nickname.
    ///
    /// Please note that roomParticipantChanged() would be emitted nevertheless,
    /// both to signal about the unavailability of the participant under oldNick
    /// and about the availability under newNick. However, this signal would be
    /// emitted before both of them, so you may keep track of nick changes and
    /// react to roomParticipantChanged() accordingly.
    void roomParticipantNickChanged(const QString &roomJid, const QString &oldNick, const QString &newNick);

    /// This signal is emitted when a room participant's JID changes.
    void roomParticipantJidChanged(const QString &roomJid, const QString &nick, const QString &newJid);

    /// This signal is emitted when a room participant's permissions change.
    ///
    /// Please note that this also includes the events of kicking and banning,
    /// in this case newRole or newAff change to NoRole and OutcastAffiliation
    /// accordingly.
    void roomParticipantPermsChanged(const QString &roomJid, const QString &nick,
                                     QXmppMucAdminIq::Item::Affiliation newAff,
                                     QXmppMucAdminIq::Item::Role newRole,
                                     const QString &reason);

    /// This signal is emitted when a room participant's presence changes.
    ///
    /// This signal is emitted after roomParticipantJidChanged(), if any, but before
    /// roomParticipantPermsChanged() and roomParticipandChanged().
    void roomPresenceChanged(const QString &roomJid, const QString &nick, const QXmppPresence &presence);

protected:
    /// \cond
    void setClient(QXmppClient* client);
    /// \endcond

private slots:
    void messageReceived(const QXmppMessage &message);
    void mucAdminIqReceived(const QXmppMucAdminIq &iq);
    void mucOwnerIqReceived(const QXmppMucOwnerIq &iq);
    void presenceReceived(const QXmppPresence &presence);

private:
    QMap<QString, QString> m_nickNames;
    QMap<QString, QMap<QString, QXmppPresence> > m_participants;
    QMap<QString, QMap<QString, QXmppMucAdminIq::Item::Affiliation> > m_affiliations;
    QMap<QString, QMap<QString, QXmppMucAdminIq::Item::Role> > m_roles;
    QMap<QString, QMap<QString, QString> > m_realJids;
};

#endif
