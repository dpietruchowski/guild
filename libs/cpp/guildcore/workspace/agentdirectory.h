#pragma once
#include <QString>

class AgentDirectory
{
public:
    AgentDirectory() = default;
    AgentDirectory(const QString& name, const QString& path);

    bool isValid() const;

    QString name() const;
    QString path() const;

    QString configFile() const;
    QString promptFile() const;
    QString memoryPath() const;
    QString transcriptPath() const;

private:
    QString child(const QString& relative) const;

    QString m_name;
    QString m_path;
};
