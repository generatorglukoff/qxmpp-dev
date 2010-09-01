/*
 * Copyright (C) 2008-2010 The QXmpp developers
 *
 * Author:
 *  Manjeet Dahiya
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


#include <QBuffer>
#include <QXmlStreamWriter>

#ifndef QXMPP_NO_GUI
#include <QImage>
#include <QImageReader>
#endif

#include "QXmppVCard.h"
#include "QXmppUtils.h"
#include "QXmppConstants.h"

QString getImageType(const QByteArray &contents)
{
    if (contents.startsWith("\x89PNG\x0d\x0a\x1a\x0a"))
        return "image/png";
    else if (contents.startsWith("\x8aMNG"))
        return "video/x-mng";
    else if (contents.startsWith("GIF8"))
        return "image/gif";
    else if (contents.startsWith("BM"))
        return "image/bmp";
    else if (contents.contains("/* XPM */"))
        return "image/x-xpm";
    else if (contents.contains("<?xml") && contents.contains("<svg"))
        return "image/svg+xml"; 
    else if (contents.startsWith("\xFF\xD8\xFF\xE0"))
        return "image/jpeg";
    return "image/unknown";
}

QXmppVCard::QXmppVCard(const QString& jid) : QXmppIq(QXmppIq::Get)
{
    // for self jid should be empty
    setTo(jid);
}

/// Returns the date of birth of the individual associated with the vCard.
///

QDate QXmppVCard::birthday() const
{
    return m_birthday;
}

/// Sets the date of birth of the individual associated with the vCard.
///
/// \param birthday

void QXmppVCard::setBirthday(const QDate &birthday)
{
    m_birthday = birthday;
}

/// Returns the email address.
///

QString QXmppVCard::email() const
{
    return m_email;
}

/// Sets the email address.
///
/// \param email

void QXmppVCard::setEmail(const QString &email)
{
    m_email = email;
}

/// Returns the first name.
///

QString QXmppVCard::firstName() const
{
    return m_firstName;
}

/// Sets the first name.
///
/// \param firstName

void QXmppVCard::setFirstName(const QString &firstName)
{
    m_firstName = firstName;
}

/// Returns the full name.
///

QString QXmppVCard::fullName() const
{
    return m_fullName;
}

/// Sets the full name.
///
/// \param fullName

void QXmppVCard::setFullName(const QString &fullName)
{
    m_fullName = fullName;
}

/// Returns the last name.
///

QString QXmppVCard::lastName() const
{
    return m_lastName;
}

/// Sets the last name.
///
/// \param lastName

void QXmppVCard::setLastName(const QString &lastName)
{
    m_lastName = lastName;
}

/// Returns the middle name.
///

QString QXmppVCard::middleName() const
{
    return m_middleName;
}

/// Sets the middle name.
///
/// \param middleName

void QXmppVCard::setMiddleName(const QString &middleName)
{
    m_middleName = middleName;
}

/// Returns the nickname.
///

QString QXmppVCard::nickName() const
{
    return m_nickName;
}

/// Sets the nickname.
///
/// \param nickName

void QXmppVCard::setNickName(const QString &nickName)
{
    m_nickName = nickName;
}

/// Returns the URL associated with the vCard. It can represent the user's
/// homepage or a location at which you can find real-time information about
/// the vCard.

QString QXmppVCard::url() const
{
    return m_url;
}

/// Sets the URL associated with the vCard. It can represent the user's
/// homepage or a location at which you can find real-time information about
/// the vCard.
///
/// \param url

void QXmppVCard::setUrl(const QString& url)
{
    m_url = url;
}

/// Returns the photo's binary contents.

QByteArray QXmppVCard::photo() const
{
    return m_photo;
}

/// Sets the photo's binary contents.

void QXmppVCard::setPhoto(const QByteArray& photo)
{
    m_photo = photo;
}

/// Returns the photo's MIME type.

QString QXmppVCard::photoType() const
{
    return m_photoType;
}

/// Sets the photo's MIME type.

void QXmppVCard::setPhotoType(const QString& photoType)
{
    m_photoType = photoType;
}

