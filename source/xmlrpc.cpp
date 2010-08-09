#include "xmlrpc.h"
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <QStringList>
#include <QTextStream>

void XMLRPC::MessageBase::marshall( QXmlStreamWriter *writer, const QVariant &value ) const
{
    writer->writeStartElement("value");
    switch( value.type() )
    {
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
            writer->writeTextElement("i4", value.toString());
            break;
        case QVariant::Double:
            writer->writeTextElement("double", value.toString());
            break;
        case QVariant::Bool:
            writer->writeTextElement("boolean", (value.toBool()?"true":"false") );
            break;
        case QVariant::Date:
            writer->writeTextElement("dateTime.iso8601", value.toDate().toString( Qt::ISODate ) );
            break;
        case QVariant::DateTime:
            writer->writeTextElement("dateTime.iso8601", value.toDateTime().toString( Qt::ISODate ) );
            break;
        case QVariant::Time:
            writer->writeTextElement("dateTime.iso8601", value.toTime().toString( Qt::ISODate ) );
            break;
        case QVariant::StringList:
        case QVariant::List:
        {
            writer->writeStartElement("array");
            writer->writeStartElement("data");
            foreach( QVariant item, value.toList() )
                marshall( writer, item );
            writer->writeEndElement();
            writer->writeEndElement();
            break;
        }
        case QVariant::Map:
        {
            writer->writeStartElement("struct");
            QMap<QString, QVariant> map = value.toMap();
            QMap<QString, QVariant>::ConstIterator index = map.begin();
            while( index != map.end() )
            {
                writer->writeStartElement("member");
                writer->writeTextElement("name", index.key());
                marshall( writer, *index );
                writer->writeEndElement();
                ++index;
            }
            writer->writeEndElement();
            break;
        }
        case QVariant::ByteArray:
        {
            writer->writeTextElement("base64", value.toByteArray().toBase64() );
            break;
        }
        default:
        {
            if( value.canConvert(QVariant::String) )
            {
                writer->writeTextElement( "string", value.toString() );
            }
            break;
        }
    }
    writer->writeEndElement();
}

bool XMLRPC::MessageBase::isValid() const
{
    return m_valid;
}

QString XMLRPC::MessageBase::error() const
{
    return m_message;
}

QVariant XMLRPC::MessageBase::demarshall( const QDomElement &elem ) const
{
    if ( elem.tagName().toLower() != "value" )
    {
        m_valid = false;
        m_message = "bad param value";
        return QVariant();
    }

    if ( !elem.firstChild().isElement() )
    {
        return QVariant( elem.text() );
    }

    const QDomElement typeData = elem.firstChild().toElement();
    const QString typeName = typeData.tagName().toLower();

    if ( typeName == "string" )
                return QVariant( typeData.text() );
    else if (typeName == "int" || typeName == "i4" )
    {
        bool ok = false;
        QVariant val( typeData.text().toInt( &ok ) );
        if( ok )
            return val;
        m_message = "I was looking for an integer but data was courupt";
    }
    else if( typeName == "double" )
    {
        bool ok = false;
        QVariant val( typeData.text().toDouble( &ok ) );
        if( ok )
            return val;
        m_message = "I was looking for an double but data was courupt";
    }
    else if( typeName == "boolean" )
        return QVariant( ( typeData.text().toLower() == "true" || typeData.text() == "1")?true:false );
    else if( typeName == "datetime" || typeName == "dateTime.iso8601" )
        return QVariant( QDateTime::fromString( typeData.text(), Qt::ISODate ) );
    else if( typeName == "array" )
    {
        QList<QVariant> arr;
        QDomNode valueNode = typeData.firstChild().firstChild();
        while( !valueNode.isNull() && m_valid )
        {
            arr.append( demarshall( valueNode.toElement() ) );
            valueNode = valueNode.nextSibling();
        }
        return QVariant( arr );
    }
    else if( typeName == "struct" )
    {
        QMap<QString,QVariant> stct;
        QDomNode valueNode = typeData.firstChild();
        while( !valueNode.isNull() && m_valid )
        {
            const QDomElement memberNode = valueNode.toElement().elementsByTagName("name").item(0).toElement();
            const QDomElement dataNode = valueNode.toElement().elementsByTagName("value").item(0).toElement();
            stct[ memberNode.text() ] = demarshall( dataNode );
            valueNode = valueNode.nextSibling();
        }
        return QVariant(stct);
    }
    else if( typeName == "base64" )
    {
        QVariant returnVariant;
        QByteArray dest;
        QByteArray src = typeData.text().toLatin1();
        dest = QByteArray::fromBase64( src );
        QDataStream ds(&dest, QIODevice::ReadOnly);
        ds.setVersion(QDataStream::Qt_4_0);
        ds >> returnVariant;
        if( returnVariant.isValid() )
            return returnVariant;
        else
            return QVariant( dest );
    }
    setError(QString( "Cannot handle type %1").arg(typeName));
    return QVariant();
}

