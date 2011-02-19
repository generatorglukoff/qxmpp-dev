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

#include <QDomElement>

#include "QXmppClient.h"
#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "QXmppUtils.h"

void QXmppMucManager::setClient(QXmppClient* client)
{
    QXmppClientExtension::setClient(client);

    bool check = connect(client, SIGNAL(messageReceived(QXmppMessage)),
        this, SLOT(messageReceived(QXmppMessage)));
    Q_ASSERT(check);

    check = QObject::connect(client, SIGNAL(presenceReceived(QXmppPresence)),
        this, SLOT(presenceReceived(QXmppPresence)));
    Q_ASSERT(check);
}

QStringList QXmppMucManager::discoveryFeatures() const
{
    // XEP-0045: Multi-User Chat
    return QStringList()
        << ns_muc
        << ns_muc_admin
        << ns_muc_owner
        << ns_muc_user;
}

bool QXmppMucManager::handleStanza(const QDomElement &element)
{
    if (element.tagName() == "iq")
    {
        if (QXmppMucAdminIq::isMucAdminIq(element))
        {
            QXmppMucAdminIq iq;
            iq.parse(element);
            mucAdminIqReceived(iq);
            return true;
        }
        else if (QXmppMucOwnerIq::isMucOwnerIq(element))
        {
            QXmppMucOwnerIq iq;
            iq.parse(element);
            mucOwnerIqReceived(iq);
            return true;
        }
    }
    return false;
}

/// Joins the given chat room with the requested nickname.
///
/// \param roomJid
/// \param nickName
/// \param password an optional password if the room is password-protected
///
/// \return true if the request was sent, false otherwise
///

bool QXmppMucManager::joinRoom(const QString &roomJid, const QString &nickName, const QString &password)
{
    QXmppPresence packet = client()->clientPresence();
    packet.setTo(roomJid + "/" + nickName);
    packet.setType(QXmppPresence::Available);
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_muc);
    if (!password.isEmpty())
    {
        QXmppElement p;
        p.setTagName("password");
        p.setValue(password);
        x.appendChild(p);
    }
    packet.setExtensions(x);
    if (client()->sendPacket(packet))
    {
        m_nickNames[roomJid] = nickName;
        return true;
    } else {
        return false;
    }
}

/// Leaves the given chat room.
///
/// \param roomJid
///
/// \return true if the request was sent, false otherwise
///

bool QXmppMucManager::leaveRoom(const QString &roomJid)
{
    if (!m_nickNames.contains(roomJid))
        return false;
    QString nickName = m_nickNames.take(roomJid);
    QXmppPresence packet;
    packet.setTo(roomJid + "/" + nickName);
    packet.setType(QXmppPresence::Unavailable);
    return client()->sendPacket(packet);
}

/// Retrieves the list of participants for the given room.
///
/// \param roomJid
///

QMap<QString, QXmppPresence> QXmppMucManager::roomParticipants(const QString& roomJid) const
{
    return m_participants.value(roomJid);
}

/// Retrieves the affiliation of the participant for the given room.
///
/// If there is no such participant in the room, or if the affiliation is unknown,
/// UnspecifiedAffiliation is returned.
///
/// \param roomJid
/// \param nick
///

QXmppMucAdminIq::Item::Affiliation QXmppMucManager::getAffiliation(const QString &roomJid, const QString &nick) const
{
    return m_affiliations[roomJid].value(nick, QXmppMucAdminIq::Item::UnspecifiedAffiliation);
}

/// Retrieves the role of the participant for the given room.
///
/// If there is no such participant in the room, or if the role is unknown,
/// UnspecifiedRole is returned.
///
/// \param roomJid
/// \param nick
///

QXmppMucAdminIq::Item::Role QXmppMucManager::getRole(const QString &roomJid, const QString &nick) const
{
    return m_roles[roomJid].value(nick, QXmppMucAdminIq::Item::UnspecifiedRole);
}

/// Retrieves the real JID of the participant for the given room.
///
/// If there is no such participant, or if the combination of room's configuration
/// and our user's rights is insufficient to obtain real JIDs, an empty string is
/// returned.
///
/// \param roomJid
/// \param nick
///

