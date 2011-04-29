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
#include <QMap>

#include "QXmppClient.h"
#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "QXmppUtils.h"

class QXmppMucManagerPrivate
{
public:
    QMap<QString, QXmppMucRoom*> rooms;
};

class QXmppMucRoomPrivate
{
public:
    QString ownJid() const { return jid + "/" + nickName; }
    QXmppClient *client;
    QXmppMucRoom::Actions allowedActions;
    QString jid;
    QMap<QString, QXmppPresence> participants;
    QString password;
    QMap<QString, QXmppMucAdminIq::Item::Affiliation> affiliations;
    QString nickName;
    QXmppPresence::Status status;
    QString subject;
};

/// Constructs a new QXmppMucManager.

QXmppMucManager::QXmppMucManager()
{
    d = new QXmppMucManagerPrivate;
}


/// Destroys a new QXmppMucManager.

QXmppMucManager::~QXmppMucManager()
{
    delete d;
}

/// Adds the given chat room to the set of manged rooms.
///
/// \param roomJid

QXmppMucRoom *QXmppMucManager::addRoom(const QString &roomJid)
{
    QXmppMucRoom *room = d->rooms.value(roomJid);
    if (!room) {
        room = new QXmppMucRoom(client(), roomJid, this);
        d->rooms.insert(roomJid, room);
    }
    return room;
}

void QXmppMucManager::setClient(QXmppClient* client)
{
    QXmppClientExtension::setClient(client);

    bool check = connect(client, SIGNAL(messageReceived(QXmppMessage)),
        this, SLOT(_q_messageReceived(QXmppMessage)));
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

            QXmppMucRoom *room = d->rooms.value(iq.from());
            if (room && iq.type() == QXmppIq::Result) {
                foreach (const QXmppMucAdminIq::Item &item, iq.items()) {
                    const QString jid = item.jid();
                    if (!room->d->affiliations.contains(jid))
                        room->d->affiliations.insert(jid, item.affiliation());
                }
                emit room->permissionsReceived(iq.items());
            }
            return true;
        }
        else if (QXmppMucOwnerIq::isMucOwnerIq(element))
        {
            QXmppMucOwnerIq iq;
            iq.parse(element);

            QXmppMucRoom *room = d->rooms.value(iq.from());
            if (room && iq.type() == QXmppIq::Result && !iq.form().isNull())
                emit room->configurationReceived(iq.form());
            return true;
        }
    }
    return false;
}

void QXmppMucManager::_q_messageReceived(const QXmppMessage &msg)
{
    if (msg.type() != QXmppMessage::Normal)
        return;

    // process room invitations
    foreach (const QXmppElement &extension, msg.extensions())
    {
        if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_conference)
        {
            const QString roomJid = extension.attribute("jid");
            if (!roomJid.isEmpty() && !d->rooms.contains(roomJid))
                emit invitationReceived(roomJid, msg.from(), extension.attribute("reason"));
            break;
        }
    }
}

/// Constructs a new QXmppMucRoom.
///
/// \param parent

QXmppMucRoom::QXmppMucRoom(QXmppClient *client, const QString &jid, QObject *parent)
    : QObject(parent)
{
    bool check;

    d = new QXmppMucRoomPrivate;
    d->allowedActions = NoAction;
    d->client = client;
    d->jid = jid;

    check = connect(d->client, SIGNAL(disconnected()),
                    this, SLOT(_q_disconnected()));
    Q_ASSERT(check);

    check = connect(d->client, SIGNAL(messageReceived(QXmppMessage)),
                    this, SLOT(_q_messageReceived(QXmppMessage)));
    Q_ASSERT(check);

    check = connect(d->client, SIGNAL(presenceReceived(QXmppPresence)),
                    this, SLOT(_q_presenceReceived(QXmppPresence)));
    Q_ASSERT(check);
}

/// Destroys a QXmppMucRoom.

QXmppMucRoom::~QXmppMucRoom()
{
    delete d;
}

/// Returns the actions you are allowed to perform on the room.

QXmppMucRoom::Actions QXmppMucRoom::allowedActions() const
{
    return d->allowedActions;
}

/// Returns true if you are currently in the room.

bool QXmppMucRoom::isJoined() const
{
    return d->participants.contains(d->ownJid());
}

/// Returns the chat room's bare JID.

QString QXmppMucRoom::jid() const
{
    return d->jid;
}

/// Joins the chat room.
///
/// \return true if the request was sent, false otherwise

bool QXmppMucRoom::join()
{
    if (isJoined() || d->nickName.isEmpty())
        return false;

    QXmppPresence packet;
    packet.setTo(d->ownJid());
    packet.setType(QXmppPresence::Available);
    packet.setStatus(d->status);
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_muc);
    if (!d->password.isEmpty())
    {
        QXmppElement p;
        p.setTagName("password");
        p.setValue(d->password);
        x.appendChild(p);
    }
    packet.setExtensions(x);
    return d->client->sendPacket(packet);
}

