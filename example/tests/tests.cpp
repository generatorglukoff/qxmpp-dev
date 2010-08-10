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

#include <cstdlib>

#include <QCoreApplication>
#include <QDomDocument>
#include <QVariant>
#include <QtTest/QtTest>

#include "QXmppBind.h"
#include "QXmppJingleIq.h"
#include "QXmppMessage.h"
#include "QXmppPresence.h"
#include "QXmppRpcIq.h"
#include "QXmppSession.h"
#include "QXmppUtils.h"
#include "tests.h"

void TestUtils::testHmac()
{
    QByteArray hmac = generateHmacMd5(QByteArray(16, 0x0b), QByteArray("Hi There"));
    QCOMPARE(hmac, QByteArray::fromHex("9294727a3638bb1c13f48ef8158bfc9d"));

    hmac = generateHmacMd5(QByteArray("Jefe"), QByteArray("what do ya want for nothing?"));
    QCOMPARE(hmac, QByteArray::fromHex("750c783e6ab0b503eaa86e310a5db738"));

    hmac = generateHmacMd5(QByteArray(16, 0xaa), QByteArray(50, 0xdd));
    QCOMPARE(hmac, QByteArray::fromHex("56be34521d144c88dbb8c733f0e8b3f6"));
}

template <class T>
static void parsePacket(T &packet, const QByteArray &xml)
{
    //qDebug() << "parsing" << xml;
    QDomDocument doc;
    QCOMPARE(doc.setContent(xml, true), true);
    QDomElement element = doc.documentElement();
    packet.parse(element);
}

template <class T>
static void serializePacket(T &packet, const QByteArray &xml)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QXmlStreamWriter writer(&buffer);
    packet.toXml(&writer);
    qDebug() << "expect " << xml;
    qDebug() << "writing" << buffer.data();
    QCOMPARE(buffer.data(), xml);
}

void TestPackets::testBindNoResource()
{
    const QByteArray xml(
        "<iq id=\"bind_1\" type=\"set\">"
        "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\"/>"
        "</iq>");

    QXmppBind bind;
    parsePacket(bind, xml);
    QCOMPARE(bind.type(), QXmppIq::Set);
    QCOMPARE(bind.id(), QString("bind_1"));
    QCOMPARE(bind.jid(), QString());
    QCOMPARE(bind.resource(), QString());
    serializePacket(bind, xml);
}

void TestPackets::testBindResource()
{
    const QByteArray xml(
        "<iq id=\"bind_2\" type=\"set\">"
        "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\">"
        "<resource>someresource</resource>"
        "</bind>"
        "</iq>");

    QXmppBind bind;
    parsePacket(bind, xml);
    QCOMPARE(bind.type(), QXmppIq::Set);
    QCOMPARE(bind.id(), QString("bind_2"));
    QCOMPARE(bind.jid(), QString());
    QCOMPARE(bind.resource(), QString("someresource"));
    serializePacket(bind, xml);
}

void TestPackets::testBindResult()
{
    const QByteArray xml(
        "<iq id=\"bind_2\" type=\"result\">"
        "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\">"
        "<jid>somenode@example.com/someresource</jid>"
        "</bind>"
        "</iq>");

    QXmppBind bind;
    parsePacket(bind, xml);
    QCOMPARE(bind.type(), QXmppIq::Result);
    QCOMPARE(bind.id(), QString("bind_2"));
    QCOMPARE(bind.jid(), QString("somenode@example.com/someresource"));
    QCOMPARE(bind.resource(), QString());
    serializePacket(bind, xml);
}

void TestPackets::testMessage()
{
    const QByteArray xml(
        "<message to=\"foo@example.com/QXmpp\" from=\"bar@example.com/QXmpp\" type=\"normal\"/>");

    QXmppMessage message;
    parsePacket(message, xml);
    QCOMPARE(message.to(), QString("foo@example.com/QXmpp"));
    QCOMPARE(message.from(), QString("bar@example.com/QXmpp"));
    serializePacket(message, xml);
}

