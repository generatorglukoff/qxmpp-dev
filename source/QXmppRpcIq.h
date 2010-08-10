/*
 * Copyright (C) 2009-2010 Ian Reinhard Geiser
 *
 * Authors:
 *	Ian Reinhard Geiser
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

#ifndef QXMPPRPCIQ_H
#define QXMPPRPCIQ_H

#include "QXmppIq.h"
#include <QVariant>

class QXmlStreamWriter;
class QDomElement;

class QXmppRpcResponseIq : public QXmppIq
{
public:
    QXmppRpcResponseIq();

    QVariantList values() const;
    void setValues(const QVariantList &values);

    static bool isRpcResponseIq(const QDomElement &element);

protected:
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QVariantList m_values;
};

class QXmppRpcInvokeIq : public QXmppIq
{
public:
    QXmppRpcInvokeIq();

    QString interface() const;
    void setInterface( const QString &interface );

    QString method() const;
    void setMethod( const QString &method );

    QVariantList arguments() const;
    void setArguments(const QVariantList &arguments);

    static bool isRpcInvokeIq(const QDomElement &element);

protected:
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QVariantList m_arguments;
    QString m_method;
    QString m_interface;

    friend class QXmppRpcErrorIq;
};

class QXmppRpcErrorIq : public QXmppIq
{
public:
    QXmppRpcErrorIq();

    QXmppRpcInvokeIq query() const;
    void setQuery(const QXmppRpcInvokeIq &query);

    static bool isRpcErrorIq(const QDomElement &element);

protected:
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QXmppRpcInvokeIq m_query;
};

#endif // QXMPPRPCIQ_H