QString QXmppMucManager::getRealJid(const QString &roomJid, const QString &nick) const
{
    return m_realJids[roomJid].value(nick);
}

/// Request the configuration form for the given room.
///
/// \param roomJid
///
/// \return true if the request was sent, false otherwise
///
/// \sa roomConfigurationReceived()

bool QXmppMucManager::requestRoomConfiguration(const QString &roomJid)
{
    QXmppMucOwnerIq iq;
    iq.setTo(roomJid);
    return client()->sendPacket(iq);
}

/// Send the configuration form for the given room.
///
/// \param roomJid
/// \param form
///
/// \return true if the request was sent, false otherwise

bool QXmppMucManager::setRoomConfiguration(const QString &roomJid, const QXmppDataForm &form)
{
    QXmppMucOwnerIq iqPacket;
    iqPacket.setType(QXmppIq::Set);
    iqPacket.setTo(roomJid);
    iqPacket.setForm(form);
    return client()->sendPacket(iqPacket);
}

/// Request the room's permissions.
///
/// \param roomJid
///
/// \return true if the request was sent, false otherwise

bool QXmppMucManager::requestRoomPermissions(const QString &roomJid)
{
    QList<QXmppMucAdminIq::Item::Affiliation> affiliations;
    affiliations << QXmppMucAdminIq::Item::OwnerAffiliation;
    affiliations << QXmppMucAdminIq::Item::AdminAffiliation;
    affiliations << QXmppMucAdminIq::Item::MemberAffiliation;
    affiliations << QXmppMucAdminIq::Item::OutcastAffiliation;
    foreach (QXmppMucAdminIq::Item::Affiliation affiliation, affiliations)
    {
        QXmppMucAdminIq::Item item;
        item.setAffiliation(affiliation);

        QXmppMucAdminIq iq;
        iq.setTo(roomJid);
        iq.setItems(QList<QXmppMucAdminIq::Item>() << item);
        if (!client()->sendPacket(iq))
            return false;
    }
    return true;
}

/// Sets the subject for the given room.
///
/// \param roomJid
/// \param subject
///
/// \return true if the request was sent, false otherwise
///

bool QXmppMucManager::setRoomSubject(const QString &roomJid, const QString &subject)
{
    QXmppMessage msg;
    msg.setTo(roomJid);
    msg.setType(QXmppMessage::GroupChat);
    msg.setSubject(subject);
    return  client()->sendPacket(msg);
}

/// Invite a user to a chat room.
///
/// \param roomJid
/// \param jid
/// \param reason
///
/// \return true if the message was sent, false otherwise
///

bool QXmppMucManager::sendInvitation(const QString &roomJid, const QString &jid, const QString &reason)
{
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_conference);
    x.setAttribute("jid", roomJid);
    x.setAttribute("reason", reason);

    QXmppMessage message;
    message.setTo(jid);
    message.setType(QXmppMessage::Normal);
    message.setExtensions(x);
    return client()->sendPacket(message);
}

/// Send a message to a chat room.
///
/// \param roomJid
/// \param text
///
/// \return true if the message was sent, false otherwise
///

bool QXmppMucManager::sendMessage(const QString &roomJid, const QString &text)
{
    if (!m_nickNames.contains(roomJid))
    {
        qWarning("Cannot send message to unknown chat room");
        return false;
    }
    QXmppMessage msg;
    msg.setBody(text);
    msg.setTo(roomJid);
    msg.setType(QXmppMessage::GroupChat);
    return client()->sendPacket(msg);
}

void QXmppMucManager::messageReceived(const QXmppMessage &msg)
{
    if (msg.type() != QXmppMessage::Normal)
        return;

    // process room invitations
    foreach (const QXmppElement &extension, msg.extensions())
    {
        if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_conference)
        {
            const QString roomJid = extension.attribute("jid");
            if (!roomJid.isEmpty() && !m_nickNames.contains(roomJid))
                emit invitationReceived(roomJid, msg.from(), extension.attribute("reason"));
            break;
        }
    }
}

void QXmppMucManager::mucAdminIqReceived(const QXmppMucAdminIq &iq)
{
    if (iq.type() != QXmppIq::Result) 
        return;
    emit roomPermissionsReceived(iq.from(), iq.items());
}