void TestPackets::testMessageFull()
{
    const QByteArray xml(
        "<message to=\"foo@example.com/QXmpp\" from=\"bar@example.com/QXmpp\" type=\"normal\">"
        "<subject>test subject</subject>"
        "<body>test body</body>"
        "<thread>test thread</thread>"
        "<composing xmlns=\"http://jabber.org/protocol/chatstates\"/>"
        "</message>");

    QXmppMessage message;
    parsePacket(message, xml);
    QCOMPARE(message.to(), QString("foo@example.com/QXmpp"));
    QCOMPARE(message.from(), QString("bar@example.com/QXmpp"));
    QCOMPARE(message.type(), QXmppMessage::Normal);
    QCOMPARE(message.body(), QString("test body"));
    QCOMPARE(message.subject(), QString("test subject"));
    QCOMPARE(message.thread(), QString("test thread"));
    QCOMPARE(message.state(), QXmppMessage::Composing);
    serializePacket(message, xml);
}

void TestPackets::testMessageDelay()
{
    const QByteArray xml(
        "<message to=\"foo@example.com/QXmpp\" from=\"bar@example.com/QXmpp\" type=\"normal\">"
        "<delay xmlns=\"urn:xmpp:delay\" stamp=\"2010-06-29T08:23:06Z\"/>"
        "</message>");

    QXmppMessage message;
    parsePacket(message, xml);
    QCOMPARE(message.stamp(), QDateTime(QDate(2010, 06, 29), QTime(8, 23, 6), Qt::UTC));
    serializePacket(message, xml);
}

void TestPackets::testMessageLegacyDelay()
{
    const QByteArray xml(
        "<message to=\"foo@example.com/QXmpp\" from=\"bar@example.com/QXmpp\" type=\"normal\">"
        "<x xmlns=\"jabber:x:delay\" stamp=\"20100629T08:23:06\"/>"
        "</message>");

    QXmppMessage message;
    parsePacket(message, xml);
    QCOMPARE(message.stamp(), QDateTime(QDate(2010, 06, 29), QTime(8, 23, 6), Qt::UTC));
    serializePacket(message, xml);
}

void TestPackets::testPresence()
{
    const QByteArray xml(
        "<presence to=\"foo@example.com/QXmpp\" from=\"bar@example.com/QXmpp\"/>");

    QXmppPresence presence;
    parsePacket(presence, xml);
    QCOMPARE(presence.to(), QString("foo@example.com/QXmpp"));
    QCOMPARE(presence.from(), QString("bar@example.com/QXmpp"));
    serializePacket(presence, xml);
}

void TestPackets::testPresenceFull()
{
    const QByteArray xml(
        "<presence to=\"foo@example.com/QXmpp\" from=\"bar@example.com/QXmpp\">"
        "<show>away</show>"
        "<status>In a meeting</status>"
        "<priority>5</priority>"
        "</presence>");

    QXmppPresence presence;
    parsePacket(presence, xml);
    QCOMPARE(presence.to(), QString("foo@example.com/QXmpp"));
    QCOMPARE(presence.from(), QString("bar@example.com/QXmpp"));
    QCOMPARE(presence.status().type(), QXmppPresence::Status::Away);
    QCOMPARE(presence.status().statusText(), QString("In a meeting"));
    QCOMPARE(presence.status().priority(), 5);
    serializePacket(presence, xml);
}

void TestPackets::testSession()
{
    const QByteArray xml(
        "<iq id=\"session_1\" to=\"example.com\" type=\"set\">"
        "<session xmlns=\"urn:ietf:params:xml:ns:xmpp-session\"/>"
        "</iq>");

    QXmppSession session;
    parsePacket(session, xml);
    QCOMPARE(session.id(), QString("session_1"));
    QCOMPARE(session.to(), QString("example.com"));
    QCOMPARE(session.type(), QXmppIq::Set);
    serializePacket(session, xml);
}

void TestJingle::testSession()
{
    const QByteArray xml(
        "<iq"
        " id=\"zid615d9\""
        " to=\"juliet@capulet.lit/balcony\""
        " from=\"romeo@montague.lit/orchard\""
        " type=\"set\">"
        "<jingle xmlns=\"urn:xmpp:jingle:1\""
        " action=\"session-initiate\""
        " initiator=\"romeo@montague.lit/orchard\""
        " sid=\"a73sjjvkla37jfea\">"
        "<content creator=\"initiator\" name=\"this-is-a-stub\">"
        "<description xmlns=\"urn:xmpp:jingle:apps:stub:0\"/>"
        "<transport xmlns=\"urn:xmpp:jingle:transports:stub:0\"/>"
        "</content>"
        "</jingle>"
        "</iq>");

    QXmppJingleIq session;
    parsePacket(session, xml);
    QCOMPARE(session.action(), QXmppJingleIq::SessionInitiate);
    QCOMPARE(session.initiator(), QLatin1String("romeo@montague.lit/orchard"));
    QCOMPARE(session.sid(), QLatin1String("a73sjjvkla37jfea"));
    QCOMPARE(session.content().creator(), QLatin1String("initiator"));
    QCOMPARE(session.content().name(), QLatin1String("this-is-a-stub"));
    QCOMPARE(session.reason().text(), QString());
    QCOMPARE(session.reason().type(), QXmppJingleIq::Reason::None);
    serializePacket(session, xml);
}

