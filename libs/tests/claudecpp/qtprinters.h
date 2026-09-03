#pragma once
#include <QString>
#include <QStringList>
#include <ostream>

inline void PrintTo(const QString& value, std::ostream* stream)
{
    *stream << '"' << value.toStdString() << '"';
}

inline void PrintTo(const QStringList& value, std::ostream* stream)
{
    *stream << '[';
    for (int i = 0; i < value.size(); ++i)
    {
        if (i > 0)
        {
            *stream << ", ";
        }
        *stream << '"' << value.at(i).toStdString() << '"';
    }
    *stream << ']';
}