void XMLRPC::MessageBase::setError( const QString & message ) const
{
    m_valid = false;
    m_message = message;
}

XMLRPC::MessageBase::MessageBase( ) : m_valid(true)
{
}

XMLRPC::MessageBase::~MessageBase( )
{
}

XMLRPC::RequestMessage::RequestMessage( const QByteArray &method, const QList<QVariant> &args )
: MessageBase()
{
    m_method = method;
    m_args = args;
}

XMLRPC::RequestMessage::RequestMessage( const QDomElement  &element )
{
    m_args.clear();
    m_method.clear();

    const QDomElement methodName = element.firstChildElement("methodName");
    if( !methodName.isNull() )
    {
        m_method = methodName.text().toLatin1();
    }
    else
    {
        setError("Missing methodName property.");
        return;
    }

    const QDomElement methodParams = element.firstChildElement("params");
    if( !methodParams.isNull() )
    {
        QDomNode param = methodParams.firstChild();
        while( !param.isNull() && isValid() )
        {
            m_args.append( demarshall( param.firstChild().toElement() ) );
            param = param.nextSibling();
        }
    }
}

QList< QVariant > XMLRPC::RequestMessage::args() const
{
    return m_args;
}

QByteArray XMLRPC::RequestMessage::method() const
{
    return m_method;
}

void XMLRPC::RequestMessage::writeXml( QXmlStreamWriter *writer ) const
{
    writer->writeStartElement("methodCall");
    writer->writeTextElement("methodName", m_method );
    if( !m_args.isEmpty() )
    {
        writer->writeStartElement("params");
        foreach( QVariant arg, m_args)
        {
            writer->writeStartElement("param");
            marshall( writer, arg );
            writer->writeEndElement();
        }
        writer->writeEndElement();
    }
    writer->writeEndElement();
}

XMLRPC::ResponseMessage::ResponseMessage( const QList< QVariant > & theValue  )
: MessageBase(), m_values(theValue)
{
}

XMLRPC::ResponseMessage::ResponseMessage( const QDomElement &element )
{
    const QDomElement contents = element.firstChild().toElement();
    if( contents.tagName().toLower() == "params")
    {
        QDomNode param = contents.firstChild();
        while( !param.isNull() && isValid() )
        {
            m_values.append( demarshall( param.firstChild().toElement() ) );
            param = param.nextSibling();
        }
    }
    else if( contents.tagName().toLower() == "fault")
    {
        const QDomElement errElement = contents.firstChild().toElement();
        const QVariant error = demarshall( errElement );

        setError( QString("XMLRPC Fault %1: %2")
                        .arg(error.toMap()["faultCode"].toString() )
                        .arg(error.toMap()["faultString"].toString() ) );
    }
    else
    {
        setError("Bad XML response");
    }
}

QList< QVariant > XMLRPC::ResponseMessage::values() const
{
    return m_values;
}

void XMLRPC::ResponseMessage::writeXml( QXmlStreamWriter *writer ) const
{
    writer->writeStartElement("methodResponse");

    if( !m_values.isEmpty() )
    {
        writer->writeStartElement("params");
        foreach( QVariant arg, m_values)
        {
            writer->writeStartElement("param");
            marshall( writer, arg );
            writer->writeEndElement();
        }
        writer->writeEndElement();
    }
    writer->writeEndElement();
}