void TestJingle::testTerminate()
{
    const QByteArray xml(
        "<iq"
        " id=\"le71fa63\""
        " to=\"romeo@montague.lit/orchard\""
        " from=\"juliet@capulet.lit/balcony\""
        " type=\"set\">"
        "<jingle xmlns=\"urn:xmpp:jingle:1\""
        " action=\"session-terminate\""
        " sid=\"a73sjjvkla37jfea\">"
        "<reason>"
        "<success/>"
        "</reason>"
        "</jingle>"
        "</iq>");

    QXmppJingleIq session;
    parsePacket(session, xml);
    QCOMPARE(session.action(), QXmppJingleIq::SessionTerminate);
    QCOMPARE(session.initiator(), QString());
    QCOMPARE(session.sid(), QLatin1String("a73sjjvkla37jfea"));
    QCOMPARE(session.reason().text(), QString());
    QCOMPARE(session.reason().type(), QXmppJingleIq::Reason::Success);
    serializePacket(session, xml);
}

void TestJingle::testPayloadType()
{
    const QByteArray xml("<payload-type id=\"103\" name=\"L16\" channels=\"2\" clockrate=\"16000\"/>");
    QXmppJinglePayloadType payload;
    parsePacket(payload, xml);
    QCOMPARE(payload.id(), static_cast<unsigned char>(103));
    QCOMPARE(payload.name(), QLatin1String("L16"));
    QCOMPARE(payload.channels(), static_cast<unsigned char>(2));
    QCOMPARE(payload.clockrate(), 16000u);
    serializePacket(payload, xml);
}

void TestJingle::testRinging()
{
    const QByteArray xml(
        "<iq"
        " id=\"tgr515bt\""
        " to=\"romeo@montague.lit/orchard\""
        " from=\"juliet@capulet.lit/balcony\""
        " type=\"set\">"
        "<jingle xmlns=\"urn:xmpp:jingle:1\""
        " action=\"session-info\""
        " initiator=\"romeo@montague.lit/orchard\""
        " sid=\"a73sjjvkla37jfea\">"
        "<ringing xmlns=\"urn:xmpp:jingle:apps:rtp:info:1\"/>"
        "</jingle>"
        "</iq>");

    QXmppJingleIq iq;
    parsePacket(iq, xml);
    QCOMPARE(iq.ringing(), true);
    serializePacket(iq, xml);
}

static void checkVariant(const QVariant &value, const QByteArray &xml)
{
    // serialise
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QXmlStreamWriter writer(&buffer);
    XMLRPC::marshall(&writer, value);
    qDebug() << "expect " << xml;
    qDebug() << "writing" << buffer.data();
    QCOMPARE(buffer.data(), xml);

    // parse
    QDomDocument doc;
    QCOMPARE(doc.setContent(xml, true), true);
    QDomElement element = doc.documentElement();
    QStringList errors;
    QVariant test = XMLRPC::demarshall(element, errors);
    if (!errors.isEmpty())
        qDebug() << errors;
    QCOMPARE(errors, QStringList());
    QCOMPARE(test, value);
}

void TestXmlRpc::testBase64()
{
    checkVariant(QByteArray("\0\1\2\3", 4),
                 QByteArray("<value><base64>AAECAw==</base64></value>"));
}

void TestXmlRpc::testBool()
{
    checkVariant(false,
                 QByteArray("<value><boolean>0</boolean></value>"));
    checkVariant(true,
                 QByteArray("<value><boolean>1</boolean></value>"));
}

void TestXmlRpc::testDateTime()
{
    checkVariant(QDateTime(QDate(1998, 7, 17), QTime(14, 8, 55)),
                 QByteArray("<value><dateTime.iso8601>1998-07-17T14:08:55</dateTime.iso8601></value>"));
}

void TestXmlRpc::testDouble()
{
    checkVariant(double(-12.214),
                 QByteArray("<value><double>-12.214</double></value>"));
}