bool QXmppVCard::isVCard(const QDomElement &nodeRecv)
{
    return nodeRecv.firstChildElement("vCard").namespaceURI() == ns_vcard;
}

void QXmppVCard::parseElementFromChild(const QDomElement& nodeRecv)
{
    // vCard
    QDomElement cardElement = nodeRecv.firstChildElement("vCard");
    m_birthday = QDate::fromString(cardElement.firstChildElement("BDAY").text(), "yyyy-MM-dd");
    QDomElement emailElement = cardElement.firstChildElement("EMAIL");
    m_email = emailElement.firstChildElement("USERID").text();
    m_fullName = cardElement.firstChildElement("FN").text();
    m_nickName = cardElement.firstChildElement("NICKNAME").text();
    QDomElement nameElement = cardElement.firstChildElement("N");
    m_firstName = nameElement.firstChildElement("GIVEN").text();
    m_lastName = nameElement.firstChildElement("FAMILY").text();
    m_middleName = nameElement.firstChildElement("MIDDLE").text();
    m_url = cardElement.firstChildElement("URL").text();
    QDomElement photoElement = cardElement.firstChildElement("PHOTO");
    QByteArray base64data = photoElement.
                            firstChildElement("BINVAL").text().toAscii();
    m_photo = QByteArray::fromBase64(base64data);
    m_photoType = photoElement.firstChildElement("TYPE").text();
}

void QXmppVCard::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("vCard");
    helperToXmlAddAttribute(writer,"xmlns", ns_vcard);
    if (m_birthday.isValid())
        helperToXmlAddTextElement(writer, "BDAY", m_birthday.toString("yyyy-MM-dd"));
    if (!m_email.isEmpty())
    {
        writer->writeStartElement("EMAIL");
        writer->writeEmptyElement("INTERNET");
        helperToXmlAddTextElement(writer, "USERID", m_email);
        writer->writeEndElement();
    }
    if (!m_fullName.isEmpty())
        helperToXmlAddTextElement(writer, "FN", m_fullName);
    if(!m_nickName.isEmpty())
        helperToXmlAddTextElement(writer, "NICKNAME", m_nickName);
    if (!m_firstName.isEmpty() ||
        !m_lastName.isEmpty() ||
        !m_middleName.isEmpty())
    {
        writer->writeStartElement("N");
        if (!m_firstName.isEmpty())
            helperToXmlAddTextElement(writer, "GIVEN", m_firstName);
        if (!m_lastName.isEmpty())
            helperToXmlAddTextElement(writer, "FAMILY", m_lastName);
        if (!m_middleName.isEmpty())
            helperToXmlAddTextElement(writer, "MIDDLE", m_middleName);
        writer->writeEndElement();
    }
    if (!m_url.isEmpty())
        helperToXmlAddTextElement(writer, "URL", m_url);

    if(!photo().isEmpty())
    {
        writer->writeStartElement("PHOTO");
        QString photoType = m_photoType;
        if (photoType.isEmpty())
            photoType = getImageType(m_photo);
        helperToXmlAddTextElement(writer, "TYPE", photoType);
        helperToXmlAddTextElement(writer, "BINVAL", m_photo.toBase64());
        writer->writeEndElement();
    }

    writer->writeEndElement();
}

#ifndef QXMPP_NO_GUI
QImage QXmppVCard::photoAsImage() const
{
    QBuffer buffer;
    buffer.setData(m_photo);
    buffer.open(QIODevice::ReadOnly);
    QImageReader imageReader(&buffer);
    return imageReader.read();
}

void QXmppVCard::setPhoto(const QImage& image)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    m_photo = ba;
    m_photoType = "image/png";
}
#endif

QString QXmppVCard::getFullName() const
{
    return m_fullName;
}

QString QXmppVCard::getNickName() const
{
    return m_nickName;
}

const QByteArray& QXmppVCard::getPhoto() const
{
    return m_photo;
}

#ifndef QXMPP_NO_GUI
QImage QXmppVCard::getPhotoAsImage() const
{
    return photoAsImage();
}
#endif
