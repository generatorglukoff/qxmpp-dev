/*
 * Copyright (C) 2008-2011 The QXmpp developers
 *
 * Authors:
 *  Manjeet Dahiya
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
#include "QXmppPresence.h"
#include "QXmppRosterIq.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"

/// Constructs a roster manager.

QXmppRosterManager::QXmppRosterManager(QXmppClient* client)
    : m_isRosterReceived(false)
{
    bool check = QObject::connect(client, SIGNAL(connected()),
        this, SLOT(connected()));
    Q_ASSERT(check);

    check = QObject::connect(client, SIGNAL(disconnected()),
        this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = QObject::connect(client, SIGNAL(presenceReceived(const QXmppPresence&)),
        this, SLOT(presenceReceived(const QXmppPresence&)));
    Q_ASSERT(check);
}

/// Accepts a subscription request.
///
/// You can call this method in reply to the subscriptionRequest() signal.

bool QXmppRosterManager::acceptSubscription(const QString &bareJid, const QString &reason)
{
    QXmppPresence presence;
    presence.setTo(bareJid);
    presence.setType(QXmppPresence::Subscribed);
    return client()->sendPacket(presence);
}

/// Upon XMPP connection, request the roster.
///
void QXmppRosterManager::connected()
{
    QXmppRosterIq roster;
    roster.setType(QXmppIq::Get);
    roster.setFrom(client()->configuration().jid());
    m_rosterReqId = roster.id();
    client()->sendPacket(roster);
}

void QXmppRosterManager::disconnected()
{
    m_entries.clear();
    m_presences.clear();
    m_isRosterReceived = false;
}

bool QXmppRosterManager::handleStanza(const QDomElement &element)
{
    if(element.tagName() == "iq" && QXmppRosterIq::isRosterIq(element))
    {
        QXmppRosterIq rosterIq;
        rosterIq.parse(element);

        // Security check: only server should send this iq
        // from() should be either empty or bareJid of the user
        QString fromJid = rosterIq.from();
        if(fromJid.isEmpty() ||
           jidToBareJid(fromJid) == client()->configuration().jidBare())
        {
            rosterIqReceived(rosterIq);
            return true;
        }
    }

    return false;
}

void QXmppRosterManager::presenceReceived(const QXmppPresence& presence)
{
    const QString jid = presence.from();
    const QString bareJid = jidToBareJid(jid);
    const QString resource = jidToResource(jid);

    if (bareJid.isEmpty())
        return;

    switch(presence.type())
    {
    case QXmppPresence::Available:
        m_presences[bareJid][resource] = presence;
        emit presenceChanged(bareJid, resource);
        break;
    case QXmppPresence::Unavailable:
        m_presences[bareJid].remove(resource);
        emit presenceChanged(bareJid, resource);
        break;
    case QXmppPresence::Subscribe:
        if (client()->configuration().autoAcceptSubscriptions())
        {
            // accept subscription request
            acceptSubscription(bareJid);

            // ask for reciprocal subscription
            subscribe(bareJid);
        } else {
            emit subscriptionReceived(bareJid);
        }
        break;
    default:
        break;
    }
}

/// Refuses a subscription request.
///
/// You can call this method in reply to the subscriptionRequest() signal.

bool QXmppRosterManager::refuseSubscription(const QString &bareJid, const QString &reason)
{
    QXmppPresence presence;
    presence.setTo(bareJid);
    presence.setType(QXmppPresence::Unsubscribed);
    return client()->sendPacket(presence);
}

/// Removes a roster entry and cancels subscriptions to and from the contact.
///
/// As a result, the server will initiate a roster push, causing the
/// itemRemoved() signal to be emitted.
///
/// \param bareJid

bool QXmppRosterManager::removeItem(const QString &bareJid)
{
    QXmppRosterIq::Item item;
    item.setBareJid(bareJid);
    item.setSubscriptionType(QXmppRosterIq::Item::Remove);

    QXmppRosterIq iq;
    iq.setType(QXmppIq::Set);
    iq.addItem(item);
    return client()->sendPacket(iq);
}

/// Adds a new entry to the roster without sending any subscription requests.
///
/// The server will initiate a roster push, as with removeRosterEntry(), causing
/// the rosterChanged() signal to be emitted for the newly given roster entry.
///
/// \param bareJid
/// \param name Optional name of the entry to save it under.
/// \param message Optional message to send to the entry.
/// \param groups Groups that this entry belongs to.
///
/// \todo Use the message parameter.

void QXmppRosterManager::addRosterEntry(const QString &bareJid, const QString &name,
                                        const QString &message, const QSet<QString> &groups)
{
    QXmppRosterIq::Item item;
    item.setBareJid(bareJid);
    item.setName(name);
    item.setGroups(groups);
    item.setSubscriptionType(QXmppRosterIq::Item::NotSet);

    QXmppRosterIq iq;
    iq.setType(QXmppIq::Set);
    iq.addItem(item);
    client()->sendPacket(iq);
}

void QXmppRosterManager::rosterIqReceived(const QXmppRosterIq& rosterIq)
{
    bool isInitial = (m_rosterReqId == rosterIq.id());

    switch(rosterIq.type())
    {
    case QXmppIq::Set:
        {
            // send result iq
            QXmppIq returnIq(QXmppIq::Result);
            returnIq.setId(rosterIq.id());
            client()->sendPacket(returnIq);

            // store updated entries and notify changes
            const QList<QXmppRosterIq::Item> items = rosterIq.items();
            foreach (const QXmppRosterIq::Item &item, items) {
                const QString bareJid = item.bareJid();
                if (item.subscriptionType() == QXmppRosterIq::Item::Remove) {
                    if (m_entries.remove(bareJid)) {
                        // notify the user that the item was removed
                        emit itemRemoved(bareJid);
                    }
                } else {
                    const bool added = !m_entries.contains(bareJid);
                    m_entries.insert(bareJid, item);
                    if (added) {
                        // notify the user that the item was added
                        emit itemAdded(bareJid);
                    } else {
                        // notify the user that the item changed
                        emit itemChanged(bareJid);
                    }

                    // FIXME: remove legacy signal
                    emit rosterChanged(bareJid);
                }
            }
        }
        break;
    case QXmppIq::Result:
        {
            QList<QXmppRosterIq::Item> items = rosterIq.items();
            for(int i = 0; i < items.count(); ++i)
            {
                QString bareJid = items.at(i).bareJid();
                m_entries[bareJid] = items.at(i);
                if (!isInitial)
                    emit rosterChanged(bareJid);
            }
            if (isInitial)
            {
                m_isRosterReceived = true;
                emit rosterReceived();
            }
            break;
        }
    default:
        break;
    }
}

/// Requests a subscription to the given contact.
///
/// As a result, the server will initiate a roster push, causing the
/// itemAdded() or itemChanged() signal to be emitted.

bool QXmppRosterManager::subscribe(const QString &bareJid, const QString &reason)
{
    QXmppPresence packet;
    packet.setTo(jidToBareJid(bareJid));
    packet.setType(QXmppPresence::Subscribe);
    return client()->sendPacket(packet);
}

/// Removes a subscription to the given contact.
///
/// As a result, the server will initiate a roster push, causing the
/// itemChanged() signal to be emitted.

bool QXmppRosterManager::unsubscribe(const QString &bareJid, const QString &reason)
{
    QXmppPresence packet;
    packet.setTo(jidToBareJid(bareJid));
    packet.setType(QXmppPresence::Unsubscribe);
    return client()->sendPacket(packet);
}

/// Function to get all the bareJids present in the roster.
///
/// \return QStringList list of all the bareJids
///

QStringList QXmppRosterManager::getRosterBareJids() const
{
    return m_entries.keys();
}

/// Returns the roster entry of the given bareJid. If the bareJid is not in the
/// database and empty QXmppRosterIq::Item will be returned.
///
/// \param bareJid as a QString
///

QXmppRosterIq::Item QXmppRosterManager::getRosterEntry(
        const QString& bareJid) const
{
    // will return blank entry if bareJid does'nt exist
    if(m_entries.contains(bareJid))
        return m_entries.value(bareJid);
    else
        return QXmppRosterIq::Item();
}

/// Get all the associated resources with the given bareJid.
///
/// \param bareJid as a QString
/// \return list of associated resources as a QStringList
///

QStringList QXmppRosterManager::getResources(const QString& bareJid) const
{
    if(m_presences.contains(bareJid))
        return m_presences[bareJid].keys();
    else
        return QStringList();
}

/// Get all the presences of all the resources of the given bareJid. A bareJid
/// can have multiple resources and each resource will have a presence
/// associated with it.
///
/// \param bareJid as a QString
/// \return Map of resource and its respective presence QMap<QString, QXmppPresence>
///

QMap<QString, QXmppPresence> QXmppRosterManager::getAllPresencesForBareJid(
        const QString& bareJid) const
{
    if(m_presences.contains(bareJid))
        return m_presences[bareJid];
    else
        return QMap<QString, QXmppPresence>();
}

/// Get the presence of the given resource of the given bareJid.
///
/// \param bareJid as a QString
/// \param resource as a QString
/// \return QXmppPresence
///

QXmppPresence QXmppRosterManager::getPresence(const QString& bareJid,
                                       const QString& resource) const
{
    if(m_presences.contains(bareJid) && m_presences[bareJid].contains(resource))
        return m_presences[bareJid][resource];
    else
    {
        QXmppPresence presence;
        presence.setType(QXmppPresence::Unavailable);
        presence.setStatus(QXmppPresence::Status::Offline);
        return presence;
    }
}

/// Function to check whether the roster has been received or not.
///
/// \return true if roster received else false

bool QXmppRosterManager::isRosterReceived()
{
    return m_isRosterReceived;
}

// deprecated

void QXmppRosterManager::removeRosterEntry(const QString &bareJid)
{
    removeItem(bareJid);
}