void TestXmlRpc::testInt()
{
    checkVariant(int(-12),
                 QByteArray("<value><i4>-12</i4></value>"));
}

void TestXmlRpc::testNil()
{
    checkVariant(QVariant(),
                 QByteArray("<value><nil/></value>"));
}

void TestXmlRpc::testString()
{
    checkVariant(QString("hello world"),
                 QByteArray("<value><string>hello world</string></value>"));
}

void TestXmlRpc::testArray()
{
    checkVariant(QVariantList() << QString("hello world") << double(-12.214),
                 QByteArray("<value><array><data>"
                            "<value><string>hello world</string></value>"
                            "<value><double>-12.214</double></value>"
                            "</data></array></value>"));
}

void TestXmlRpc::testStruct()
{
    QMap<QString, QVariant> map;
    map["bar"] = QString("hello world");
    map["foo"] = double(-12.214);
    checkVariant(map,
                 QByteArray("<value><struct>"
                            "<member>"
                                "<name>bar</name>"
                                "<value><string>hello world</string></value>"
                            "</member>"
                            "<member>"
                                "<name>foo</name>"
                                "<value><double>-12.214</double></value>"
                            "</member>"
                            "</struct></value>"));
}

void TestXmlRpc::testInvoke()
{
    const QByteArray xml(
        "<iq"
        " id=\"rpc1\""
        " to=\"responder@company-a.com/jrpc-server\""
        " from=\"requester@company-b.com/jrpc-client\""
        " type=\"set\">"
        "<query xmlns=\"jabber:iq:rpc\">"
        "<methodCall>"
        "<methodName>examples.getStateName</methodName>"
        "<params>"
        "<param>"
        "<value><i4>6</i4></value>"
        "</param>"
        "</params>"
        "</methodCall>"
        "</query>"
        "</iq>");

    QXmppRpcInvokeIq iq;
    parsePacket(iq, xml);
    QCOMPARE(iq.method(), QLatin1String("examples.getStateName"));
    QCOMPARE(iq.arguments(), QVariantList() << int(6));
    serializePacket(iq, xml);
}

void TestXmlRpc::testResponse()
{
    const QByteArray xml(
        "<iq"
        " id=\"rpc1\""
        " to=\"requester@company-b.com/jrpc-client\""
        " from=\"responder@company-a.com/jrpc-server\""
        " type=\"result\">"
        "<query xmlns=\"jabber:iq:rpc\">"
        "<methodResponse>"
        "<params>"
        "<param>"
        "<value><string>Colorado</string></value>"
        "</param>"
        "</params>"
        "</methodResponse>"
        "</query>"
        "</iq>");

    QXmppRpcResponseIq iq;
    parsePacket(iq, xml);
    QCOMPARE(iq.faultCode(), 0);
    QCOMPARE(iq.faultString(), QString());
    QCOMPARE(iq.values(), QVariantList() << QString("Colorado"));
    serializePacket(iq, xml);
}

void TestXmlRpc::testResponseFault()
{
    const QByteArray xml(
        "<iq"
        " id=\"rpc1\""
        " to=\"requester@company-b.com/jrpc-client\""
        " from=\"responder@company-a.com/jrpc-server\""
        " type=\"result\">"
        "<query xmlns=\"jabber:iq:rpc\">"
        "<methodResponse>"
        "<fault>"
        "<value>"
            "<struct>"
                "<member>"
                    "<name>faultCode</name>"
                    "<value><i4>404</i4></value>"
                "</member>"
                "<member>"
                    "<name>faultString</name>"
                    "<value><string>Not found</string></value>"
                "</member>"
            "</struct>"
        "</value>"
        "</fault>"
        "</methodResponse>"
        "</query>"
        "</iq>");

    QXmppRpcResponseIq iq;
    parsePacket(iq, xml);
    QCOMPARE(iq.faultCode(), 404);
    QCOMPARE(iq.faultString(), QLatin1String("Not found"));
    QCOMPARE(iq.values(), QVariantList());
    serializePacket(iq, xml);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // run tests
    int errors = 0;

    TestUtils testUtils;
    errors += QTest::qExec(&testUtils);

    TestPackets testPackets;
    errors += QTest::qExec(&testPackets);

    TestJingle testJingle;
    errors += QTest::qExec(&testJingle);

    TestXmlRpc testXmlRpc;
    errors += QTest::qExec(&testXmlRpc);

    if (errors)
    {
        qWarning() << "Total failed tests:" << errors;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
};