/// Kicks the specified user from the chat room.
///
/// \param jid
///
/// \return true if the request was sent, false otherwise

bool QXmppMucRoom::kick(const QString &jid, const QString &reason)
{
    QXmppMucAdminIq::Item item;
    item.setNick(jidToResource(jid));
    item.setRole(QXmppMucAdminIq::Item::NoRole);
    item.setReason(reason);

    QXmppMucAdminIq iq;
    iq.setType(QXmppIq::Set);
    iq.setTo(d->jid);
    iq.setItems(QList<QXmppMucAdminIq::Item>() << item);

    return d->client->sendPacket(iq);
}

/// Leaves the chat room.
///
/// \return true if the request was sent, false otherwise

bool QXmppMucRoom::leave()
{
    QXmppPresence packet;
    packet.setTo(d->ownJid());
    packet.setType(QXmppPresence::Unavailable);
    return d->client->sendPacket(packet);
}

/// Returns your own nickname.

QString QXmppMucRoom::nickName() const
{
    return d->nickName;
}

/// Invites a user to the chat room.
///
/// \param jid
/// \param reason
///
/// \return true if the request was sent, false otherwise

bool QXmppMucRoom::sendInvitation(const QString &jid, const QString &reason)
{
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_conference);
    x.setAttribute("jid", d->jid);
    x.setAttribute("reason", reason);

    QXmppMessage message;
    message.setTo(jid);
    message.setType(QXmppMessage::Normal);
    message.setExtensions(x);
    return d->client->sendPacket(message);
}

/// Sends a message to the room.
///
/// \return true if the request was sent, false otherwise

bool QXmppMucRoom::sendMessage(const QString &text)
{
    QXmppMessage msg;
    msg.setTo(d->jid);
    msg.setType(QXmppMessage::GroupChat);
    msg.setBody(text);
    return d->client->sendPacket(msg);
}

/// Sets your own nickname.

void QXmppMucRoom::setNickName(const QString &nickName)
{
    d->nickName = nickName;
}

/// Returns the presence for the given participant.
///
/// \param jid

QXmppPresence QXmppMucRoom::participantPresence(const QString &jid) const
{
    if (d->participants.contains(jid))
        return d->participants.value(jid);

    QXmppPresence presence;
    presence.setFrom(jid);
    presence.setType(QXmppPresence::Unavailable);
    return presence;
}

/// Returns the list of participant JIDs.

QStringList QXmppMucRoom::participants() const
{
    return d->participants.keys();
}

/// Returns the chat room password.

QString QXmppMucRoom::password() const
{
    return d->password;
}

/// Sets the chat room password.
///
/// \param password

void QXmppMucRoom::setPassword(const QString &password)
{
    d->password = password;
}

QXmppPresence::Status QXmppMucRoom::status() const
{
    return d->status;
}

void QXmppMucRoom::setStatus(const QXmppPresence::Status &status)
{
    d->status = status;

    if (isJoined()) {
        QXmppPresence packet;
        packet.setTo(d->ownJid());
        packet.setType(QXmppPresence::Available);
        packet.setStatus(status);
        d->client->sendPacket(packet);
    }
}

/// Returns the room's subject.

QString QXmppMucRoom::subject() const
{
    return d->subject;
}

/// Sets the chat room's subject.
///
/// \param subject

void QXmppMucRoom::setSubject(const QString &subject)
{
    QXmppMessage msg;
    msg.setTo(d->jid);
    msg.setType(QXmppMessage::GroupChat);
    msg.setSubject(subject);
    d->client->sendPacket(msg);
}

/// Request the configuration form for the chat room.
///
/// \return true if the request was sent, false otherwise
///
/// \sa configurationReceived()

bool QXmppMucRoom::requestConfiguration()
{
    QXmppMucOwnerIq iq;
    iq.setTo(d->jid);
    return d->client->sendPacket(iq);
}

/// Send the configuration form for the chat room.
///
/// \param form
///
/// \return true if the request was sent, false otherwise

bool QXmppMucRoom::setConfiguration(const QXmppDataForm &form)
{
    QXmppMucOwnerIq iqPacket;
    iqPacket.setType(QXmppIq::Set);
    iqPacket.setTo(d->jid);
    iqPacket.setForm(form);
    return d->client->sendPacket(iqPacket);
}

/// Request the room's permissions.
///
/// \return true if the request was sent, false otherwise
///
/// \sa permissionsReceived()

bool QXmppMucRoom::requestPermissions()
{
    QList<QXmppMucAdminIq::Item::Affiliation> affiliations;
    affiliations << QXmppMucAdminIq::Item::OwnerAffiliation;
    affiliations << QXmppMucAdminIq::Item::AdminAffiliation;
    affiliations << QXmppMucAdminIq::Item::MemberAffiliation;
    affiliations << QXmppMucAdminIq::Item::OutcastAffiliation;
    foreach (QXmppMucAdminIq::Item::Affiliation affiliation, affiliations) {
        QXmppMucAdminIq::Item item;
        item.setAffiliation(affiliation);

        QXmppMucAdminIq iq;
        iq.setTo(d->jid);
        iq.setItems(QList<QXmppMucAdminIq::Item>() << item);
        if (!d->client->sendPacket(iq))
            return false;
    }
    return true;
}

