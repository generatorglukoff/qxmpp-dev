/*
 * Copyright (C) 2008-2010 The QXmpp developers
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
#include "QXmppMessage.h"
#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "QXmppStream.h"
#include "QXmppUtils.h"

QXmppMucManager::QXmppMucManager(QXmppStream* stream, QObject *parent)
    : QObject(parent),
    m_stream(stream)
{
    bool check = connect(stream, SIGNAL(messageReceived(QXmppMessage)),
        this, SLOT(messageReceived(QXmppMessage)));
    Q_ASSERT(check);

    check = connect(stream, SIGNAL(mucAdminIqReceived(QXmppMucAdminIq)),
        this, SLOT(mucAdminIqReceived(QXmppMucAdminIq)));
    Q_ASSERT(check);

    check = connect(stream, SIGNAL(mucOwnerIqReceived(QXmppMucOwnerIq)),
        this, SLOT(mucOwnerIqReceived(QXmppMucOwnerIq)));
    Q_ASSERT(check);

    check = QObject::connect(m_stream, SIGNAL(presenceReceived(QXmppPresence)),
        this, SLOT(presenceReceived(QXmppPresence)));
    Q_ASSERT(check);
}

/// Joins the given chat room with the requested nickname.
///
/// \param roomJid
/// \param nickName
///
/// \return true if the request was sent, false otherwise
///

bool QXmppMucManager::joinRoom(const QString &roomJid, const QString &nickName)
{
    QXmppPresence packet;
    packet.setTo(roomJid + "/" + nickName);
    packet.setType(QXmppPresence::Available);
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_muc);
    packet.setExtensions(x);
    if (m_stream->sendPacket(packet))
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
    return m_stream->sendPacket(packet);
}

/// Retrieves the list of participants for the given room.
///
/// \param roomJid
///

QMap<QString, QXmppPresence> QXmppMucManager::roomParticipants(const QString& roomJid) const
{
    return m_participants.value(roomJid);
}

/// Request the configuration form for the given room.
///
/// \param roomJid
///
/// \return true if the request was sent, false otherwise
///
/// \sa roomConfigurationReceived()
///

bool QXmppMucManager::requestRoomConfiguration(const QString &roomJid)
{
    QXmppMucOwnerIq iq;
    iq.setTo(roomJid);
    return m_stream->sendPacket(iq);
}

/// Send the configuration form for the given room.
///
/// \param roomJid
/// \param form
///
/// \return true if the request was sent, false otherwise
///

bool QXmppMucManager::setRoomConfiguration(const QString &roomJid, const QXmppDataForm &form)
{
    QXmppMucOwnerIq iqPacket;
    iqPacket.setType(QXmppIq::Set);
    iqPacket.setTo(roomJid);
    iqPacket.setForm(form);
    return m_stream->sendPacket(iqPacket);
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
    return m_stream->sendPacket(message);
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
    msg.setFrom(roomJid + "/" + m_nickNames[roomJid]);
    msg.setTo(roomJid);
    msg.setType(QXmppMessage::GroupChat);
    return m_stream->sendPacket(msg);
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
    Q_UNUSED(iq);
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

    emit roomParticipantChanged(bareJid, resource);
}