void QXmppMucManager::mucOwnerIqReceived(const QXmppMucOwnerIq &iq)
{
    if (iq.type() == QXmppIq::Result && !iq.form().isNull())
        emit roomConfigurationReceived(iq.from(), iq.form());
}

void QXmppMucManager::presenceReceived(const QXmppPresence &presence)
{
    QString jid = presence.from();
    QString bareJid = jidToBareJid(jid);
    QString resource = jidToResource(jid);
    if (!m_nickNames.contains(bareJid))
        return;

    if (presence.type() == QXmppPresence::Available)
        m_participants[bareJid][resource] = presence;
    else if (presence.type() == QXmppPresence::Unavailable)
        m_participants[bareJid].remove(resource);
    else
        return;

    // Would be also used to check if the item is going to change its nick.
    QString newNick;

    QXmppMucAdminIq::Item::Affiliation newAffiliation = QXmppMucAdminIq::Item::UnspecifiedAffiliation;
    QXmppMucAdminIq::Item::Role newRole = QXmppMucAdminIq::Item::UnspecifiedRole;
    QString realJid;
    QString reason;

    bool beenKicked = false;
    foreach (const QXmppElement &extension, presence.extensions())
    {
        if (!(extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc_user))
            continue;

        const QXmppElement &itemElem = extension.firstChildElement("item");
        QXmppElement status = extension.firstChildElement("status");
        while (!status.isNull())
        {
            const int code = status.attribute("code").toInt();
            if (code == 303)
            {
                newNick = itemElem.attribute("nick");
                emit roomParticipantNickChanged(bareJid, resource, newNick);
            }
            else if (code == 307)
            {
                beenKicked = true;
                newRole = QXmppMucAdminIq::Item::NoRole;
            }
            else if (code == 301)
            {
                beenKicked = true;
                newAffiliation = QXmppMucAdminIq::Item::OutcastAffiliation;
            }
            status = status.nextSiblingElement("status");
        }

        if (!newNick.isEmpty() || presence.type() != QXmppPresence::Unavailable)
        {
            newAffiliation = QXmppMucAdminIq::Item::affiliationFromString(itemElem.attribute("affiliation"));
            newRole = QXmppMucAdminIq::Item::roleFromString(itemElem.attribute("role"));
            realJid = itemElem.attribute("jid");
        }
        reason = itemElem.firstChildElement("reason").value();
    }

    bool rightsChanged = false;
    if (presence.type() == QXmppPresence::Unavailable)
    {
        // If newNick is empty, then user really left the room, otherwise he just
        // changed his nickname, so copy data from old nick into new one.
        if (newAffiliation == QXmppMucAdminIq::Item::OutcastAffiliation ||
            newRole == QXmppMucAdminIq::Item::NoRole)
        {
            rightsChanged = true;
            m_realJids[bareJid].remove(resource);
            m_affiliations[bareJid].remove(resource);
            m_roles[bareJid].remove(resource);
        }
        else if (newNick.isEmpty())
        {
            m_realJids[bareJid].remove(resource);
            m_affiliations[bareJid].remove(resource);
            m_roles[bareJid].remove(resource);
        }
        else
        {
            m_realJids[bareJid][newNick] = m_realJids[bareJid].take(resource);
            m_affiliations[bareJid][newNick] = m_affiliations[bareJid].take(resource);
            m_roles[bareJid][newNick] = m_roles[bareJid].take(resource);
        }
    }
    else
    {
        if (m_realJids[bareJid][resource] != realJid)
        {
            m_realJids[bareJid][resource] = realJid;
            emit roomParticipantJidChanged(bareJid, resource, realJid);
        }
        if (m_affiliations[bareJid].value(resource) != newAffiliation ||
            m_roles[bareJid].value(resource) != newRole)
        {
            // Rights have been changed only if there were old ones.
            rightsChanged = m_affiliations[bareJid].contains(resource);
            m_affiliations[bareJid][resource] = newAffiliation;
            m_roles[bareJid][resource] = newRole;
        }
    }

    if (!beenKicked)
        emit roomPresenceChanged(bareJid, resource, presence);
    if (rightsChanged)
        emit roomParticipantPermsChanged(bareJid, resource, newAffiliation, newRole, reason);

    emit roomParticipantChanged(bareJid, resource);
}

