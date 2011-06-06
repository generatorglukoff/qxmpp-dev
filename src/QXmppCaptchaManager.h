/*
 * Copyright (C) 2008-2011 The QXmpp developers
 *
 * Author:
 *  Andrey Batyiev
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

#ifndef QXMPPCAPTCHAMANAGER_H
#define QXMPPCAPTCHAMANAGER_H

#include "QXmppClientExtension.h"

class QDomElement;
class QXmppDataForm;

class QXmppCaptchaManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    virtual bool handleStanza(const QDomElement &stanza);

signals:
    void captchaFormReceived(const QXmppDataForm &form);
};

#endif // QXMPPCAPTCHAMANAGER_H
