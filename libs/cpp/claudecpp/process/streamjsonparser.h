#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QStringList>
#include <QVector>

struct ParseResult
{
    QVector<QJsonObject> events;
    QStringList invalidLines;
};

class StreamJsonParser
{
public:
    ParseResult feed(const QByteArray& data);
    QByteArray pending() const;
    void reset();

private:
    QByteArray m_buffer;
};
