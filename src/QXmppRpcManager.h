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

#ifndef QXMPPRPCMANAGER_H
#define QXMPPRPCMANAGER_H

#include <QMap>
#include <QVariant>

#include "QXmppClientExtension.h"

class QXmppInvokable;
class QXmppRemoteMethodResult;
class QXmppRpcInvokeIq;

class QXmppRpcManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    QXmppRpcManager();

    void addInvokableInterface( QXmppInvokable *interface );
    QXmppRemoteMethodResult callRemoteMethod( const QString &jid,
                                              const QString &interface,
                                              const QVariant &arg1 = QVariant(),
                                              const QVariant &arg2 = QVariant(),
                                              const QVariant &arg3 = QVariant(),
                                              const QVariant &arg4 = QVariant(),
                                              const QVariant &arg5 = QVariant(),
                                              const QVariant &arg6 = QVariant(),
                                              const QVariant &arg7 = QVariant(),
                                              const QVariant &arg8 = QVariant(),
                                              const QVariant &arg9 = QVariant(),
                                              const QVariant &arg10 = QVariant() );

    /// \cond
    QStringList discoveryFeatures() const;
    bool handleStanza(const QDomElement &element);
    /// \endcond

private slots:
    void invokeInterfaceMethod(const QXmppRpcInvokeIq &iq);

private:
    QMap<QString,QXmppInvokable*> m_interfaces;
};

#endif
