/*
 * Copyright (C) 2008-2011 The QXmpp developers
 *
 * Author:
 *  Alex Ermolov
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

#include "QXmppConstants.h"
#include "QXmppActivityIq.h"

static const char* activity_general[] = {
    "doing_chores",
    "drinking",
    "eating",
    "exercising",
    "grooming",
    "having_appointment",
    "inactive",
    "relaxing",
    "talking",
    "traveling",
    "working"
};

static const char* activity_specific[] = {
    "buying_groceries",
    "cleaning",
    "cooking",
    "doing_maintenance",
    "doing_the_dishes",
    "doing_the_laundry",
    "gardening",
    "running_an_errand",
    "walking_the_dog",
    "having_a_beer",
    "having_coffee",
    "having_tea",
    "having_a_snack",
    "having_breakfast",
    "having_dinner",
    "having_lunch",
    "dancing",
    "hiking",
    "jogging",
    "playing_sports",
    "running",
    "skiing",
    "swimming",
    "working_out",
    "at_the_spa",
    "brushing_teeth",
    "getting_a_haircut",
    "shaving",
    "taking_a_bath",
    "taking_a_shower",
    "day_off",
    "hanging_out",
    "hiding",
    "on_vacation",
    "praying",
    "scheduled_holiday",
    "sleeping",
    "thinking",
    "fishing",
    "gaming",
    "going_out",
    "partying",
    "reading",
    "rehearsing",
    "shopping",
    "smoking",
    "socializing",
    "sunbathing",
    "watching_tv",
    "watching_a_movie",
    "in_real_life",
    "on_the_phone",
    "on_video_phone",
    "commuting",
    "cycling",
    "driving",
    "in_a_car",
    "on_a_bus",
    "on_a_plane",
    "on_a_train",
    "on_a_trip",
    "walking",
    "coding",
    "in_a_meeting",
    "studying",
    "writing",
    "other"
};

QXmppActivityIq::QXmppActivityIq() :
    m_actgen(QXmppActivityIq::GeneralEmpty),
    m_actspec(QXmppActivityIq::SpecificEmpty),
    m_specadd(QString()),
    m_specaddns(QString()),
    m_detailed(QString()),
    m_detailedns(QString()),
    m_text(QString()),
    m_lang(QString()),
    m_bstop(false),
    m_bspecadd(false),
    m_bdetailed(false)
{
}

QXmppActivityIq::General QXmppActivityIq::activityGeneral() const
{
    return m_actgen;
}

void QXmppActivityIq::setActivityGeneral(QXmppActivityIq::General general)
{
    m_actgen = general;
}

QXmppActivityIq::Specific QXmppActivityIq::activitySpecific() const
{
    return m_actspec;
}

void QXmppActivityIq::setActivitySpecific(QXmppActivityIq::Specific specific)
{
    m_actspec = specific;
}

QString QXmppActivityIq::additionalSpecific() const
{
    return m_specadd;
}

void QXmppActivityIq::setAdditionalSpecific(const QString& addspec)
{
    m_specadd = addspec;
}

QString QXmppActivityIq::additionalSpecificNS() const
{
    return m_specaddns;
}

void QXmppActivityIq::setAdditionalSpecificNS(const QString& addspecns)
{
    m_specaddns = addspecns;
}

QString QXmppActivityIq::activityDetailed() const
{
    return m_detailed;
}

void QXmppActivityIq::setActivityDetailed(const QString& detailed)
{
    m_detailed = detailed;
}

QString QXmppActivityIq::activityDetailedNS() const
{
    return m_detailedns;
}

void QXmppActivityIq::setActivityDetailedNS(const QString& detailedns)
{
    m_detailedns = detailedns;
}

QString QXmppActivityIq::text() const
{
    return m_text;
}

void QXmppActivityIq::setText(const QString& text)
{
    m_text = text;
}

QString QXmppActivityIq::lang() const
{
    return m_lang;
}

void QXmppActivityIq::setLang(const QString& lang)
{
    m_lang = lang;
}

bool QXmppActivityIq::isStop() const
{
    return m_bstop;
}

bool QXmppActivityIq::isAdditionalSpecific() const
{
    return m_bspecadd;
}

bool QXmppActivityIq::isDetailed() const
{
    return m_bdetailed;
}

bool QXmppActivityIq::isActivityIq(const QDomElement &element)
{
    return (element.namespaceURI() == ns_activity);
}

void QXmppActivityIq::parse(const QDomElement &activity)
{
    QXmppStanza::Error error(QXmppStanza::Error::Cancel, QXmppStanza::Error::BadRequest);
    if (activity.namespaceURI() == ns_activity)
    {
        QDomElement general = QDomElement();
        QDomElement specific = QDomElement();
        QDomElement additional_specific = QDomElement();
        QDomElement detailed = QDomElement();
        m_bstop = !activity.hasChildNodes();
        if (!m_bstop)
        {
            QDomElement text = activity.firstChildElement("text");
            if (!text.isNull())
            {
                m_text = text.text();
                m_lang = text.attribute("lang");
            }
            for (int i = DoingChores; i <= Working; i++)
            {
                if (!activity.firstChildElement(activity_general[i]).isNull())
                {
                    general = activity.firstChildElement(activity_general[i]);
                    m_actgen = static_cast<General>(i);
                    break;
                }
            }
            if(!general.isNull())
            {
                for (int i = BuyingGroceries; i <= Other; i++)
                {
                    if (!general.firstChildElement(activity_specific[i]).isNull())
                    {
                        specific = general.firstChildElement(activity_specific[i]);
                        m_actspec = static_cast<Specific>(i);
                        break;
                    }
                }
            }
            else
                setError(error);
            if(!specific.isNull())
            {
                if (specific.hasChildNodes())
                {
                    m_bdetailed = true;
                    detailed = specific.firstChildElement();
                    m_detailed = detailed.tagName();
                    m_detailedns = detailed.namespaceURI();
                    if (m_detailedns == QString())
                        setError(error);
                }
            }
            else if (general.hasChildNodes())
            {
                m_bspecadd = true;
                additional_specific = general.firstChildElement();
                m_specadd = additional_specific.tagName();
                m_specaddns = additional_specific.namespaceURI();
                if (m_specaddns == QString())
                    setError(error);
            }
            else
                setError(error);
        }
    }
    else
        setError(error);
}

void QXmppActivityIq::toXml(QXmlStreamWriter *writer) const
{
    if (m_actgen < GeneralEmpty || m_actgen > Working)
        return;
    if (m_actspec < SpecificEmpty || m_actspec > Other)
        return;
    if (!m_bstop)
    {
        writer->writeStartElement("activity");
        writer->writeAttribute("xmlns", ns_activity);
        writer->writeStartElement(activity_general[m_actgen]);
        if (m_bdetailed)
        {
            writer->writeStartElement(activity_specific[m_actspec]);
            writer->writeEmptyElement(m_detailed);
            writer->writeAttribute("xmlns", m_detailedns);
            writer->writeEndElement();
        }
        else if (m_bspecadd)
        {
            writer->writeEmptyElement(m_specadd);
            writer->writeAttribute("xmlns", m_specaddns);
        }
        else
            writer->writeEmptyElement(activity_specific[m_actspec]);
        writer->writeEndElement();
        if (!m_text.isEmpty())
        {
            writer->writeStartElement("text");
            writer->writeAttribute("xml:lang", m_lang);
            writer->writeCharacters(m_text);
            writer->writeEndElement();
        }
        writer->writeEndElement();
    }
    else
    {
        writer->writeEmptyElement("activity");
        writer->writeAttribute("xmlns", ns_activity);
        writer->writeEndElement();
    }
}
