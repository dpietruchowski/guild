#pragma once
#include <QString>
#include <QStringList>

#include "agentdirectory.h"

class Workspace
{
public:
    explicit Workspace(const QString& path);

    static Workspace fromEnvironment();

    bool exists() const;

    QString path() const;
    QString agentsPath() const;
    QString sharedPromptsPath() const;
    QString sharedSkillsPath() const;

    QStringList agentNames() const;
    bool hasAgent(const QString& name) const;
    AgentDirectory agent(const QString& name) const;

private:
    QString m_path;
};
