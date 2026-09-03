#pragma once
#include <QString>
#include <QStringList>

class Command
{
public:
    virtual ~Command() = default;

    virtual QString program() const = 0;
    virtual QStringList arguments() const = 0;
};