/// Sets the room's permissions.
///
/// \param permissions
///
/// \return true if the request was sent, false otherwise

bool QXmppMucRoom::setPermissions(const QList<QXmppMucAdminIq::Item> &permissions)
{
    QList<QXmppMucAdminIq::Item> items;

    // Process changed members
    foreach (const QXmppMucAdminIq::Item &item, permissions) {
        const QString jid = item.jid();
        if (d->affiliations.value(jid) != item.affiliation())
            items << item;
        d->affiliations.remove(jid);
    }

    // Process deleted members
    foreach (const QString &jid, d->affiliations.keys()) {
        QXmppMucAdminIq::Item item;
        item.setAffiliation(QXmppMucAdminIq::Item::NoAffiliation);
        item.setJid(jid);
        items << item;
        d->affiliations.remove(jid);
    }

    // Don't send request if there are no changes
    if (items.isEmpty())
        return false;

    QXmppMucAdminIq iq;
    iq.setTo(d->jid);
    iq.setType(QXmppIq::Set);
    iq.setItems(items);
    return d->client->sendPacket(iq);
}

void QXmppMucRoom::_q_disconnected()
{
    const bool wasJoined = isJoined();

    // clear chat room participants
    const QStringList removed = d->participants.keys();
    d->participants.clear();
    foreach (const QString &jid, removed)
        emit participantRemoved(jid);

    // emit "left" signal if we had joined the room
    if (wasJoined)
        emit left();
}

void QXmppMucRoom::_q_messageReceived(const QXmppMessage &message)
{
    if (jidToBareJid(message.from())!= d->jid ||
        message.type() != QXmppMessage::GroupChat)
        return;

    // handle message subject
    const QString subject = message.subject();
    if (!subject.isEmpty()) {
        d->subject = subject;
        emit subjectChanged(subject);
    }

    emit messageReceived(message);
}

void QXmppMucRoom::_q_presenceReceived(const QXmppPresence &presence)
{
    const QString jid = presence.from();

    // if our own presence changes, reflect it in the chat room
    if (jid == d->client->configuration().jid())
        setStatus(presence.status());

    if (jidToBareJid(jid) != d->jid)
        return;

    if (presence.type() == QXmppPresence::Available) {
        const bool added = !d->participants.contains(jid);
        d->participants.insert(jid, presence);
        if (added) {
            emit participantAdded(jid);
            if (jid == d->ownJid()) {

                // check whether we own the room
                foreach (const QXmppElement &x, presence.extensions()) {
                    if (x.tagName() == "x" && x.attribute("xmlns") == ns_muc_user)
                    {
                        QXmppElement item = x.firstChildElement("item");
                        if (item.attribute("jid") == d->client->configuration().jid()) {
                            d->allowedActions = NoAction;

                            // role
                            if (item.attribute("role") == "moderator")
                                d->allowedActions |= (KickAction | SubjectAction);

                            // affiliation
                            if (item.attribute("affiliation") == "owner")
                                d->allowedActions |= (ConfigurationAction | PermissionsAction | SubjectAction);
                            else if (item.attribute("affiliation") == "admin")
                                d->allowedActions |= (PermissionsAction | SubjectAction);
                        }
                    }
                }

                emit joined();
            }
        } else {
            emit participantChanged(jid);
        }
    }
    else if (presence.type() == QXmppPresence::Unavailable) {
        if (d->participants.remove(jid)) {
            emit participantRemoved(jid);

            // check whether this was our own presence
            if (jid == d->ownJid()) {

                // check whether we were kicked
                foreach (const QXmppElement &extension, presence.extensions()) {
                    if (extension.tagName() == "x" &&
                        extension.attribute("xmlns") == ns_muc_user) {
                        QXmppElement status = extension.firstChildElement("status");
                        while (!status.isNull()) {
                            if (status.attribute("code").toInt() == 307) {
                                // emit kick
                                const QString actor = extension.firstChildElement("item").firstChildElement("actor").attribute("jid");
                                const QString reason = extension.firstChildElement("item").firstChildElement("reason").value();
                                emit kicked(actor, reason);
                                break;
                            }
                            status = status.nextSiblingElement("status");
                        }
                    }
                }

                // notify user we left the room
                emit left();
            }
        }
    }
    else if (presence.type() == QXmppPresence::Error) {
        foreach (const QXmppElement &extension, presence.extensions()) {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc) {
                // emit error
                emit error(presence.error());

                // notify the user we left the room
                emit left();
                break;
            }
        }
   }
}
