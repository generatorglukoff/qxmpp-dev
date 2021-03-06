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

#ifndef QXMPPBOOKMARKMANAGER_H
#define QXMPPBOOKMARKMANAGER_H

#include "QXmppClientExtension.h"

class QXmppBookmarkManagerPrivate;
class QXmppBookmarkSet;

/// \brief The QXmppBookmarkManager class allows you to store and retrieve
/// bookmarks as defined by XEP-0048: Bookmarks.
///

class QXmppBookmarkManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    QXmppBookmarkManager(QXmppClient *client);
    ~QXmppBookmarkManager();

    bool areBookmarksReceived() const;
    QXmppBookmarkSet bookmarks() const;
    bool setBookmarks(const QXmppBookmarkSet &bookmarks);

    /// \cond
    bool handleStanza(const QDomElement &stanza);
    /// \endcond

signals:
    /// This signal is emitted when bookmarks are received.
    void bookmarksReceived(const QXmppBookmarkSet &bookmarks);

private slots:
    void slotConnected();
    void slotDisconnected();

private:
    QXmppBookmarkManagerPrivate * const d;
};

#endif
