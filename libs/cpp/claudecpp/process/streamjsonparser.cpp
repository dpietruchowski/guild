#include "streamjsonparser.h"

#include <QJsonDocument>
#include <QJsonParseError>

ParseResult StreamJsonParser::feed(const QByteArray& data)
{
    m_buffer.append(data);

    ParseResult result;

    int newline = m_buffer.indexOf('\n');
    while (newline >= 0)
    {
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);

        if (!line.isEmpty())
        {
            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(line, &error);
            if (error.error == QJsonParseError::NoError && document.isObject())
            {
                result.events.append(document.object());
            }
            else
            {
                result.invalidLines.append(QString::fromUtf8(line));
            }
        }

        newline = m_buffer.indexOf('\n');
    }

    return result;
}

QByteArray StreamJsonParser::pending() const { return m_buffer; }

void StreamJsonParser::reset() { m_buffer.clear(); }
